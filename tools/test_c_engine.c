#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define HASH_SIZE 4096
#define MAX_MATCH 256

typedef struct {
    uint32_t offset;
    uint16_t kind; // 0=LIT, 1=MATCH
    uint16_t val;  // lit val or dist
    uint16_t len;
} SavedToken;

static inline uint16_t update_hash(uint16_t h, uint8_t byte)
{
    uint16_t bx = (uint16_t)(h << 4);
    uint8_t bl = (uint8_t)((bx & 0xFF) + byte);
    uint8_t bh = (uint8_t)((bx >> 8) & 0x0F);
    return (uint16_t)((bh << 8) | bl);
}

static inline int is_valid_cand(uint16_t bp, uint16_t cand)
{
    uint16_t diff = (uint16_t)(bp - cand);
    return (diff < 0x8000) && (cand != 0x8100);
}

static inline uint16_t get_u16(const uint8_t *buf, size_t n, size_t pos)
{
    if (pos + 1 < n)
        return (uint16_t)(buf[pos] | (buf[pos + 1] << 8));
    if (pos < n)
        return (uint16_t)buf[pos];
    return 0;
}

static int compare_strings(const uint8_t *buf, size_t n, size_t pos1, size_t pos2, int max_l)
{
    int l = 0;
    while (l < max_l && pos1 + l < n && pos2 + l < n && buf[pos1 + l] == buf[pos2 + l])
        l++;
    return l;
}

typedef struct {
    uint16_t head[HASH_SIZE];
    uint16_t prev[65536];
    const uint8_t *buf;
    size_t n;
    int max_chain;

    uint16_t best_pos;
    int best_len;
} Engine;

static void suffix_chain(Engine *e, uint16_t bp, size_t cur_pos, int target_len, uint16_t bx, int ax, int chain)
{
    int si = ax - target_len;
    uint16_t cur_w = get_u16(e->buf, e->n, cur_pos);

    while (chain > 0 && is_valid_cand(bp, bx)) {
        chain--;
        int cand_prefix = (int)bx + si;
        if (cand_prefix >= 0 && is_valid_cand(bp, (uint16_t)cand_prefix)) {
            uint16_t cand_w = get_u16(e->buf, e->n, (size_t)cand_prefix);
            if (cand_w == cur_w) {
                int ml = compare_strings(e->buf, e->n, cur_pos, (size_t)cand_prefix, MAX_MATCH);
                if (ml > e->best_len) {
                    e->best_len = ml;
                    e->best_pos = (uint16_t)cand_prefix;

                    // Hop check at 0x7E0:
                    size_t t1 = cur_pos + ml - 2;
                    uint16_t cx = (t1 < e->n) ? e->prev[(uint16_t)t1] : 0x8100;
                    if (!is_valid_cand(bp, cx))
                        return; // 0x7EF: js 0x854 -> ret
                    if ((int16_t)(cx - bx) < 0) {
                        // 0x82C: ax = 2
                        bx = cx;
                        si = -(ml - 2);
                        continue;
                    }

                    // 0x7F5:
                    int t2 = cand_prefix + ml - 3;
                    uint16_t cx2 = (t2 >= 0 && t2 < (int)e->n) ? e->prev[(uint16_t)t2] : 0x8100;
                    if (!is_valid_cand(bp, cx2))
                        return;
                    uint16_t nxt = e->prev[bx];
                    if (!is_valid_cand(bp, nxt))
                        return;
                    if ((int16_t)(cx2 - nxt) < 0) {
                        // 0x840: ax = 3
                        bx = cx2;
                        si = -(ml - 3);
                        continue;
                    }

                    // 0x818:
                    bx = nxt;
                    continue;
                }
            }
        }
        bx = e->prev[bx];
    }
}

