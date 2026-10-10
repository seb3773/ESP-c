#include "esp_int.h"

#include <stdlib.h>
#include <string.h>

static int huff_bits_counter[17];
static int16_t huff_sorted[ESP_MAX_HUFF];
static int16_t *huff_sorted_ptr;
static int huff_tabl_size;

static void huff_make_node(EspHuffTree *tree, int len, int p, int code)
{
	int p2;
	int sym;

	if ((--huff_bits_counter[len]) >= 0) {
		p2 = code;
		sym = (*huff_sorted_ptr++);
		tree->dyn[p] = (int16_t)(-sym);
		if (len > 8)
			return;
		p2 >>= (16 - len);
		sym |= (len << 10);
		do {
			tree->stat[p2] = (uint16_t)sym;
			p2 += (1 << len);
		} while (p2 < 0x100);
		return;
	}

	tree->dyn[p] = (int16_t)huff_tabl_size;
	if (len == 8)
		tree->stat[code >> 8] = (uint16_t)(huff_tabl_size | 0x8000);
	p = huff_tabl_size;
	huff_tabl_size += 2;
	code >>= 1;
	++len;
	huff_make_node(tree, len, p, code);
	huff_make_node(tree, len, p + 1, code | 0x8000);
}

int esp_huff_from_lengths(const int *bits_tabl, int code_db, EspHuffTree *tree)
{
	int first[17];
	int next[ESP_MAX_HUFF];
	int i, j, d, c, s;

	memset(tree, 0, sizeof(*tree));
	for (i = 0; i < 17; i++) {
		first[i] = -1;
		huff_bits_counter[i] = 0;
	}

	for (i = 0; i < code_db; i++) {
		int b = bits_tabl[i];
		if (b < 0 || b > 16)
			return ESP_ERR_HUFF;
		huff_bits_counter[b]++;
		next[i] = first[b];
		first[b] = i;
	}

	d = 0;
	c = 15;
	s = 1;
	do {
		d += (huff_bits_counter[s++] << c);
	} while ((--c) >= 0);
	if (d != 65536)
		return ESP_ERR_HUFF;

	j = 0;
	for (i = 1; i < 17; i++) {
		d = first[i];
		while (d >= 0) {
			huff_sorted[j++] = (int16_t)d;
			d = next[d];
		}
	}
	if (j == 0)
		return ESP_ERR_HUFF;

	huff_bits_counter[0] = 0;
	huff_sorted_ptr = huff_sorted;
	huff_tabl_size = 0;
	huff_make_node(tree, 0, 0, 0);
	return ESP_OK;
}

int esp_get_huff(EspBitReader *br, const EspHuffTree *tree)
{
	int a;
	while (8 > br->bits) {
		uint8_t b = (br->pos < br->len) ? br->data[br->pos++] : 0;
		br->acc |= ((uint32_t)b) << br->bits;
		br->bits += 8;
	}
	a = tree->stat[br->acc & 0xff];
	if (a < 0x8000) {
		int len = a >> 10;
		br->acc >>= len;
		br->bits -= len;
		return a & 0x3ff;
	}
	a &= 0x3ff;
	br->acc >>= 8;
	br->bits -= 8;
	while (16 > br->bits) {
		uint8_t b = (br->pos < br->len) ? br->data[br->pos++] : 0;
		br->acc |= ((uint32_t)b) << br->bits;
		br->bits += 8;
	}
	do {
		a = tree->dyn[a + (br->acc & 1)];
		br->bits--;
		br->acc >>= 1;
	} while (a > 0);
	return -a;
}

int esp_read_huff_table(EspBitReader *br, EspHuffTree *tree, int size)
{
	int bittabl[ESP_MAX_HUFF + 32];
	int minibits[ESP_MAX_HUFF];
	EspHuffTree mini;
	int i, n, a;

	memset(bittabl, 0, sizeof(bittabl));
	memset(minibits, 0, sizeof(minibits));

	n = 0x13 - esp_getbits(br, 5);
	if (n < 0)
		n = 0;
	i = 0;
	while (i < n) {
		a = esp_getbits(br, 3);
		if (a == 7) {
			while (esp_getbits(br, 1))
				a++;
		}
		minibits[i++] = a;
	}
	if (esp_huff_from_lengths(minibits, ESP_MINI_SYMS, &mini) != ESP_OK)
		return ESP_ERR_HUFF;

	n = size - esp_getbits(br, 9);
	if (n < 0)
		n = 0;
	i = 0;
	while (i < n) {
		a = esp_get_huff(br, &mini);
		if (a > 2) {
			bittabl[i++] = a - 2;
			continue;
		}
		if (a == 0) {
			bittabl[i++] = 0;
			continue;
		}
		if (a == 1)
			a = esp_getbits(br, 4) + 3;
		else
			a = esp_getbits(br, 9) + 0x14;
		while (a) {
			bittabl[i++] = 0;
			a--;
		}
	}
	return esp_huff_from_lengths(bittabl, size, tree);
}

