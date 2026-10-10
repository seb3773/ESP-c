#include "esp_int.h"

#include <stdlib.h>
#include <string.h>

int esp_lzss_decode_key(const uint8_t *in, size_t in_len, EspBuf *out, uint32_t key)
{
	uint8_t *plain;
	EspBitReader br;
	EspHuffTree h1, h2;
	int a, dist, len, ready;
	int rc;

	plain = (uint8_t *)malloc(in_len ? in_len : 1);
	if (!plain)
		return ESP_ERR_MEM;
	if (in_len)
		memcpy(plain, in, in_len);
	esp_xor_key(plain, in_len, key);

	if (esp_buf_init(out, 256) != ESP_OK) {
		free(plain);
		return ESP_ERR_MEM;
	}

	esp_br_init(&br, plain, in_len);
	(void)esp_getbits(&br, 1);

	if (esp_read_huff_table(&br, &h1, ESP_LIT_SYMS) != ESP_OK) {
		free(plain);
		esp_buf_free(out);
		return ESP_ERR_HUFF;
	}
	if (esp_read_huff_table(&br, &h2, ESP_LEN_SYMS) != ESP_OK) {
		free(plain);
		esp_buf_free(out);
		return ESP_ERR_HUFF;
	}

	ready = 0;
	while (!ready) {
		a = esp_get_huff(&br, &h1);
		if (a < 0x100) {
			rc = esp_buf_putc(out, (uint8_t)a);
			if (rc != ESP_OK) {
				free(plain);
				esp_buf_free(out);
				return rc;
			}
			continue;
		}
		dist = a - 0x100;
		if (dist > 1) {
			int n = dist - 1;
			int extra;
			dist = 1 << n;
			extra = esp_getbits(&br, n);
			if (extra == 0x3fff) {
				a = esp_getbits(&br, 1);
				if (a == 0) {
					if (esp_read_huff_table(&br, &h1, ESP_LIT_SYMS) != ESP_OK ||
					    esp_read_huff_table(&br, &h2, ESP_LEN_SYMS) != ESP_OK) {
						free(plain);
						esp_buf_free(out);
						return ESP_ERR_HUFF;
					}
					continue;
				}
				ready = 1;
				break;
			}
			dist |= extra;
		}
		dist += 1;
		len = 3 + esp_get_huff(&br, &h2);
		while (len-- > 0) {
			size_t i = out->len;
			uint8_t c = 0;
			if ((int)i >= dist)
				c = out->data[i - (size_t)dist];
			rc = esp_buf_putc(out, c);
			if (rc != ESP_OK) {
				free(plain);
				esp_buf_free(out);
				return rc;
			}
		}
	}

	free(plain);
	return ESP_OK;
}

int esp_lzss_decode(const uint8_t *in, size_t in_len, EspBuf *out)
{
	return esp_lzss_decode_key(in, in_len, out, ESP_XOR_KEY);
}

static uint32_t hash3(const uint8_t *p)
{
	return ((uint32_t)p[0] << 8) ^ ((uint32_t)p[1] << 3) ^ p[2];
}

static int find_match(const uint8_t *in, size_t n, size_t pos,
		      uint32_t *head, uint32_t *prev, int max_chain,
		      int min_match, int *out_dist, int *out_len)
{
	int best_len = min_match - 1;
	int best_dist = 0;
	int chain = max_chain;
	uint32_t h;
	uint32_t p;
	size_t max_len;
	size_t window;

	if (pos + (size_t)min_match > n)
		return 0;
	max_len = n - pos;
	if (max_len > ESP_MAX_MATCH)
		max_len = ESP_MAX_MATCH;
	window = pos > (ESP_DICT_SIZE - 1) ? pos - (ESP_DICT_SIZE - 1) : 0;
	if (pos == 0 && in[0] == 0 && min_match == 3) {
		int z = 0;
		while ((size_t)z < max_len && in[pos + (size_t)z] == 0)
			z++;
		if (z >= min_match) {
			best_len = z;
			best_dist = 1;
		}
	}
	h = hash3(in + pos) & (ESP_HASH_SIZE - 1);
	p = head[h];

	while (chain-- > 0 && p != 0xFFFFFFFFu) {
		size_t mpos = p;
		int ml;
		if (mpos >= pos || mpos < window)
			break;
		if (in[mpos] == in[pos] && in[mpos + 1] == in[pos + 1] &&
		    in[mpos + 2] == in[pos + 2]) {
			ml = 3;
			while ((size_t)ml < max_len && in[mpos + (size_t)ml] == in[pos + (size_t)ml])
				ml++;
			if (ml > best_len) {
				best_len = ml;
				best_dist = (int)(pos - mpos);
				if ((size_t)best_len >= max_len)
					break;
			}
		}
		p = prev[mpos & (ESP_DICT_SIZE - 1)];
		if (p >= mpos)
			break;
	}
	if (best_len >= min_match) {
		*out_dist = best_dist;
		*out_len = best_len;
		return 1;
	}
	return 0;
}

static void insert_hash(const uint8_t *in, size_t n, size_t pos,
			uint32_t *head, uint32_t *prev)
{
	uint32_t h;
	if (pos + 2 >= n)
		return;
	h = hash3(in + pos) & (ESP_HASH_SIZE - 1);
	prev[pos & (ESP_DICT_SIZE - 1)] = head[h];
	head[h] = (uint32_t)pos;
}

