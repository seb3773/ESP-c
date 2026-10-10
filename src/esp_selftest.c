#include "esp.h"
#include "esp_int.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int fails;

static void expect(int cond, const char *msg)
{
	if (!cond) {
		printf("FAIL %s\n", msg);
		fails++;
	} else {
		printf("OK %s\n", msg);
	}
}

static int roundtrip(const uint8_t *in, size_t n, int method, const char *tag)
{
	EspBuf enc, dec;
	int rc;
	rc = esp_lzss_encode(in, n, &enc, method);
	if (rc != ESP_OK) {
		expect(0, tag);
		return -1;
	}
	rc = esp_lzss_decode(enc.data, enc.len, &dec);
	expect(rc == ESP_OK && dec.len == n && (n == 0 || memcmp(dec.data, in, n) == 0), tag);
	esp_buf_free(&enc);
	esp_buf_free(&dec);
	return 0;
}

int main(void)
{
	const char *hello = "Hello ESP world!\nThis is a test of the ESP compression format.\n";
	uint8_t de[28];
	uint8_t *zeros;
	EspInputFile files[2];
	EspBuf arc, a, b;
	EspArchive opened;
	static const uint8_t fa[] = "HEADER----aaaa";
	static const uint8_t fb[] = "HEADER----bbbb";

	roundtrip((const uint8_t *)hello, strlen(hello), 0, "lzss-hello-m0");
	roundtrip((const uint8_t *)hello, strlen(hello), 3, "lzss-hello-m3");
	roundtrip((const uint8_t *)hello, 0, 0, "lzss-empty");

	memset(de, 0, sizeof de);
	de[4] = 0xff;
	de[5] = 0xff;
	memcpy(de + 6, "HELLO.TXT", 9);
	de[19] = 0x20;
	de[24] = 63;
	roundtrip(de, 28, 0, "lzss-dirent");

	zeros = (uint8_t *)calloc(200000, 1);
	if (!zeros)
		return 1;
	roundtrip(zeros, 200000, 0, "lzss-zeros");
	free(zeros);

	memset(files, 0, sizeof files);
	files[0].stored_name[0] = 'A';
	memcpy(files[0].stored_name, "A.TXT", 5);
	files[0].attrib = 0x20;
	files[0].size = sizeof fa - 1;
	files[0].data = (uint8_t *)fa;
	files[1].stored_name[0] = 'B';
	memcpy(files[1].stored_name, "B.TXT", 5);
	files[1].attrib = 0x20;
	files[1].size = sizeof fb - 1;
	files[1].data = (uint8_t *)fb;
	expect(esp_create_archive(files, 2, 0, 1, ESP_MM_NONE, ESP_ME_NEW, NULL, 0, NULL, 0, &arc) == ESP_OK, "create-rep");
	expect(esp_open_archive(arc.data, arc.len, &opened) == ESP_OK, "open-rep");
	expect(opened.n_entries == 2 && opened.entries[1].repsize == 10, "repsize-10");
	expect(esp_extract_entry(&opened, 0, &a) == ESP_OK &&
		   a.len == sizeof fa - 1 && memcmp(a.data, fa, a.len) == 0,
	       "extract-a");
	expect(esp_extract_entry(&opened, 1, &b) == ESP_OK &&
		   b.len == sizeof fb - 1 && memcmp(b.data, fb, b.len) == 0,
	       "extract-b");
	esp_buf_free(&a);
	esp_buf_free(&b);
	esp_close_archive(&opened);
	esp_buf_free(&arc);

	/* MM (8-bit delta) and MM2 (24-bit planar) tests */
	{
		uint8_t mm_test[256];
		uint8_t mm_copy[256];
		int i, tlen;
		for (i = 0; i < 256; i++) {
			mm_test[i] = (uint8_t)((i * 37 + 13) & 0xFF);
		}
		memcpy(mm_copy, mm_test, 256);
		esp_filter_mm_encode(mm_copy, 256);
		esp_filter_mm_decode(mm_copy, 256);
		expect(memcmp(mm_test, mm_copy, 256) == 0, "filter-mm-roundtrip");

		for (tlen = 1; tlen <= 20; tlen++) {
			memcpy(mm_copy, mm_test, (size_t)tlen);
			expect(esp_filter_mm2_encode(mm_copy, (size_t)tlen) == ESP_OK, "filter-mm2-enc");
			expect(esp_filter_mm2_decode(mm_copy, (size_t)tlen) == ESP_OK, "filter-mm2-dec");
			expect(memcmp(mm_test, mm_copy, (size_t)tlen) == 0, "filter-mm2-roundtrip");
		}

		/* Archive roundtrip with /MM */
		expect(esp_create_archive(files, 2, 0, 1, ESP_MM_8BIT, ESP_ME_NEW, NULL, 0, NULL, 0, &arc) == ESP_OK, "create-mm");
		expect(esp_open_archive(arc.data, arc.len, &opened) == ESP_OK, "open-mm");
		expect(esp_extract_entry(&opened, 0, &a) == ESP_OK &&
			   a.len == sizeof fa - 1 && memcmp(a.data, fa, a.len) == 0,
		       "extract-mm-a");
		expect(esp_extract_entry(&opened, 1, &b) == ESP_OK &&
			   b.len == sizeof fb - 1 && memcmp(b.data, fb, b.len) == 0,
		       "extract-mm-b");
		esp_buf_free(&a);
		esp_buf_free(&b);
		esp_close_archive(&opened);
		esp_buf_free(&arc);

		/* Archive roundtrip with /MM2 */
		expect(esp_create_archive(files, 2, 0, 1, ESP_MM_24BIT, ESP_ME_NEW, NULL, 0, NULL, 0, &arc) == ESP_OK, "create-mm2");
		expect(esp_open_archive(arc.data, arc.len, &opened) == ESP_OK, "open-mm2");
		expect(opened.version == ESP_VERSION_19, "version-0x19");
		expect(esp_extract_entry(&opened, 0, &a) == ESP_OK &&
			   a.len == sizeof fa - 1 && memcmp(a.data, fa, a.len) == 0,
		       "extract-mm2-a");
		expect(esp_extract_entry(&opened, 1, &b) == ESP_OK &&
			   b.len == sizeof fb - 1 && memcmp(b.data, fb, b.len) == 0,
		       "extract-mm2-b");
		esp_buf_free(&a);
		esp_buf_free(&b);
		esp_close_archive(&opened);
		esp_buf_free(&arc);
	}

	/* Password Encryption (/ME1 and /ME2) tests */
	{
		uint32_t me1_abcd = esp_hash_password_me1("ABCD");
		uint32_t me1_abce = esp_hash_password_me1("ABCE");
		uint32_t me1_abcc = esp_hash_password_me1("ABCC");
		uint32_t me2_abcd = esp_hash_password_me2("ABCD");
		uint32_t me2_abce = esp_hash_password_me2("ABCE");

		/* Verify historical GyikSoft behavior documented in WHATSNEW.TXT */
		expect(me1_abcd == me1_abce, "me1-alias-bug");
		expect(me1_abcd != me1_abcc, "me1-abcc-diff");
		expect(me2_abcd != me2_abce, "me2-fixed");

		/* ME2 and MM2 conflict validation */
		expect(esp_create_archive(files, 2, 0, 1, ESP_MM_24BIT, ESP_ME_NEW, "secret", 0, NULL, 0, &arc) == ESP_ERR_PARAM,
		       "me2-mm2-conflict");

		/* Encrypted archive with /ME1 */
		expect(esp_create_archive(files, 2, 0, 1, ESP_MM_NONE, ESP_ME_LEGACY, "secret", 0, NULL, 0, &arc) == ESP_OK,
		       "create-me1");
		expect(esp_open_archive_password(arc.data, arc.len, NULL, &opened) == ESP_ERR_PASSWORD,
		       "open-me1-no-pass");
		expect(esp_open_archive_password(arc.data, arc.len, "wrong", &opened) == ESP_ERR_PASSWORD,
		       "open-me1-wrong-pass");
		expect(esp_open_archive_password(arc.data, arc.len, "secret", &opened) == ESP_OK,
		       "open-me1-ok");
		expect(opened.version == ESP_VERSION_16, "me1-version-0x16");
		expect((opened.method & ESP_METHOD_PASS) != 0, "me1-method-pass-flag");
		expect(esp_extract_entry(&opened, 0, &a) == ESP_OK &&
			   a.len == sizeof fa - 1 && memcmp(a.data, fa, a.len) == 0,
		       "extract-me1-a");
		expect(esp_extract_entry(&opened, 1, &b) == ESP_OK &&
			   b.len == sizeof fb - 1 && memcmp(b.data, fb, b.len) == 0,
		       "extract-me1-b");
		esp_buf_free(&a);
		esp_buf_free(&b);
		esp_close_archive(&opened);
		esp_buf_free(&arc);

		/* Encrypted archive with /ME2 */
		expect(esp_create_archive(files, 2, 0, 1, ESP_MM_NONE, ESP_ME_NEW, "secret", 0, NULL, 0, &arc) == ESP_OK,
		       "create-me2");
		expect(esp_open_archive_password(arc.data, arc.len, NULL, &opened) == ESP_ERR_PASSWORD,
		       "open-me2-no-pass");
		expect(esp_open_archive_password(arc.data, arc.len, "wrong", &opened) == ESP_ERR_PASSWORD,
		       "open-me2-wrong-pass");
		expect(esp_open_archive_password(arc.data, arc.len, "secret", &opened) == ESP_OK,
		       "open-me2-ok");
		expect(opened.version == ESP_VERSION_17, "me2-version-0x17");
		expect((opened.method & ESP_METHOD_PASS) != 0, "me2-method-pass-flag");
		expect(esp_extract_entry(&opened, 0, &a) == ESP_OK &&
			   a.len == sizeof fa - 1 && memcmp(a.data, fa, a.len) == 0,
		       "extract-me2-a");
		expect(esp_extract_entry(&opened, 1, &b) == ESP_OK &&
			   b.len == sizeof fb - 1 && memcmp(b.data, fb, b.len) == 0,
		       "extract-me2-b");
		esp_buf_free(&a);
		esp_buf_free(&b);
		esp_close_archive(&opened);
		esp_buf_free(&arc);
	}

	/* Multi-volume tests */
	{
		uint32_t sz = 0;
		char p[1024];
		expect(esp_parse_volume_size("V1440", &sz) == ESP_OK && sz == 1456400, "vol-parse-vv1440");
		expect(esp_parse_volume_size("V1200", &sz) == ESP_OK && sz == 1212400, "vol-parse-vv1200");
		expect(esp_parse_volume_size("V720", &sz) == ESP_OK && sz == 729400, "vol-parse-vv720");
		expect(esp_parse_volume_size("V360", &sz) == ESP_OK && sz == 361400, "vol-parse-vv360");
		expect(esp_parse_volume_size("V999", &sz) == ESP_ERR_PARAM, "vol-parse-vv-bad");
		expect(esp_parse_volume_size("10K", &sz) == ESP_OK && sz == 10240, "vol-parse-10k");
		expect(esp_parse_volume_size("10k", &sz) == ESP_OK && sz == 10000, "vol-parse-10k-dec");
		expect(esp_parse_volume_size("1M", &sz) == ESP_OK && sz == 1048576, "vol-parse-1m");
		expect(esp_parse_volume_size("100", &sz) == ESP_OK && sz == 102400, "vol-parse-100-kb");
		expect(esp_parse_volume_size("5000", &sz) == ESP_OK && sz == 5000, "vol-parse-5000-b");

		esp_format_volume_path("test.esp", 1, p, sizeof p);
		expect(strcmp(p, "test.esp") == 0, "vol-path-v1");
		esp_format_volume_path("test.esp", 2, p, sizeof p);
		expect(strcmp(p, "test.e01") == 0, "vol-path-v2");
		esp_format_volume_path("TEST.ESP", 2, p, sizeof p);
		expect(strcmp(p, "TEST.E01") == 0, "vol-path-v2-upper");
		esp_format_volume_path("data.xyz", 12, p, sizeof p);
		expect(strcmp(p, "data.x11") == 0, "vol-path-v12");

		/* Multi-volume creation, extraction & error handling */
		uint8_t big[12000];
		size_t i;
		for (i = 0; i < sizeof big; i++)
			big[i] = (uint8_t)((i * 37 + 13) & 0xFF);

		EspInputFile vf[1];
		memset(vf, 0, sizeof vf);
		vf[0].src_path = "big.bin";
		strcpy(vf[0].stored_name, "BIG.BIN");
		vf[0].size = sizeof big;
		vf[0].data = big;

		EspBuf arc, reasm, ext;
		EspArchive opened;
		expect(esp_create_archive(vf, 1, 4, 0, ESP_MM_NONE, ESP_ME_NEW, NULL, 0, NULL, 0, &arc) == ESP_OK, "vol-create-big");
		expect(esp_write_archive_volumes("/tmp/selftest_vol.esp", &arc, 4096) == ESP_OK, "vol-write-split");
		esp_buf_free(&arc);

		expect(esp_read_archive_volumes("/tmp/selftest_vol.esp", &reasm) == ESP_OK, "vol-read-reassemble");
		expect(esp_open_archive(reasm.data, reasm.len, &opened) == ESP_OK, "vol-open-reasm");
		expect(esp_extract_entry(&opened, 0, &ext) == ESP_OK, "vol-extract");
		expect(ext.len == sizeof big && memcmp(ext.data, big, sizeof big) == 0, "vol-data-match");
		esp_buf_free(&ext);
		esp_close_archive(&opened);
		esp_buf_free(&reasm);

		/* Remove one continuation volume and test missing volume detection */
		unlink("/tmp/selftest_vol.e01");
		expect(esp_read_archive_volumes("/tmp/selftest_vol.esp", &reasm) == ESP_ERR_VOLUME, "vol-missing-detected");
		unlink("/tmp/selftest_vol.esp");
		unlink("/tmp/selftest_vol.e02");
		unlink("/tmp/selftest_vol.e03");
	}

	/* Archive Comments (/Cn) tests */
	{
		const char *cmt_text = "Hello world archive comment!\r\n";
		size_t cmt_len = strlen(cmt_text);
		EspBuf arc;
		EspArchive opened;

		/* 1. ASCII Text Comment (/C1) */
		expect(esp_create_archive(files, 2, 0, 1, ESP_MM_NONE, ESP_ME_NEW, NULL,
					  ESP_CMT_TEXT, (const uint8_t *)cmt_text, cmt_len, &arc) == ESP_OK,
		       "create-cmt-text");
		expect(arc.len >= 16 && memcmp(arc.data + arc.len - 4, "ESPC", 4) == 0, "cmt-magic-espc");
		expect(esp_open_archive(arc.data, arc.len, &opened) == ESP_OK, "open-cmt-text");
		expect(opened.has_comment == 1, "cmt-has-flag");
		expect(opened.cmt_type == ESP_CMT_TEXT, "cmt-type-text");
		expect(opened.comment.len == cmt_len &&
		       memcmp(opened.comment.data, cmt_text, cmt_len) == 0, "cmt-content-match");
		esp_close_archive(&opened);
		esp_buf_free(&arc);

		/* 2. Color ANSI Screen Binary Comment (/C2) */
		uint8_t ansi_screen[80];
		memset(ansi_screen, 0x5A, sizeof(ansi_screen));
		expect(esp_create_archive(files, 2, 0, 1, ESP_MM_NONE, ESP_ME_NEW, NULL,
					  ESP_CMT_ANSI, ansi_screen, sizeof(ansi_screen), &arc) == ESP_OK,
		       "create-cmt-ansi");
		expect(esp_open_archive(arc.data, arc.len, &opened) == ESP_OK, "open-cmt-ansi");
		expect(opened.has_comment == 1 && opened.cmt_type == ESP_CMT_ANSI, "cmt-type-ansi");
		expect(opened.comment.len == sizeof(ansi_screen) &&
		       memcmp(opened.comment.data, ansi_screen, sizeof(ansi_screen)) == 0, "cmt-ansi-match");
		esp_close_archive(&opened);
		esp_buf_free(&arc);

		/* 3. VGA PIC Comment (/C5 -> type 7) */
		uint8_t pic_data[64];
		memset(pic_data, 0x33, sizeof(pic_data));
		expect(esp_create_archive(files, 2, 0, 1, ESP_MM_NONE, ESP_ME_NEW, NULL,
					  ESP_CMT_PIC_VGA, pic_data, sizeof(pic_data), &arc) == ESP_OK,
		       "create-cmt-vga");
		expect(esp_open_archive(arc.data, arc.len, &opened) == ESP_OK, "open-cmt-vga");
		expect(opened.has_comment == 1 && opened.cmt_type == ESP_CMT_PIC_VGA, "cmt-type-vga");
		expect(opened.comment.len == sizeof(pic_data) &&
		       memcmp(opened.comment.data, pic_data, sizeof(pic_data)) == 0, "cmt-vga-match");
		esp_close_archive(&opened);
		esp_buf_free(&arc);

		/* 4. Encrypted archive with comment */
		expect(esp_create_archive(files, 2, 0, 1, ESP_MM_NONE, ESP_ME_NEW, "cmtpass",
					  ESP_CMT_TEXT, (const uint8_t *)cmt_text, cmt_len, &arc) == ESP_OK,
		       "create-cmt-pass");
		expect(esp_open_archive_password(arc.data, arc.len, "wrong", &opened) == ESP_ERR_PASSWORD,
		       "open-cmt-pass-wrong");
		expect(esp_open_archive_password(arc.data, arc.len, "cmtpass", &opened) == ESP_OK,
		       "open-cmt-pass-ok");
		expect(opened.has_comment == 1 && opened.comment.len == cmt_len &&
		       memcmp(opened.comment.data, cmt_text, cmt_len) == 0, "cmt-pass-content-match");
		esp_close_archive(&opened);
		esp_buf_free(&arc);
	}

	if (fails) {
		printf("%d selftests failed\n", fails);
		return 1;
	}
	printf("selftest ok\n");
	return 0;
}