static int hop_check(Engine *e, uint16_t bp, size_t cur_pos, int chain)
{
    int L = e->best_len;
    uint16_t cand = e->best_pos;
    size_t target = cur_pos + L - 2;
    uint16_t cx = (target < e->n) ? e->prev[(uint16_t)target] : 0x8100;

    if (!is_valid_cand(bp, cx))
        return 1; // 0x6fc: return

    int16_t diff = (int16_t)(cx - cand);
    if (diff < 0) {
        suffix_chain(e, bp, cur_pos, L, cx, 2, chain);
        return 1;
    }

    size_t t2 = (size_t)cand + L - 3;
    uint16_t cx2 = (t2 < e->n) ? e->prev[(uint16_t)t2] : 0x8100;
    if (!is_valid_cand(bp, cx2))
        return 1;

    uint16_t nxt = e->prev[cand];
    if (!is_valid_cand(bp, nxt))
        return 1;

    int16_t diff2 = (int16_t)(cx2 - nxt);
    if (diff2 < 0) {
        suffix_chain(e, bp, cur_pos, L, cx2, 3, chain);
        return 1;
    }

    return 0; // continue main chain at 0x668
}

static void find_match_64c(Engine *e, uint16_t bp, size_t cur_pos)
{
    e->best_len = 1;
    e->best_pos = 0;
    uint16_t cand = e->prev[(uint16_t)cur_pos];
    int chain = e->max_chain;
    uint16_t cur_w = get_u16(e->buf, e->n, cur_pos);

    while (chain > 0 && is_valid_cand(bp, cand)) {
        chain--;
        if (e->best_len <= 1) {
            uint16_t cand_w = get_u16(e->buf, e->n, cand);
            if (cand_w == cur_w) {
                int ml = compare_strings(e->buf, e->n, cur_pos, cand, MAX_MATCH);
                if (ml > e->best_len) {
                    e->best_len = ml;
                    e->best_pos = cand;
                    if (hop_check(e, bp, cur_pos, chain))
                        return;
                }
            }
        } else {
            uint16_t cur_end = get_u16(e->buf, e->n, cur_pos + e->best_len - 1);
            uint16_t cand_end = get_u16(e->buf, e->n, cand + e->best_len - 1);
            if (cand_end == cur_end) {
                int ml = compare_strings(e->buf, e->n, cur_pos, cand, MAX_MATCH);
                if (ml > e->best_len) {
                    e->best_len = ml;
                    e->best_pos = cand;
                    if (hop_check(e, bp, cur_pos, chain))
                        return;
                }
            }
        }
        cand = e->prev[cand];
    }
}

static void find_match_5f4(Engine *e, uint16_t bp, size_t cur_pos, int prev_len)
{
    if (prev_len < 3) {
        find_match_64c(e, bp, cur_pos);
        return;
    }

    size_t si = (cur_pos - 2) + prev_len;
    uint16_t cand0 = (si < e->n) ? e->prev[(uint16_t)si] : 0x8100;
    if (!is_valid_cand(bp, cand0)) {
        e->best_len = 1; e->best_pos = 0; return;
    }
    uint16_t cand1 = (si - 1 < e->n) ? e->prev[(uint16_t)(si - 1)] : 0x8100;
    if (!is_valid_cand(bp, cand1)) {
        e->best_len = 1; e->best_pos = 0; return;
    }

    int16_t dist0 = (int16_t)(bp - cand0);
    int16_t dist1 = (int16_t)(bp - cand1);

    int ax = -(prev_len - 2);
    int cx = ax;
    int16_t max_dist = dist0;
    uint16_t best_cand = cand0;

    if (dist1 >= dist0) {
        max_dist = dist1;
        best_cand = cand1;
        cx++;
    }

    si -= 2;
    ax += 2;
    while (ax <= 0) {
        uint16_t p = (si < e->n) ? e->prev[(uint16_t)si] : 0x8100;
        int16_t dist = (int16_t)(bp - p);
        if (dist >= max_dist) {
            max_dist = dist;
            best_cand = p;
            cx = ax;
        }
        si -= 1;
        ax += 1;
    }

    e->best_len = prev_len;
    e->best_pos = 0;

    if (cx == 0) {
        uint16_t cand = best_cand;
        int chain = e->max_chain;
        while (chain > 0 && is_valid_cand(bp, cand)) {
            chain--;
            uint16_t cur_end = get_u16(e->buf, e->n, cur_pos + e->best_len - 1);
            uint16_t cand_end = get_u16(e->buf, e->n, cand + e->best_len - 1);
            if (cand_end == cur_end) {
                int ml = compare_strings(e->buf, e->n, cur_pos, cand, MAX_MATCH);
                if (ml > e->best_len) {
                    e->best_len = ml;
                    e->best_pos = cand;
                    if (hop_check(e, bp, cur_pos, chain))
                        return;
                }
            }
            cand = e->prev[cand];
        }
    } else {
        suffix_chain(e, bp, cur_pos, prev_len, best_cand, cx + prev_len, e->max_chain);
    }
}