static void token_freqs(const EspToken *tok, int nt, int *lf, int *mf)
{
	int i;
	memset(lf, 0, ESP_LIT_SYMS * sizeof(int));
	memset(mf, 0, ESP_LEN_SYMS * sizeof(int));
	for (i = 0; i < nt; i++) {
		if (tok[i].kind == ESP_TOK_LIT) {
			lf[tok[i].lit]++;
		} else if (tok[i].kind == ESP_TOK_MATCH) {
			int eb, ex, slot;
			slot = esp_dist_slot(tok[i].dist, &eb, &ex);
			lf[0x100 + slot]++;
			mf[tok[i].len - 3]++;
		} else if (tok[i].kind == ESP_TOK_END ||
			   tok[i].kind == ESP_TOK_NEWTABLE) {
			lf[0x10F]++;
			mf[0]++;
		}
	}
}

static void emit_tokens(EspBitWriter *bw, const EspToken *tok, int nt,
			const uint32_t *lc, const int *ll,
			const uint32_t *mc, const int *ml)
{
	int i;
	for (i = 0; i < nt; i++) {
		if (tok[i].kind == ESP_TOK_LIT) {
			esp_put_huff(bw, lc[tok[i].lit], ll[tok[i].lit]);
		} else if (tok[i].kind == ESP_TOK_MATCH) {
			int eb, ex, slot;
			slot = esp_dist_slot(tok[i].dist, &eb, &ex);
			esp_put_huff(bw, lc[0x100 + slot], ll[0x100 + slot]);
			if (eb)
				esp_putbits(bw, (uint32_t)ex, eb);
			esp_put_huff(bw, mc[tok[i].len - 3], ml[tok[i].len - 3]);
		} else if (tok[i].kind == ESP_TOK_NEWTABLE) {
			esp_put_huff(bw, lc[0x10F], ll[0x10F]);
			esp_putbits(bw, 0x3fff, 14);
			esp_putbits(bw, 0, 1);
		} else if (tok[i].kind == ESP_TOK_END) {
			esp_put_huff(bw, lc[0x10F], ll[0x10F]);
			esp_putbits(bw, 0x3fff, 14);
			esp_putbits(bw, 1, 1);
		}
	}
}

static int encode_block(EspBitWriter *bw, const EspToken *tok, int nt)
{
	int lf[ESP_LIT_SYMS];
	int mf[ESP_LEN_SYMS];
	int lb[ESP_LIT_SYMS];
	int mb[ESP_LEN_SYMS];
	uint32_t lc[ESP_LIT_SYMS];
	uint32_t mc[ESP_LEN_SYMS];
	int ll[ESP_LIT_SYMS];
	int ml[ESP_LEN_SYMS];

	token_freqs(tok, nt, lf, mf);
	if (lf[0x10F] == 0)
		lf[0x10F] = 1;
	if (mf[0] == 0)
		mf[0] = 1;
	esp_huff_lengths_from_freq(lf, ESP_LIT_SYMS, lb);
	esp_huff_lengths_from_freq(mf, ESP_LEN_SYMS, mb);
	if (esp_write_huff_table(bw, lb, ESP_LIT_SYMS) != ESP_OK)
		return ESP_ERR_HUFF;
	if (esp_write_huff_table(bw, mb, ESP_LEN_SYMS) != ESP_OK)
		return ESP_ERR_HUFF;
	esp_huff_codes_from_lengths(lb, ESP_LIT_SYMS, lc, ll);
	esp_huff_codes_from_lengths(mb, ESP_LEN_SYMS, mc, ml);
	emit_tokens(bw, tok, nt, lc, ll, mc, ml);
	return ESP_OK;
}

#define M0_HASH_SIZE 4096
#define M0_MAX_MATCH 256

static inline uint16_t m0_update_hash(uint16_t h, uint8_t byte)
{
	uint16_t bx = (uint16_t)(h << 4);
	uint8_t bl = (uint8_t)((bx & 0xFF) + byte);
	uint8_t bh = (uint8_t)((bx >> 8) & 0x0F);
	return (uint16_t)((bh << 8) | bl);
}

static inline int m0_is_valid_cand(uint16_t bp, uint16_t cand)
{
	return ((int16_t)(bp - cand) >= 0) && (bp != cand);
}

static inline uint16_t m0_get_u16(const uint8_t *buf, size_t n, size_t pos)
{
	if (pos + 1 < n)
		return (uint16_t)(buf[pos] | (buf[pos + 1] << 8));
	if (pos < n)
		return (uint16_t)buf[pos];
	return 0;
}

static int m0_compare_strings(const uint8_t *buf, size_t n, size_t pos1, size_t pos2, int max_l)
{
	int l = 0;
	while (l < max_l && pos1 + (size_t)l < n && pos2 + (size_t)l < n && buf[pos1 + l] == buf[pos2 + l])
		l++;
	return l;
}

typedef struct {
	uint16_t head[M0_HASH_SIZE];
	uint16_t prev[32768];
	const uint8_t *buf;
	size_t n;
	int max_chain;

	uint16_t best_pos;
	int best_len;
	uint16_t clamp_val;
} M0Engine;

