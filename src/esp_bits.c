#include "esp_int.h"

#include <string.h>

void esp_br_init(EspBitReader *br, const uint8_t *data, size_t len)
{
	br->data = data;
	br->len = len;
	br->pos = 0;
	br->acc = 0;
	br->bits = 0;
}

static uint8_t esp_br_getbyte(EspBitReader *br)
{
	if (br->pos >= br->len)
		return 0;
	return br->data[br->pos++];
}

int esp_getbits(EspBitReader *br, int n)
{
	uint32_t v;
	while (n > br->bits) {
		br->acc |= ((uint32_t)esp_br_getbyte(br)) << br->bits;
		br->bits += 8;
	}
	v = br->acc & ((1u << n) - 1u);
	br->acc >>= n;
	br->bits -= n;
	return (int)v;
}

void esp_bw_init(EspBitWriter *bw, EspBuf *out)
{
	bw->out = out;
	bw->acc = 0;
	bw->bits = 0;
}

void esp_putbits(EspBitWriter *bw, uint32_t val, int n)
{
	bw->acc |= (val & ((1u << n) - 1u)) << bw->bits;
	bw->bits += n;
	while (bw->bits >= 8) {
		esp_buf_putc(bw->out, (uint8_t)(bw->acc & 0xFF));
		bw->acc >>= 8;
		bw->bits -= 8;
	}
}

void esp_bw_flush(EspBitWriter *bw)
{
	if (bw->bits > 0) {
		esp_buf_putc(bw->out, (uint8_t)(bw->acc & 0xFF));
		bw->acc = 0;
		bw->bits = 0;
	}
}

int esp_dist_slot(int dist, int *extra_bits, int *extra)
{
	int n;
	if (dist <= 2) {
		*extra_bits = 0;
		*extra = 0;
		return dist - 1;
	}
	n = 1;
	while ((1 << (n + 1)) < dist)
		n++;
	*extra_bits = n;
	*extra = (dist - 1) - (1 << n);
	return n + 1;
}

int esp_slot_to_dist(int slot, int extra)
{
	if (slot <= 1)
		return slot + 1;
	return ((1 << (slot - 1)) | extra) + 1;
}