int main(int argc, char **argv)
{
    const char *path = (argc > 1) ? argv[1] : "corpus/eng.txt";
    FILE *f = fopen(path, "rb");
    if (!f) { perror("fopen"); return 1; }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    size_t full_size = 256 + fsize;
    uint8_t *full_buf = (uint8_t *)calloc(1, full_size);
    fread(full_buf + 256, 1, fsize, f);
    fclose(f);

    FILE *fout = fopen("/tmp/c_tokens.bin", "wb");

    Engine eng;
    memset(&eng, 0, sizeof(eng));
    for (int i = 0; i < HASH_SIZE; i++) eng.head[i] = 0x8100;
    for (int i = 0; i < 65536; i++) eng.prev[i] = 0x8100;
    eng.prev[0xFF] = 0x80FF;
    eng.buf = full_buf;
    eng.n = full_size;
    eng.max_chain = 256;

    uint16_t h = 0;
    uint16_t bp = 0x00FD;

    for (int i = 0; i < 0x103; i++) {
        bp++;
        uint8_t b = (bp + 2 < full_size) ? full_buf[bp + 2] : 0;
        h = update_hash(h, b);
        uint16_t old_h = eng.head[h];
        eng.head[h] = bp;
        eng.prev[bp] = (bp >= old_h) ? old_h : (uint16_t)(bp - 0x8000);
    }

    size_t cur_pos = 0x100;
    find_match_64c(&eng, bp, cur_pos);
    int curr_len = eng.best_len;
    uint16_t curr_pos_cand = eng.best_pos;
    bp++;

    size_t token_count = 0;
    size_t uncomp_offset = 0;

    while (cur_pos < full_size) {
        uint8_t b = (bp + 2 < full_size) ? full_buf[bp + 2] : 0;
        h = update_hash(h, b);
        uint16_t old_h = eng.head[h];
        eng.head[h] = bp;
        eng.prev[bp] = (bp >= old_h) ? old_h : (uint16_t)(bp - 0x8000);

        int prev_match_len = curr_len;
        uint16_t prev_match_pos = curr_pos_cand;

        find_match_5f4(&eng, bp, cur_pos + 1, prev_match_len);
        curr_len = eng.best_len;
        curr_pos_cand = eng.best_pos;

        if (prev_match_len < 3) {
            SavedToken tok = { (uint32_t)uncomp_offset, 0, full_buf[cur_pos], 0 };
            fwrite(&tok, sizeof(tok), 1, fout);
            uncomp_offset++;
            cur_pos++;
            bp++;
            token_count++;
        } else if (prev_match_len >= curr_len) {
            int dist = (int)(cur_pos - prev_match_pos);
            SavedToken tok = { (uint32_t)uncomp_offset, 1, (uint16_t)dist, (uint16_t)prev_match_len };
            fwrite(&tok, sizeof(tok), 1, fout);
            uncomp_offset += prev_match_len;

            for (int k = 1; k < prev_match_len; k++) {
                bp++;
                b = (bp + 2 < full_size) ? full_buf[bp + 2] : 0;
                h = update_hash(h, b);
                old_h = eng.head[h];
                eng.head[h] = bp;
                eng.prev[bp] = (bp >= old_h) ? old_h : (uint16_t)(bp - 0x8000);
            }
            cur_pos += prev_match_len;
            find_match_64c(&eng, bp, cur_pos);
            curr_len = eng.best_len;
            curr_pos_cand = eng.best_pos;
            bp++;
            token_count++;
        } else {
            SavedToken tok = { (uint32_t)uncomp_offset, 0, full_buf[cur_pos], 0 };
            fwrite(&tok, sizeof(tok), 1, fout);
            uncomp_offset++;
            cur_pos++;
            bp++;
            token_count++;
        }
    }

    fclose(fout);
    printf("Wrote %zu tokens to /tmp/c_tokens.bin\n", token_count);
    free(full_buf);
    return 0;
}