static inline void m0_check_clamp(M0Engine *e, uint16_t bp)
{
	uint16_t diff = (uint16_t)(e->clamp_val - bp);
	if (diff <= 0x100) {
		uint16_t ax = (uint16_t)(bp - 0x8000);
		int i;
		e->clamp_val = ax;
		for (i = 0; i < M0_HASH_SIZE; i++) {
			if ((int16_t)(e->head[i] - ax) < 0)
				e->head[i] = ax;
		}
	}
}

static void m0_suffix_chain(M0Engine *e, uint16_t bp, size_t cur_pos, int target_len, uint16_t bx, int ax, int dx)
{
	int si = ax - target_len;
	uint16_t cur_w = m0_get_u16(e->buf, e->n, cur_pos);

loop_start:
	while (m0_is_valid_cand(bp, bx)) {
		int cand_prefix;
		dx--;
		if (dx < 0)
			return;

		cand_prefix = (int)bx + si;
		if (cand_prefix >= 0) {
			uint16_t dist = (uint16_t)((uint16_t)cur_pos - (uint16_t)cand_prefix);
			if (cur_pos >= dist) {
				size_t cand_pos = cur_pos - dist;
				uint16_t cand_w = m0_get_u16(e->buf, e->n, cand_pos);
				if (cand_w == cur_w) {
					int ml = m0_compare_strings(e->buf, e->n, cur_pos, cand_pos, M0_MAX_MATCH);
					if (ml > e->best_len) {
						uint16_t t1, cx, t2, cx2, nxt;
						e->best_len = ml;
						e->best_pos = (uint16_t)cand_prefix;
						t1 = (uint16_t)(cur_pos + (size_t)ml - 2);
						cx = e->prev[t1 & 0x7FFF];
						if (!m0_is_valid_cand(bp, cx))
							return;
						if ((int16_t)(cx - bx) < 0) {
							bx = cx;
							si = -(ml - 2);
							goto loop_start;
						}
						t2 = (uint16_t)(cand_prefix + ml - 3);
						cx2 = e->prev[t2 & 0x7FFF];
						if (!m0_is_valid_cand(bp, cx2))
							return;
						nxt = e->prev[bx & 0x7FFF];
						if (!m0_is_valid_cand(bp, nxt))
							return;
						if ((int16_t)(cx2 - nxt) < 0) {
							bx = cx2;
							si = -(ml - 3);
							goto loop_start;
						}
						bx = nxt;
						goto loop_start;
					} else {
						bx = e->prev[bx & 0x7FFF];
						if (!m0_is_valid_cand(bp, bx))
							return;
						goto loop_start;
					}
				}
			}
		}

		bx = e->prev[bx & 0x7FFF];
		if (!m0_is_valid_cand(bp, bx))
			return;

		dx--;
		cand_prefix = (int)bx + si;
		if (cand_prefix >= 0) {
			uint16_t dist = (uint16_t)((uint16_t)cur_pos - (uint16_t)cand_prefix);
			if (cur_pos >= dist) {
				size_t cand_pos = cur_pos - dist;
				uint16_t cand_w = m0_get_u16(e->buf, e->n, cand_pos);
				if (cand_w == cur_w) {
					int ml = m0_compare_strings(e->buf, e->n, cur_pos, cand_pos, M0_MAX_MATCH);
					if (ml > e->best_len) {
						uint16_t t1, cx, t2, cx2, nxt;
						e->best_len = ml;
						e->best_pos = (uint16_t)cand_prefix;
						t1 = (uint16_t)(cur_pos + (size_t)ml - 2);
						cx = e->prev[t1 & 0x7FFF];
						if (!m0_is_valid_cand(bp, cx))
							return;
						if ((int16_t)(cx - bx) < 0) {
							bx = cx;
							si = -(ml - 2);
							goto loop_start;
						}
						t2 = (uint16_t)(cand_prefix + ml - 3);
						cx2 = e->prev[t2 & 0x7FFF];
						if (!m0_is_valid_cand(bp, cx2))
							return;
						nxt = e->prev[bx & 0x7FFF];
						if (!m0_is_valid_cand(bp, nxt))
							return;
						if ((int16_t)(cx2 - nxt) < 0) {
							bx = cx2;
							si = -(ml - 3);
							goto loop_start;
						}
						bx = nxt;
						goto loop_start;
					} else {
						bx = e->prev[bx & 0x7FFF];
						if (!m0_is_valid_cand(bp, bx))
							return;
						goto loop_start;
					}
				}
			}
		}

		bx = e->prev[bx & 0x7FFF];
		if (!m0_is_valid_cand(bp, bx))
			return;

		dx--;
		cand_prefix = (int)bx + si;
		if (cand_prefix >= 0) {
			uint16_t dist = (uint16_t)((uint16_t)cur_pos - (uint16_t)cand_prefix);
			if (cur_pos >= dist) {
				size_t cand_pos = cur_pos - dist;
				uint16_t cand_w = m0_get_u16(e->buf, e->n, cand_pos);
				if (cand_w == cur_w) {
					int ml = m0_compare_strings(e->buf, e->n, cur_pos, cand_pos, M0_MAX_MATCH);
					if (ml > e->best_len) {
						uint16_t t1, cx, t2, cx2, nxt;
						e->best_len = ml;
						e->best_pos = (uint16_t)cand_prefix;
						t1 = (uint16_t)(cur_pos + (size_t)ml - 2);
						cx = e->prev[t1 & 0x7FFF];
						if (!m0_is_valid_cand(bp, cx))
							return;
						if ((int16_t)(cx - bx) < 0) {
							bx = cx;
							si = -(ml - 2);
							goto loop_start;
						}
						t2 = (uint16_t)(cand_prefix + ml - 3);
						cx2 = e->prev[t2 & 0x7FFF];
						if (!m0_is_valid_cand(bp, cx2))
							return;
						nxt = e->prev[bx & 0x7FFF];
						if (!m0_is_valid_cand(bp, nxt))
							return;
						if ((int16_t)(cx2 - nxt) < 0) {
							bx = cx2;
							si = -(ml - 3);
							goto loop_start;
						}
						bx = nxt;
						goto loop_start;
					} else {
						bx = e->prev[bx & 0x7FFF];
						if (!m0_is_valid_cand(bp, bx))
							return;
						goto loop_start;
					}
				}
			}
		}

		bx = e->prev[bx & 0x7FFF];
	}
}

