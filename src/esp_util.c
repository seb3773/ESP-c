#include "esp.h"
#include "esp_int.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int esp_buf_init(EspBuf *b, size_t cap)
{
	b->len = 0;
	b->cap = cap ? cap : 64;
	b->data = (uint8_t *)malloc(b->cap);
	return b->data ? ESP_OK : ESP_ERR_MEM;
}

static int esp_buf_grow(EspBuf *b, size_t need)
{
	size_t cap;
	uint8_t *p;
	if (need <= b->cap)
		return ESP_OK;
	cap = b->cap ? b->cap : 64;
	while (cap < need) {
		if (cap > (size_t)-1 / 2)
			return ESP_ERR_MEM;
		cap *= 2;
	}
	p = (uint8_t *)realloc(b->data, cap);
	if (!p)
		return ESP_ERR_MEM;
	b->data = p;
	b->cap = cap;
	return ESP_OK;
}

int esp_buf_append(EspBuf *b, const void *p, size_t n)
{
	if (esp_buf_grow(b, b->len + n) != ESP_OK)
		return ESP_ERR_MEM;
	if (n)
		memcpy(b->data + b->len, p, n);
	b->len += n;
	return ESP_OK;
}

int esp_buf_putc(EspBuf *b, uint8_t c)
{
	if (esp_buf_grow(b, b->len + 1) != ESP_OK)
		return ESP_ERR_MEM;
	b->data[b->len++] = c;
	return ESP_OK;
}

void esp_buf_free(EspBuf *b)
{
	free(b->data);
	b->data = NULL;
	b->len = b->cap = 0;
}

int esp_read_file(const char *path, EspBuf *out)
{
	FILE *f;
	uint8_t buf[8192];
	size_t n;
	int rc;
	if (esp_buf_init(out, 0) != ESP_OK)
		return ESP_ERR_MEM;
	f = fopen(path, "rb");
	if (!f) {
		esp_buf_free(out);
		return ESP_ERR_IO;
	}
	while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
		rc = esp_buf_append(out, buf, n);
		if (rc != ESP_OK) {
			fclose(f);
			esp_buf_free(out);
			return rc;
		}
	}
	if (ferror(f)) {
		fclose(f);
		esp_buf_free(out);
		return ESP_ERR_IO;
	}
	fclose(f);
	return ESP_OK;
}

int esp_write_file(const char *path, const uint8_t *data, size_t n)
{
	FILE *f = fopen(path, "wb");
	if (!f)
		return ESP_ERR_IO;
	if (n && fwrite(data, 1, n, f) != n) {
		fclose(f);
		return ESP_ERR_IO;
	}
	fclose(f);
	return ESP_OK;
}

uint32_t esp_hash_password_me1(const char *pass)
{
	uint32_t ebx = 0;
	const uint8_t *p = (const uint8_t *)pass;
	if (!pass)
		return ESP_XOR_KEY;
	while (*p) {
		uint8_t al = *p++;
		uint32_t cf = (ebx >> 25) & 1;
		uint32_t bl, res;
		uint16_t bx;
		uint32_t old_cf;

		/* rol ebx, 7 */
		ebx = ((ebx << 7) | (ebx >> 25));

		/* adc bl, al */
		bl = ebx & 0xFF;
		res = bl + al + cf;
		cf = (res > 0xFF) ? 1 : 0;
		ebx = (ebx & 0xFFFFFF00) | (res & 0xFF);

		/* rcr bx, 1 (through carry) */
		bx = (uint16_t)(ebx & 0xFFFF);
		old_cf = cf;
		bx = (uint16_t)((bx >> 1) | (old_cf << 15));
		ebx = (ebx & 0xFFFF0000) | bx;
	}
	ebx = (uint32_t)(-(int32_t)ebx);
	if (ebx == 0)
		ebx = ESP_XOR_KEY;
	return ebx;
}

uint32_t esp_hash_password_me2(const char *pass)
{
	uint32_t ebx = 0;
	const uint8_t *p = (const uint8_t *)pass;
	if (!pass)
		return ESP_XOR_KEY;
	while (*p) {
		uint8_t al = *p++;
		uint32_t cf = (ebx >> 6) & 1;
		uint32_t bl, bh, res1, res2;
		uint16_t bx;

		/* ror ebx, 7 */
		ebx = ((ebx >> 7) | (ebx << 25));

		/* adc bl, al */
		bl = ebx & 0xFF;
		res1 = bl + al + cf;
		cf = (res1 > 0xFF) ? 1 : 0;
		ebx = (ebx & 0xFFFFFF00) | (res1 & 0xFF);

		/* adc bh, bl */
		bh = (ebx >> 8) & 0xFF;
		bl = ebx & 0xFF;
		res2 = bh + bl + cf;
		cf = (res2 > 0xFF) ? 1 : 0;
		ebx = (ebx & 0xFFFF00FF) | ((res2 & 0xFF) << 8);

		/* rol bx, 1 (without carry, 16-bit) */
		bx = (uint16_t)(ebx & 0xFFFF);
		bx = (uint16_t)((bx << 1) | (bx >> 15));
		ebx = (ebx & 0xFFFF0000) | bx;
	}
	ebx = (uint32_t)(-(int32_t)ebx);
	if (ebx == 0)
		ebx = ESP_XOR_KEY;
	return ebx;
}

