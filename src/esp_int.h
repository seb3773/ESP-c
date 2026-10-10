#ifndef ESP_INT_H
#define ESP_INT_H

#include "esp.h"

#define ESP_MAX_HUFF ESP_LIT_SYMS
#define ESP_MINI_SYMS 0x13
#define ESP_MAX_BITS 16
#define ESP_HASH_SIZE 65536
#define ESP_MAX_CHAIN_BEST 4096
#define ESP_MAX_CHAIN_FAST 1
#define ESP_BLOCK_TOKENS 12288

typedef struct {
	const uint8_t *data;
	size_t len;
	size_t pos;
	uint32_t acc;
	int bits;
} EspBitReader;

typedef struct {
	EspBuf *out;
	uint32_t acc;
	int bits;
} EspBitWriter;

typedef struct {
	uint16_t stat[256];
	int16_t dyn[ESP_MAX_HUFF * 2];
} EspHuffTree;

typedef struct {
	int kind;
	int lit;
	int dist;
	int len;
} EspToken;

enum { ESP_TOK_LIT = 0, ESP_TOK_MATCH = 1, ESP_TOK_END = 2, ESP_TOK_NEWTABLE = 3 };

void esp_br_init(EspBitReader *br, const uint8_t *data, size_t len);
int esp_getbits(EspBitReader *br, int n);

void esp_bw_init(EspBitWriter *bw, EspBuf *out);
void esp_putbits(EspBitWriter *bw, uint32_t val, int n);
void esp_bw_flush(EspBitWriter *bw);

int esp_huff_from_lengths(const int *bits, int n, EspHuffTree *tree);
int esp_get_huff(EspBitReader *br, const EspHuffTree *tree);
int esp_read_huff_table(EspBitReader *br, EspHuffTree *tree, int size);

void esp_dos_build_tree(int count, const uint16_t *freq, uint8_t *lens);
void esp_huff_lengths_from_freq(const int *freq, int n, int *bits);
int esp_write_huff_table(EspBitWriter *bw, const int *bits, int size);
void esp_huff_codes_from_lengths(const int *bits, int n, uint32_t *codes, int *clen);
void esp_put_huff(EspBitWriter *bw, uint32_t code, int clen);

void esp_xor_gyik(uint8_t *data, size_t len);

int esp_dist_slot(int dist, int *extra_bits, int *extra);
int esp_slot_to_dist(int slot, int extra);

#endif