static void m0_find_match_64c(M0Engine *e, uint16_t bp, size_t cur_pos)
{
	uint16_t cand = e->prev[cur_pos & 0x7FFF];
	int dx = e->max_chain;
	uint16_t cur_w = m0_get_u16(e->buf, e->n, cur_pos);

	e->best_len = 1;
	e->best_pos = 0;

loop_start_64c:
	while (m0_is_valid_cand(bp, cand)) {
		uint16_t dist;
		int test_match;

		dx--;
		if (dx < 0)
			return;

		dist = (uint16_t)((uint16_t)cur_pos - cand);
		if (cur_pos >= dist) {
			size_t cand_pos = cur_pos - dist;
			test_match = 0;
			if (e->best_len <= 1) {
				uint16_t cand_w = m0_get_u16(e->buf, e->n, cand_pos);
				if (cand_w == cur_w)
					test_match = 1;
			} else {
				uint16_t cur_end = m0_get_u16(e->buf, e->n, cur_pos + (size_t)e->best_len - 1);
				uint16_t cand_end = m0_get_u16(e->buf, e->n, cand_pos + (size_t)e->best_len - 1);
				if (cand_end == cur_end)
					test_match = 1;
			}
			if (test_match) {
				int ml = m0_compare_strings(e->buf, e->n, cur_pos, cand_pos, M0_MAX_MATCH);
				if (ml > e->best_len) {
					uint16_t t1, cx, t2, cx2, nxt;
					e->best_len = ml;
					e->best_pos = cand;
					t1 = (uint16_t)(cur_pos + (size_t)ml - 2);
					cx = e->prev[t1 & 0x7FFF];
					if (!m0_is_valid_cand(bp, cx))
						return;
					if ((int16_t)(cx - cand) < 0) {
						m0_suffix_chain(e, bp, cur_pos, ml, cx, 2, dx);
						return;
					}
					t2 = (uint16_t)(cand + ml - 3);
					cx2 = e->prev[t2 & 0x7FFF];
					if (!m0_is_valid_cand(bp, cx2))
						return;
					nxt = e->prev[cand & 0x7FFF];
					if (!m0_is_valid_cand(bp, nxt))
						return;
					if ((int16_t)(cx2 - nxt) < 0) {
						m0_suffix_chain(e, bp, cur_pos, ml, cx2, 3, dx);
						return;
					}
					cand = nxt;
					goto loop_start_64c;
				} else {
					cand = e->prev[cand & 0x7FFF];
					if (!m0_is_valid_cand(bp, cand))
						return;
					goto loop_start_64c;
				}
			}
		}

		cand = e->prev[cand & 0x7FFF];
		if (!m0_is_valid_cand(bp, cand))
			return;

		dx--;
		dist = (uint16_t)((uint16_t)cur_pos - cand);
		if (cur_pos >= dist) {
			size_t cand_pos = cur_pos - dist;
			test_match = 0;
			if (e->best_len <= 1) {
				uint16_t cand_w = m0_get_u16(e->buf, e->n, cand_pos);
				if (cand_w == cur_w)
					test_match = 1;
			} else {
				uint16_t cur_end = m0_get_u16(e->buf, e->n, cur_pos + (size_t)e->best_len - 1);
				uint16_t cand_end = m0_get_u16(e->buf, e->n, cand_pos + (size_t)e->best_len - 1);
				if (cand_end == cur_end)
					test_match = 1;
			}
			if (test_match) {
				int ml = m0_compare_strings(e->buf, e->n, cur_pos, cand_pos, M0_MAX_MATCH);
				if (ml > e->best_len) {
					uint16_t t1, cx, t2, cx2, nxt;
					e->best_len = ml;
					e->best_pos = cand;
					t1 = (uint16_t)(cur_pos + (size_t)ml - 2);
					cx = e->prev[t1 & 0x7FFF];
					if (!m0_is_valid_cand(bp, cx))
						return;
					if ((int16_t)(cx - cand) < 0) {
						m0_suffix_chain(e, bp, cur_pos, ml, cx, 2, dx);
						return;
					}
					t2 = (uint16_t)(cand + ml - 3);
					cx2 = e->prev[t2 & 0x7FFF];
					if (!m0_is_valid_cand(bp, cx2))
						return;
					nxt = e->prev[cand & 0x7FFF];
					if (!m0_is_valid_cand(bp, nxt))
						return;
					if ((int16_t)(cx2 - nxt) < 0) {
						m0_suffix_chain(e, bp, cur_pos, ml, cx2, 3, dx);
						return;
					}
					cand = nxt;
					goto loop_start_64c;
				} else {
					cand = e->prev[cand & 0x7FFF];
					if (!m0_is_valid_cand(bp, cand))
						return;
					goto loop_start_64c;
				}
			}
		}

		cand = e->prev[cand & 0x7FFF];
		if (!m0_is_valid_cand(bp, cand))
			return;

		dx--;
		dist = (uint16_t)((uint16_t)cur_pos - cand);
		if (cur_pos >= dist) {
			size_t cand_pos = cur_pos - dist;
			test_match = 0;
			if (e->best_len <= 1) {
				uint16_t cand_w = m0_get_u16(e->buf, e->n, cand_pos);
				if (cand_w == cur_w)
					test_match = 1;
			} else {
				uint16_t cur_end = m0_get_u16(e->buf, e->n, cur_pos + (size_t)e->best_len - 1);
				uint16_t cand_end = m0_get_u16(e->buf, e->n, cand_pos + (size_t)e->best_len - 1);
				if (cand_end == cur_end)
					test_match = 1;
			}
			if (test_match) {
				int ml = m0_compare_strings(e->buf, e->n, cur_pos, cand_pos, M0_MAX_MATCH);
				if (ml > e->best_len) {
					uint16_t t1, cx, t2, cx2, nxt;
					e->best_len = ml;
					e->best_pos = cand;
					t1 = (uint16_t)(cur_pos + (size_t)ml - 2);
					cx = e->prev[t1 & 0x7FFF];
					if (!m0_is_valid_cand(bp, cx))
						return;
					if ((int16_t)(cx - cand) < 0) {
						m0_suffix_chain(e, bp, cur_pos, ml, cx, 2, dx);
						return;
					}
					t2 = (uint16_t)(cand + ml - 3);
					cx2 = e->prev[t2 & 0x7FFF];
					if (!m0_is_valid_cand(bp, cx2))
						return;
					nxt = e->prev[cand & 0x7FFF];
					if (!m0_is_valid_cand(bp, nxt))
						return;
					if ((int16_t)(cx2 - nxt) < 0) {
						m0_suffix_chain(e, bp, cur_pos, ml, cx2, 3, dx);
						return;
					}
					cand = nxt;
					goto loop_start_64c;
				} else {
					cand = e->prev[cand & 0x7FFF];
					if (!m0_is_valid_cand(bp, cand))
						return;
					goto loop_start_64c;
				}
			}
		}

		cand = e->prev[cand & 0x7FFF];
	}
}