static void dos_radix_sort(const uint16_t *freq, uint16_t *sorted, int count)
{
	int16_t head1[256];
	int16_t head2[256];
	int16_t next[1024];
	int i, di, low, high, out_idx;

	for (i = 0; i < 256; i++) {
		head1[i] = -1;
		head2[i] = -1;
	}

	for (di = count - 1; di >= 0; di--) {
		uint8_t l = (uint8_t)(freq[di] & 0xFF);
		next[di] = head1[l];
		head1[l] = (int16_t)di;
	}

	for (low = 0; low < 256; low++) {
		di = head1[low];
		while (di >= 0) {
			int next_di = next[di];
			uint8_t h = (uint8_t)(freq[di] >> 8);
			next[di] = head2[h];
			head2[h] = (int16_t)di;
			di = next_di;
		}
	}

	out_idx = count - 1;
	for (high = 255; high >= 0; high--) {
		di = head2[high];
		while (di >= 0) {
			sorted[out_idx--] = (uint16_t)di;
			di = next[di];
		}
	}
}

typedef struct {
	uint16_t freq;
	int16_t child;
} DosNode;

static void count_depths(const DosNode *nodes, int node_idx, int depth, uint16_t *depth_counts)
{
	if (nodes[node_idx].child == -1) {
		int d = depth;
		if (d > 15)
			d = 15;
		depth_counts[d]++;
		return;
	}
	int left = nodes[node_idx].child;
	int right = left + 1;
	count_depths(nodes, left, depth + 1, depth_counts);
	count_depths(nodes, right, depth + 1, depth_counts);
}

void esp_dos_build_tree(int count, const uint16_t *freq, uint8_t *lens)
{
	uint16_t sorted[1024];
	DosNode nodes[2048];
	uint16_t depth_counts[16];
	int first_used = 0;
	int non_zero, nleaf, total_nodes;
	int leaf_ptr, internal_ptr, next_create;
	uint16_t next_internal_freq, current_leaf_freq;
	uint16_t dx;
	int cx, d, si, i;
	int16_t s_dx;

	dos_radix_sort(freq, sorted, count);

	while (first_used < count && freq[sorted[first_used]] == 0)
		first_used++;
	non_zero = count - first_used;
	if (non_zero < 2) {
		first_used = count - 2;
		if (first_used < 0)
			first_used = 0;
	}
	nleaf = count - first_used;
	total_nodes = 2 * nleaf - 1;

	memset(nodes, 0, (size_t)total_nodes * sizeof(DosNode));

	leaf_ptr = first_used;
	internal_ptr = 0;
	next_create = 0;

	next_internal_freq = 0xFFFF;
	current_leaf_freq = (leaf_ptr < count) ? freq[sorted[leaf_ptr]] : 0xFFFF;

	while (next_create < total_nodes) {
		if (current_leaf_freq <= next_internal_freq) {
			nodes[next_create].freq = current_leaf_freq;
			nodes[next_create].child = -1;
			leaf_ptr++;
			current_leaf_freq = (leaf_ptr < count) ? freq[sorted[leaf_ptr]] : 0xFFFF;
			if (next_create - internal_ptr == 1)
				next_internal_freq = (uint16_t)(nodes[internal_ptr].freq + nodes[internal_ptr + 1].freq);
			next_create++;
		} else {
			nodes[next_create].freq = next_internal_freq;
			nodes[next_create].child = (int16_t)internal_ptr;
			internal_ptr += 2;
			if (internal_ptr < next_create)
				next_internal_freq = (uint16_t)(nodes[internal_ptr].freq + nodes[internal_ptr + 1].freq);
			else
				next_internal_freq = 0xFFFF;
			next_create++;
		}
	}

	memset(depth_counts, 0, sizeof(depth_counts));
	count_depths(nodes, total_nodes - 1, -1, depth_counts);

	dx = 0;
	cx = 15;
	for (i = 0; i <= 15; i++) {
		dx += (uint16_t)(depth_counts[i] << cx);
		cx--;
	}

	depth_counts[15] = (uint16_t)(depth_counts[15] - dx);
	s_dx = (int16_t)(dx - 1);
	while (s_dx >= 0) {
		si = 14;
		while (depth_counts[si] == 0 && si > 0)
			si--;
		depth_counts[si]--;
		depth_counts[si + 1] += 2;
		s_dx--;
	}

	memset(lens, 0, (size_t)count);
	si = count - 1;
	for (d = 0; d < 16; d++) {
		uint16_t num_syms = depth_counts[d];
		while (num_syms > 0) {
			uint16_t sym = sorted[si];
			lens[sym] = (uint8_t)(d + 1);
			si--;
			num_syms--;
		}
	}
}