void esp_xor_key(uint8_t *data, size_t len, uint32_t key)
{
	uint8_t k[4];
	size_t i;
	k[0] = (uint8_t)(key);
	k[1] = (uint8_t)(key >> 8);
	k[2] = (uint8_t)(key >> 16);
	k[3] = (uint8_t)(key >> 24);
	for (i = 0; i < len; i++)
		data[i] ^= k[i & 3];
}

void esp_xor_gyik(uint8_t *data, size_t len)
{
	esp_xor_key(data, len, ESP_XOR_KEY);
}

uint32_t esp_unix_to_dos_datetime(uint32_t unix_ts)
{
	time_t t = (time_t)unix_ts;
	struct tm tm;
	uint32_t date, timev;
	if (!localtime_r(&t, &tm))
		return 0;
	date = ((tm.tm_year + 1900 - 1980) << 9) | ((tm.tm_mon + 1) << 5) |
	       tm.tm_mday;
	timev = (tm.tm_hour << 11) | (tm.tm_min << 5) | (tm.tm_sec / 2);
	return (date << 16) | timev;
}

void esp_dos_datetime_to_parts(uint32_t dt, int *year, int *mon, int *day,
			       int *hour, int *min, int *sec)
{
	uint32_t timev = dt & 0xFFFFu;
	uint32_t date = dt >> 16;
	*sec = 2 * (timev & 31);
	*min = (timev >> 5) & 63;
	*hour = (timev >> 11) & 31;
	*day = date & 31;
	*mon = (date >> 5) & 15;
	*year = 1980 + (date >> 9);
}

int esp_parse_volume_size(const char *arg, uint32_t *out_size)
{
	if (!arg || !*arg || !out_size)
		return ESP_ERR_PARAM;

	if (arg[0] == 'v' || arg[0] == 'V') {
		arg++;
		if (strcmp(arg, "1440") == 0) {
			*out_size = 1456400;
			return ESP_OK;
		} else if (strcmp(arg, "1200") == 0) {
			*out_size = 1212400;
			return ESP_OK;
		} else if (strcmp(arg, "720") == 0) {
			*out_size = 729400;
			return ESP_OK;
		} else if (strcmp(arg, "360") == 0) {
			*out_size = 361400;
			return ESP_OK;
		} else {
			return ESP_ERR_PARAM;
		}
	}

	char *end = NULL;
	unsigned long val = strtoul(arg, &end, 10);
	if (val == 0 || end == arg)
		return ESP_ERR_PARAM;

	if (*end == 'k') {
		val *= 1000;
	} else if (*end == 'K') {
		val *= 1024;
	} else if (*end == 'm') {
		val *= 1000000;
	} else if (*end == 'M') {
		val *= (1024 * 1024);
	} else if (*end == 'b' || *end == 'B') {
		/* raw bytes */
	} else if (*end == '\0') {
		if (val < 4096)
			val *= 1024;
	} else {
		return ESP_ERR_PARAM;
	}

	*out_size = (uint32_t)val;
	return ESP_OK;
}

void esp_format_volume_path(const char *base_path, int vol_num, char *out_path, size_t out_sz)
{
	if (vol_num <= 1) {
		snprintf(out_path, out_sz, "%s", base_path);
		return;
	}
	int vol_idx = vol_num - 1; /* 1 -> E01, 2 -> E02, ... */

	const char *slash = strrchr(base_path, '/');
	const char *fname = slash ? slash + 1 : base_path;
	const char *dot = strrchr(fname, '.');

	char prefix[1024];
	char first_c = 'e';

	if (dot) {
		size_t stem_len = (size_t)(dot - base_path);
		if (stem_len >= sizeof(prefix))
			stem_len = sizeof(prefix) - 1;
		memcpy(prefix, base_path, stem_len);
		prefix[stem_len] = '\0';
		if (dot[1])
			first_c = dot[1];
	} else {
		snprintf(prefix, sizeof(prefix), "%s", base_path);
		first_c = 'e';
	}

	char base_letter = first_c;
	int num_part = vol_idx;

	if (vol_idx > 99) {
		int add = (vol_idx - 100) / 100 + 1;
		base_letter = (char)(base_letter + add);
		num_part = (vol_idx - 100) % 100;
	}

	snprintf(out_path, out_sz, "%s.%c%02d", prefix, base_letter, num_part);
}

int esp_is_safe_relpath(const char *path)
{
	if (!path || !path[0])
		return 0;

	/* Reject absolute paths: leading slash or backslash */
	if (path[0] == '/' || path[0] == '\\')
		return 0;

	/* Reject DOS drive identifiers: e.g. "C:", "d:" */
	if (((path[0] >= 'a' && path[0] <= 'z') || (path[0] >= 'A' && path[0] <= 'Z')) && path[1] == ':')
		return 0;

	int depth = 0;
	const char *p = path;

	while (*p) {
		/* Skip consecutive slashes/backslashes */
		while (*p == '/' || *p == '\\')
			p++;
		if (!*p)
			break;

		/* Find the end of this component */
		const char *start = p;
		while (*p && *p != '/' && *p != '\\')
			p++;
		size_t len = (size_t)(p - start);

		if (len == 1 && start[0] == '.') {
			/* Current directory - depth unchanged */
			continue;
		} else if (len == 2 && start[0] == '.' && start[1] == '.') {
			/* Parent directory - would ascend */
			if (depth <= 0)
				return 0; /* Unsafe: ascends above destination root */
			depth--;
		} else {
			/* Normal directory or file component */
			depth++;
		}
	}

	return depth > 0;
}