static void m0_find_match_5f4(M0Engine *e, uint16_t bp, size_t cur_pos, int prev_len)
{
	uint16_t si, cand0, cand1, bx, dx, best_cand;
	int ax, cx;

	if (prev_len < 3) {
		m0_find_match_64c(e, bp, cur_pos);
		return;
	}

	si = (uint16_t)((cur_pos - 2) + (size_t)prev_len);
	cand0 = e->prev[si & 0x7FFF];
	if (!m0_is_valid_cand(bp, cand0)) {
		e->best_len = 1;
		e->best_pos = 0;
		return;
	}
	cand1 = e->prev[(si - 1) & 0x7FFF];
	if (!m0_is_valid_cand(bp, cand1)) {
		e->best_len = 1;
		e->best_pos = 0;
		return;
	}

	bx = (uint16_t)(bp - cand0);
	dx = (uint16_t)(bp - cand1);
	ax = -(prev_len - 2);
	cx = ax;

	if ((int16_t)(dx - bx) >= 0) {
		bx = dx;
		cx++;
	}

	si -= 2;
	ax += 2;
	while (ax <= 0) {
		uint16_t cand = e->prev[si & 0x7FFF];
		dx = (uint16_t)(bp - cand);
		si--;
		if ((int16_t)(dx - bx) >= 0) {
			bx = dx;
			cx = ax;
		}
		ax++;
	}

	best_cand = (uint16_t)(bp - bx);

	if (cx == 0) {
		uint16_t cand = best_cand;
		int dx_val = e->max_chain;
		int cur_chain_len = 0;
		uint16_t cur_w = m0_get_u16(e->buf, e->n, cur_pos);

		e->best_len = 1;
		e->best_pos = 0;

		while (m0_is_valid_cand(bp, cand)) {
			uint16_t dist;
			dx_val--;
			if (dx_val < 0)
				return;
			dist = (uint16_t)((uint16_t)cur_pos - cand);
			if (cur_pos >= dist) {
				size_t cand_pos = cur_pos - dist;
				uint16_t cand_w = m0_get_u16(e->buf, e->n, cand_pos);
				if (cand_w == cur_w) {
					int ml = m0_compare_strings(e->buf, e->n, cur_pos, cand_pos, M0_MAX_MATCH);
					if (ml > cur_chain_len && ml >= 3) {
						uint16_t t1, cx_h, t2, cx2, nxt;
						cur_chain_len = ml;
						if (ml > e->best_len) {
							e->best_len = ml;
							e->best_pos = cand;
						}
						t1 = (uint16_t)(cur_pos + (size_t)ml - 2);
						cx_h = e->prev[t1 & 0x7FFF];
						if (!m0_is_valid_cand(bp, cx_h))
							return;
						if ((int16_t)(cx_h - cand) < 0) {
							m0_suffix_chain(e, bp, cur_pos, ml, cx_h, 2, dx_val);
							return;
						}
						t2 = (uint16_t)(cand + ml - 3);
						cx2 = e->prev[t2 & 0x7FFF];
						if (!m0_is_valid_cand(bp, cx2))
							return;
						nxt = e->prev[cand & 0x7FFF];
						if (!m0_is_valid_cand(bp, nxt))
							return;
						if ((int16_t)(cx2 - nxt) < 0) {
							m0_suffix_chain(e, bp, cur_pos, ml, cx2, 3, dx_val);
							return;
						}
						cand = nxt;
						continue;
					}
				}
			}
			cand = e->prev[cand & 0x7FFF];
		}
	} else {
		m0_suffix_chain(e, bp, cur_pos, prev_len, best_cand, cx + prev_len, e->max_chain);
	}
}

