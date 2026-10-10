#ifndef ESP_H
#define ESP_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define ESP_MAGIC "ESP>"
#define ESP_VERSION_16 0x16
#define ESP_VERSION_17 0x17
#define ESP_DICT_SIZE 32768
#define ESP_LOOKAHEAD 268
#define ESP_MAX_MATCH 256
#define ESP_MIN_MATCH 3
#define ESP_LIT_SYMS 0x110
#define ESP_LEN_SYMS 0xFE
#define ESP_DIR_ENTRY 28
#define ESP_XOR_KEY 0x4B697947u
#define ESP_PATH_MEM 0x4000

#define ESP_METH_BEST 0
#define ESP_METH_FAST 3
#define ESP_METH_STORE 4

#define ESP_METHOD_MM   0x08
#define ESP_METHOD_LAST 0x20
#define ESP_METHOD_PASS 0x40
#define ESP_METHOD_CONT 0x80

#define ESP_ME_LEGACY 1
#define ESP_ME_NEW    2

#define ESP_OK 0
#define ESP_ERR_FORMAT -1
#define ESP_ERR_HUFF -2
#define ESP_ERR_IO -3
#define ESP_ERR_MEM -4
#define ESP_ERR_DATA -5
#define ESP_ERR_PASSWORD -8
#define ESP_ERR_PARAM -9
#define ESP_ERR_VOLUME -10

typedef struct {
	uint8_t *data;
	size_t len;
	size_t cap;
} EspBuf;

typedef struct {
	char name[13];
	char path[256];
	uint8_t attrib;
	uint32_t size;
	uint32_t repsize;
	uint32_t datetime;
	int is_dir;
} EspEntry;

typedef struct {
	uint8_t version;
	uint8_t method;
	uint32_t dir_pos;
	uint32_t xor_key;
	uint8_t *file;
	size_t file_len;
	EspEntry *entries;
	int n_entries;
	uint8_t *unpacked;
	size_t unpacked_len;
	int has_comment;
	uint8_t cmt_type;
	uint32_t cmt_pos;
	EspBuf comment;
} EspArchive;

#define ESP_CMT_TEXT    1
#define ESP_CMT_ANSI    2
#define ESP_CMT_PIC_OLD 4
#define ESP_CMT_PIC_VGA 7
#define ESP_CMT_PIC_HR  6

int esp_buf_init(EspBuf *b, size_t cap);
int esp_buf_append(EspBuf *b, const void *p, size_t n);
int esp_buf_putc(EspBuf *b, uint8_t c);
void esp_buf_free(EspBuf *b);

uint32_t esp_hash_password_me1(const char *pass);
uint32_t esp_hash_password_me2(const char *pass);
void esp_xor_key(uint8_t *data, size_t len, uint32_t key);

int esp_lzss_decode(const uint8_t *in, size_t in_len, EspBuf *out);
int esp_lzss_decode_key(const uint8_t *in, size_t in_len, EspBuf *out, uint32_t key);
int esp_lzss_encode(const uint8_t *in, size_t in_len, EspBuf *out, int method);
int esp_lzss_encode_key(const uint8_t *in, size_t in_len, EspBuf *out, int method, uint32_t key);

int esp_open_archive(const uint8_t *data, size_t len, EspArchive *arc);
int esp_open_archive_password(const uint8_t *data, size_t len, const char *password, EspArchive *arc);
void esp_close_archive(EspArchive *arc);
int esp_extract_entry(const EspArchive *arc, int index, EspBuf *out);

int esp_read_file(const char *path, EspBuf *out);
int esp_write_file(const char *path, const uint8_t *data, size_t n);

typedef struct {
	const char *src_path;
	char stored_name[13];
	char stored_path[256];
	uint8_t attrib;
	uint32_t size;
	uint32_t datetime;
	int is_dir;
	uint32_t repsize;
	uint8_t *data;
	int orig_idx;
} EspInputFile;

#define ESP_VERSION_19 0x19

#define ESP_MM_NONE 0
#define ESP_MM_8BIT 1
#define ESP_MM_24BIT 2

void esp_filter_mm_encode(uint8_t *data, size_t len);
void esp_filter_mm_decode(uint8_t *data, size_t len);
int esp_filter_mm2_encode(uint8_t *data, size_t len);
int esp_filter_mm2_decode(uint8_t *data, size_t len);

int esp_create_archive(EspInputFile *files, int n_files, int method,
		       int enable_rep, int mm_mode, int me_version,
		       const char *password, int cmt_type,
		       const uint8_t *cmt_data, size_t cmt_len,
		       EspBuf *out);

int esp_parse_volume_size(const char *arg, uint32_t *out_size);
void esp_format_volume_path(const char *base_path, int vol_num, char *out_path, size_t out_sz);
int esp_write_archive_volumes(const char *base_path, const EspBuf *arc_buf, uint32_t vol_size);
int esp_read_archive_volumes(const char *first_path, EspBuf *out_buf);

uint32_t esp_unix_to_dos_datetime(uint32_t unix_ts);
void esp_dos_datetime_to_parts(uint32_t dt, int *year, int *mon, int *day,
			       int *hour, int *min, int *sec);

#endif