void esp_huff_lengths_from_freq(const int *freq, int n, int *bits)
{
	uint16_t f16[ESP_MAX_HUFF];
	uint8_t l8[ESP_MAX_HUFF];
	int i;
	if (n <= 0)
		return;
	if (n > ESP_MAX_HUFF)
		n = ESP_MAX_HUFF;
	memset(f16, 0, sizeof(f16));
	memset(l8, 0, sizeof(l8));
	for (i = 0; i < n; i++)
		f16[i] = (freq[i] > 65535) ? 65535 : (uint16_t)freq[i];
	esp_dos_build_tree(n, f16, l8);
	for (i = 0; i < n; i++)
		bits[i] = (int)l8[i];
}

static int g_cnt[17];
static int g_sorted[ESP_MAX_HUFF];
static int g_ptr;
static uint32_t *g_codes;

static void codes_rec(int length, uint32_t path)
{
	g_cnt[length]--;
	if (g_cnt[length] >= 0) {
		int sym = g_sorted[g_ptr++];
		g_codes[sym] = path;
		return;
	}
	codes_rec(length + 1, path);
	codes_rec(length + 1, path | (1u << length));
}

void esp_huff_codes_from_lengths(const int *bits, int n, uint32_t *codes, int *clen)
{
	int first[17];
	int next[ESP_MAX_HUFF];
	int i, d;
	int ns = 0;

	for (i = 0; i < 17; i++) {
		first[i] = -1;
		g_cnt[i] = 0;
	}
	for (i = 0; i < n; i++) {
		codes[i] = 0;
		clen[i] = bits[i];
		g_cnt[bits[i]]++;
		next[i] = first[bits[i]];
		first[bits[i]] = i;
	}
	for (i = 1; i < 17; i++) {
		d = first[i];
		while (d >= 0) {
			g_sorted[ns++] = d;
			d = next[d];
		}
	}
	g_cnt[0] = 0;
	g_ptr = 0;
	g_codes = codes;
	if (ns)
		codes_rec(0, 0);
}

void esp_put_huff(EspBitWriter *bw, uint32_t code, int clen)
{
	if (clen > 0)
		esp_putbits(bw, code, clen);
}

static void write_mini_len(EspBitWriter *bw, int a)
{
	if (a < 7) {
		esp_putbits(bw, (uint32_t)a, 3);
		return;
	}
	esp_putbits(bw, 7, 3);
	a -= 7;
	while (a > 0) {
		esp_putbits(bw, 1, 1);
		a--;
	}
	esp_putbits(bw, 0, 1);
}

int esp_write_huff_table(EspBitWriter *bw, const int *bits, int size)
{
	int i, n, last;
	int minibits[ESP_MINI_SYMS];
	int mini_freq[ESP_MINI_SYMS];
	int seq[ESP_MAX_HUFF + 8];
	int extra[ESP_MAX_HUFF + 8];
	int nseq = 0;
	uint32_t mini_codes[ESP_MINI_SYMS];
	int mini_clen[ESP_MINI_SYMS];

	last = 0;
	for (i = 0; i < size; i++) {
		if (bits[i])
			last = i + 1;
	}
	n = last;
	if (n < 0)
		n = 0;

	i = 0;
	while (i < n) {
		if (bits[i] != 0) {
			seq[nseq] = bits[i] + 2;
			extra[nseq] = -1;
			nseq++;
			i++;
			continue;
		}
		{
			int z = 0;
			int j = i;
			while (j < n && bits[j] == 0) {
				z++;
				j++;
			}
			while (z > 0) {
				if (z >= 0x14) {
					int run = z;
					if (run > 0x14 + 511)
						run = 0x14 + 511;
					seq[nseq] = 2;
					extra[nseq] = run - 0x14;
					nseq++;
					z -= run;
				} else if (z >= 3 && z <= 18) {
					seq[nseq] = 1;
					extra[nseq] = z - 3;
					nseq++;
					z = 0;
				} else {
					seq[nseq] = 0;
					extra[nseq] = -1;
					nseq++;
					z--;
				}
			}
			i = j;
		}
	}

	memset(mini_freq, 0, sizeof(mini_freq));
	for (i = 0; i < nseq; i++)
		mini_freq[seq[i]]++;
	esp_huff_lengths_from_freq(mini_freq, ESP_MINI_SYMS, minibits);
	{
		int mlast = 0;
		for (i = 0; i < ESP_MINI_SYMS; i++) {
			if (minibits[i])
				mlast = i + 1;
		}
		if (mlast > 0x13)
			mlast = 0x13;
		esp_putbits(bw, (uint32_t)(0x13 - mlast), 5);
		for (i = 0; i < mlast; i++)
			write_mini_len(bw, minibits[i]);
	}
	esp_huff_codes_from_lengths(minibits, ESP_MINI_SYMS, mini_codes, mini_clen);
	esp_putbits(bw, (uint32_t)(size - n), 9);
	for (i = 0; i < nseq; i++) {
		esp_put_huff(bw, mini_codes[seq[i]], mini_clen[seq[i]]);
		if (seq[i] == 1)
			esp_putbits(bw, (uint32_t)extra[i], 4);
		else if (seq[i] == 2)
			esp_putbits(bw, (uint32_t)extra[i], 9);
	}
	return ESP_OK;
}