static int encode_block_dos(EspBitWriter *bw, const EspToken *tokens, int nt, int is_last)
{
	uint16_t lf[ESP_LIT_SYMS], mf[ESP_LEN_SYMS];
	uint8_t lb[ESP_LIT_SYMS], mb[ESP_LEN_SYMS];
	uint32_t lc[ESP_LIT_SYMS], mc[ESP_LEN_SYMS];
	int ll[ESP_LIT_SYMS], ml[ESP_LEN_SYMS];
	int lb_int[ESP_LIT_SYMS], mb_int[ESP_LEN_SYMS];
	int i;

	memset(lf, 0, sizeof(lf));
	memset(mf, 0, sizeof(mf));
	for (i = 0; i < nt; i++) {
		if (tokens[i].kind == ESP_TOK_LIT) {
			lf[tokens[i].lit]++;
		} else if (tokens[i].kind == ESP_TOK_MATCH) {
			int eb, ex, slot;
			slot = esp_dist_slot(tokens[i].dist, &eb, &ex);
			lf[0x100 + slot]++;
			mf[tokens[i].len - 3]++;
		}
	}
	lf[0x10F]++;
	mf[0]++;

	esp_dos_build_tree(ESP_LIT_SYMS, lf, lb);
	esp_dos_build_tree(ESP_LEN_SYMS, mf, mb);

	for (i = 0; i < ESP_LIT_SYMS; i++)
		lb_int[i] = (int)lb[i];
	for (i = 0; i < ESP_LEN_SYMS; i++)
		mb_int[i] = (int)mb[i];

	if (esp_write_huff_table(bw, lb_int, ESP_LIT_SYMS) != ESP_OK)
		return ESP_ERR_HUFF;
	if (esp_write_huff_table(bw, mb_int, ESP_LEN_SYMS) != ESP_OK)
		return ESP_ERR_HUFF;

	esp_huff_codes_from_lengths(lb_int, ESP_LIT_SYMS, lc, ll);
	esp_huff_codes_from_lengths(mb_int, ESP_LEN_SYMS, mc, ml);

	for (i = 0; i < nt; i++) {
		if (tokens[i].kind == ESP_TOK_LIT) {
			esp_put_huff(bw, lc[tokens[i].lit], ll[tokens[i].lit]);
		} else if (tokens[i].kind == ESP_TOK_MATCH) {
			int eb, ex, slot;
			slot = esp_dist_slot(tokens[i].dist, &eb, &ex);
			esp_put_huff(bw, lc[0x100 + slot], ll[0x100 + slot]);
			if (eb)
				esp_putbits(bw, (uint32_t)ex, eb);
			esp_put_huff(bw, mc[tokens[i].len - 3], ml[tokens[i].len - 3]);
		}
	}

	esp_put_huff(bw, lc[0x10F], ll[0x10F]);
	esp_putbits(bw, 0x3fff, 14);
	if (is_last)
		esp_putbits(bw, 1, 1);
	else
		esp_putbits(bw, 0, 1);

	return ESP_OK;
}

static int esp_m0_encode(const uint8_t *in, size_t in_len, EspBuf *out, uint32_t key)
{
	size_t full_size;
	uint8_t *full_buf;
	M0Engine eng;
	uint16_t h, bp;
	size_t cur_pos;
	int curr_len;
	uint16_t curr_pos_cand;
	EspBuf raw;
	EspBitWriter bw;
	EspToken *block_tokens;
	int b_nt;
	int sp, di, dh, dl, batch_start;
	int rc, i;

	if (esp_buf_init(out, 64) != ESP_OK)
		return ESP_ERR_MEM;

	full_size = 256 + in_len;
	full_buf = (uint8_t *)calloc(1, full_size + 1024);
	if (!full_buf) {
		esp_buf_free(out);
		return ESP_ERR_MEM;
	}
	if (in_len > 0)
		memcpy(full_buf + 256, in, in_len);
	if (in_len > 8192) {
		for (i = 0; i < 256; i++)
			full_buf[full_size + i] = full_buf[full_size + i - 8192];
	}

	memset(&eng, 0, sizeof(eng));
	for (i = 0; i < M0_HASH_SIZE; i++)
		eng.head[i] = 0x8100;
	for (i = 0; i < 32768; i++)
		eng.prev[i] = 0x8100;
	eng.prev[0xFF] = 0x80FF;
	eng.buf = full_buf;
	eng.n = full_size;
	eng.max_chain = 256;
	eng.clamp_val = 0x8100;

	h = 0;
	bp = 0x00FD;
	for (i = 0; i < 0x102; i++) {
		uint8_t b;
		uint16_t old_h;
		bp++;
		b = ((size_t)bp + 2 < full_size) ? full_buf[bp + 2] : 0;
		h = m0_update_hash(h, b);
		old_h = eng.head[h];
		eng.head[h] = bp;
		eng.prev[bp & 0x7FFF] = ((int16_t)(bp - old_h) >= 0) ? old_h : (uint16_t)(bp - 0x8000);
	}

	cur_pos = 0x100;
	m0_find_match_64c(&eng, bp, cur_pos);
	curr_len = eng.best_len;
	curr_pos_cand = eng.best_pos;
	bp++;

	if (esp_buf_init(&raw, 4096) != ESP_OK) {
		free(full_buf);
		esp_buf_free(out);
		return ESP_ERR_MEM;
	}
	esp_bw_init(&bw, &raw);
	esp_putbits(&bw, 0, 1);

	block_tokens = (EspToken *)malloc(32768 * sizeof(EspToken));
	if (!block_tokens) {
		esp_buf_free(&raw);
		free(full_buf);
		esp_buf_free(out);
		return ESP_ERR_MEM;
	}
	b_nt = 0;

	sp = 0xcd54;
	di = 0x1384;
	dh = 0x80;
	dl = 0;
	batch_start = 0;

	while (cur_pos < full_size) {
		size_t lookahead_off;
		uint8_t b;
		uint16_t old_h;
		int prev_match_len;
		uint16_t prev_match_pos;

		m0_check_clamp(&eng, bp);

		lookahead_off = cur_pos + 256;
		b = (lookahead_off + 2 < full_size) ? full_buf[lookahead_off + 2] : 0;
		h = m0_update_hash(h, b);
		old_h = eng.head[h];
		eng.head[h] = bp;
		eng.prev[bp & 0x7FFF] = ((int16_t)(bp - old_h) >= 0) ? old_h : (uint16_t)(bp - 0x8000);

		prev_match_len = curr_len;
		prev_match_pos = curr_pos_cand;

		m0_find_match_5f4(&eng, bp, cur_pos + 1, prev_match_len);
		curr_len = eng.best_len;
		curr_pos_cand = eng.best_pos;

		if (prev_match_len < 3) {
			block_tokens[b_nt].kind = ESP_TOK_LIT;
			block_tokens[b_nt].lit = full_buf[cur_pos];
			b_nt++;
			sp -= 2;
			cur_pos++;
			bp++;
		} else if (prev_match_len >= curr_len) {
			uint16_t dist;
			int k;
			if (cur_pos + (size_t)prev_match_len > full_size)
				prev_match_len = (int)(full_size - cur_pos);
			dist = (uint16_t)((uint16_t)cur_pos - prev_match_pos);
			block_tokens[b_nt].kind = ESP_TOK_MATCH;
			block_tokens[b_nt].dist = dist;
			block_tokens[b_nt].len = prev_match_len;
			b_nt++;
			sp -= 4;

			for (k = 1; k < prev_match_len; k++) {
				size_t k_off;
				bp++;
				m0_check_clamp(&eng, bp);
				k_off = cur_pos + (size_t)k + 256;
				b = (k_off + 2 < full_size) ? full_buf[k_off + 2] : 0;
				h = m0_update_hash(h, b);
				old_h = eng.head[h];
				eng.head[h] = bp;
				eng.prev[bp & 0x7FFF] = ((int16_t)(bp - old_h) >= 0) ? old_h : (uint16_t)(bp - 0x8000);
			}
			cur_pos += (size_t)prev_match_len;
			m0_find_match_64c(&eng, bp, cur_pos);
			curr_len = eng.best_len;
			curr_pos_cand = eng.best_pos;
			bp++;
		} else {
			block_tokens[b_nt].kind = ESP_TOK_LIT;
			block_tokens[b_nt].lit = full_buf[cur_pos];
			b_nt++;
			sp -= 2;
			cur_pos++;
			bp++;
		}

		if (sp < 0xcbd4) {
			int k;
			for (k = batch_start; k < b_nt; k++) {
				dh = ((dh << 1) | (dh >> 7)) & 0xFF;
				if (dh == 1) {
					di++;
					dl = 0;
				}
				if (block_tokens[k].kind == ESP_TOK_LIT) {
					di += 1;
				} else {
					dl |= dh;
					di += 3;
				}
			}
			batch_start = b_nt;
			sp = 0xcd54;

			if (di >= 0x4df4 && cur_pos < full_size) {
				rc = encode_block_dos(&bw, block_tokens, b_nt, 0);
				if (rc != ESP_OK) {
					free(block_tokens);
					free(full_buf);
					esp_buf_free(&raw);
					return rc;
				}
				b_nt = 0;
				batch_start = 0;
				di = 0x1384;
				dh = 0x80;
				dl = 0;
			}
		}
	}

	rc = encode_block_dos(&bw, block_tokens, b_nt, 1);
	free(block_tokens);
	free(full_buf);
	if (rc != ESP_OK) {
		esp_buf_free(&raw);
		return rc;
	}

	esp_bw_flush(&bw);
	esp_xor_key(raw.data, raw.len, key);
	rc = esp_buf_append(out, raw.data, raw.len);
	esp_buf_free(&raw);
	return rc;
}

int esp_lzss_encode_key(const uint8_t *in, size_t in_len, EspBuf *out, int method, uint32_t key)
{
	uint32_t *head;
	uint32_t *prev;
	EspToken *tok;
	int nt;
	size_t pos;
	int min_match;
	int max_chain;
	int max_tok;
	EspBitWriter bw;
	EspBuf raw;
	int rc;

	if ((method & 7) == ESP_METH_BEST)
		return esp_m0_encode(in, in_len, out, key);

	if (esp_buf_init(out, 64) != ESP_OK)
		return ESP_ERR_MEM;
	if (esp_buf_init(&raw, 64) != ESP_OK) {
		esp_buf_free(out);
		return ESP_ERR_MEM;
	}

	min_match = (method == ESP_METH_FAST) ? 4 : 3;
	max_chain = (method == ESP_METH_FAST) ? ESP_MAX_CHAIN_FAST : ESP_MAX_CHAIN_BEST;
	max_tok = ESP_BLOCK_TOKENS;

	head = (uint32_t *)malloc(ESP_HASH_SIZE * sizeof(uint32_t));
	prev = (uint32_t *)malloc(ESP_DICT_SIZE * sizeof(uint32_t));
	tok = (EspToken *)malloc((size_t)(max_tok + 8) * sizeof(EspToken));
	if (!head || !prev || !tok) {
		free(head);
		free(prev);
		free(tok);
		esp_buf_free(&raw);
		esp_buf_free(out);
		return ESP_ERR_MEM;
	}
	memset(head, 0xFF, ESP_HASH_SIZE * sizeof(uint32_t));
	memset(prev, 0xFF, ESP_DICT_SIZE * sizeof(uint32_t));

	esp_bw_init(&bw, &raw);
	esp_putbits(&bw, 0, 1);

	pos = 0;
	nt = 0;
	int curr_dist = 0, curr_len = 0;
	int curr_got = 0;
	int lazy_eval = (method == ESP_METH_BEST);

	if (pos < in_len)
		curr_got = find_match(in, in_len, pos, head, prev, max_chain, min_match, &curr_dist, &curr_len);

	while (pos < in_len) {
		int checked_lazy = 0;
		if (curr_got && lazy_eval && curr_len < ESP_MAX_MATCH && pos + 1 < in_len) {
			int next_dist = 0, next_len = 0;
			int next_got;
			insert_hash(in, in_len, pos, head, prev);
			checked_lazy = 1;
			next_got = find_match(in, in_len, pos + 1, head, prev, max_chain, min_match, &next_dist, &next_len);
			if (next_got && next_len > curr_len) {
				tok[nt].kind = ESP_TOK_LIT;
				tok[nt].lit = in[pos];
				nt++;
				pos++;
				curr_got = next_got;
				curr_dist = next_dist;
				curr_len = next_len;
				goto check_block;
			}
		}

		if (curr_got) {
			size_t k;
			tok[nt].kind = ESP_TOK_MATCH;
			tok[nt].dist = curr_dist;
			tok[nt].len = curr_len;
			nt++;
			if (method == ESP_METH_BEST) {
				for (k = checked_lazy ? 1 : 0; k < (size_t)curr_len; k++)
					insert_hash(in, in_len, pos + k, head, prev);
			} else if (!checked_lazy) {
				insert_hash(in, in_len, pos, head, prev);
			}
			pos += (size_t)curr_len;
			if (pos < in_len)
				curr_got = find_match(in, in_len, pos, head, prev, max_chain, min_match, &curr_dist, &curr_len);
			else
				curr_got = 0;
		} else {
			tok[nt].kind = ESP_TOK_LIT;
			tok[nt].lit = in[pos];
			nt++;
			insert_hash(in, in_len, pos, head, prev);
			pos++;
			if (pos < in_len)
				curr_got = find_match(in, in_len, pos, head, prev, max_chain, min_match, &curr_dist, &curr_len);
			else
				curr_got = 0;
		}

check_block:
		if (nt >= max_tok - 1 && pos < in_len) {
			tok[nt].kind = ESP_TOK_NEWTABLE;
			nt++;
			rc = encode_block(&bw, tok, nt);
			if (rc != ESP_OK) {
				free(head);
				free(prev);
				free(tok);
				esp_buf_free(&raw);
				esp_buf_free(out);
				return rc;
			}
			nt = 0;
		}
	}
	tok[nt].kind = ESP_TOK_END;
	nt++;
	rc = encode_block(&bw, tok, nt);
	esp_bw_flush(&bw);
	free(head);
	free(prev);
	free(tok);
	if (rc != ESP_OK) {
		esp_buf_free(&raw);
		esp_buf_free(out);
		return rc;
	}
	esp_xor_key(raw.data, raw.len, key);
	rc = esp_buf_append(out, raw.data, raw.len);
	esp_buf_free(&raw);
	if (rc != ESP_OK)
		esp_buf_free(out);
	return rc;
}

int esp_lzss_encode(const uint8_t *in, size_t in_len, EspBuf *out, int method)
{
	return esp_lzss_encode_key(in, in_len, out, method, ESP_XOR_KEY);
}
