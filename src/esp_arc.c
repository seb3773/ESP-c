#include "esp_int.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

static uint32_t rd_u32(const uint8_t *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
	       ((uint32_t)p[3] << 24);
}

static uint16_t rd_u16(const uint8_t *p)
{
	return (uint16_t)(p[0] | (p[1] << 8));
}

static void wr_u32(uint8_t *p, uint32_t v)
{
	p[0] = (uint8_t)(v);
	p[1] = (uint8_t)(v >> 8);
	p[2] = (uint8_t)(v >> 16);
	p[3] = (uint8_t)(v >> 24);
}

static void wr_u16(uint8_t *p, uint16_t v)
{
	p[0] = (uint8_t)(v);
	p[1] = (uint8_t)(v >> 8);
}

static void dos83(const char *name, char *out)
{
	int i;
	memset(out, 0, 13);
	for (i = 0; i < 12 && name[i]; i++) {
		char c = name[i];
		if (c >= 'a' && c <= 'z')
			c = (char)(c - 'a' + 'A');
		out[i] = c;
	}
}

static void parse_dir(const uint8_t *blob, size_t len, EspArchive *arc)
{
	char path_mem[ESP_PATH_MEM];
	int path_len = 0;
	size_t i = 0;
	int cap = 16;
	arc->entries = (EspEntry *)calloc((size_t)cap, sizeof(EspEntry));
	arc->n_entries = 0;
	path_mem[0] = 0;
	while (i + ESP_DIR_ENTRY <= len) {
		const uint8_t *de = blob + i;
		EspEntry e;
		uint16_t pathi;
		int k;
		memset(&e, 0, sizeof(e));
		e.repsize = rd_u32(de + 0);
		pathi = rd_u16(de + 4);
		for (k = 0; k < 12; k++) {
			e.name[k] = (char)de[6 + k];
			if (!e.name[k])
				break;
		}
		e.name[12] = 0;
		e.attrib = de[19];
		e.datetime = rd_u32(de + 20);
		e.size = rd_u32(de + 24);
		e.is_dir = (e.attrib & 0x10) != 0;
		if (pathi != 0xFFFF && pathi < (uint16_t)path_len)
			strncpy(e.path, path_mem + pathi, sizeof(e.path) - 1);
		if (e.is_dir) {
			int n;
			n = 0;
			if (e.path[0])
				n = snprintf(path_mem + path_len, (size_t)(ESP_PATH_MEM - path_len),
					     "%s/%s", e.path, e.name);
			else
				n = snprintf(path_mem + path_len, (size_t)(ESP_PATH_MEM - path_len),
					     "%s", e.name);
			if (n > 0)
				path_len += n + 1;
		}
		if (arc->n_entries >= cap) {
			EspEntry *ne;
			cap *= 2;
			ne = (EspEntry *)realloc(arc->entries, (size_t)cap * sizeof(EspEntry));
			if (!ne)
				break;
			arc->entries = ne;
		}
		arc->entries[arc->n_entries++] = e;
		i += ESP_DIR_ENTRY;
	}
}

static int expand_rep(EspArchive *arc, const uint8_t *solid, size_t solid_len)
{
	size_t total = 0;
	size_t off = 0;
	size_t outp = 0;
	int i;
	uint8_t *repbuf = NULL;
	size_t repcap = 0;
	size_t replen = 0;

	for (i = 0; i < arc->n_entries; i++) {
		if (!arc->entries[i].is_dir)
			total += arc->entries[i].size;
	}
	arc->unpacked = (uint8_t *)malloc(total ? total : 1);
	if (!arc->unpacked)
		return ESP_ERR_MEM;
	arc->unpacked_len = total;

	for (i = 0; i < arc->n_entries; i++) {
		EspEntry *e = &arc->entries[i];
		uint32_t rest;
		uint32_t next_rep = 0;
		int ni;
		if (e->is_dir)
			continue;
		if (e->repsize > e->size || e->repsize > replen)
			goto fail;
		if (e->repsize)
			memcpy(arc->unpacked + outp, repbuf, e->repsize);
		outp += e->repsize;
		rest = e->size - e->repsize;
		if (off + rest > solid_len)
			goto fail;
		if (rest) {
			memcpy(arc->unpacked + outp, solid + off, rest);
			off += rest;
			outp += rest;
		}
		for (ni = i + 1; ni < arc->n_entries; ni++) {
			if (!arc->entries[ni].is_dir) {
				next_rep = arc->entries[ni].repsize;
				break;
			}
		}
		if (next_rep > e->size)
			goto fail;
		if (next_rep > repcap) {
			uint8_t *nb = (uint8_t *)malloc(next_rep);
			if (!nb)
				goto fail;
			if (replen && repbuf)
				memcpy(nb, repbuf, replen);
			free(repbuf);
			repbuf = nb;
			repcap = next_rep;
		}
		if (next_rep > replen && repbuf)
			memcpy(repbuf + replen,
			       arc->unpacked + (outp - e->size) + replen,
			       next_rep - replen);
		replen = next_rep;
	}
	free(repbuf);
	if (off != solid_len || outp != total)
		return ESP_ERR_DATA;
	return ESP_OK;
fail:
	free(repbuf);
	return ESP_ERR_DATA;
}

void esp_filter_mm_encode(uint8_t *data, size_t len)
{
	uint8_t prev = 0;
	size_t i;
	for (i = 0; i < len; i++) {
		uint8_t c = data[i];
		data[i] = (uint8_t)(c - prev);
		prev = c;
	}
}

void esp_filter_mm_decode(uint8_t *data, size_t len)
{
	uint8_t prev = 0;
	size_t i;
	for (i = 0; i < len; i++) {
		uint8_t c = (uint8_t)(data[i] + prev);
		data[i] = c;
		prev = c;
	}
}

int esp_filter_mm2_encode(uint8_t *data, size_t len)
{
	size_t N0, N1, N2;
	uint8_t *tmp;
	uint8_t prev0 = 0, prev1 = 0, prev2 = 0;
	size_t in_idx = 0;
	size_t p;

	if (len == 0)
		return ESP_OK;

	N0 = (len + 2) / 3;
	N1 = (len + 1) / 3;
	N2 = len / 3;

	tmp = (uint8_t *)malloc(len);
	if (!tmp)
		return ESP_ERR_MEM;

	for (p = 0; p < N0; p++) {
		uint8_t c0 = data[in_idx++];
		uint8_t delta0 = (uint8_t)(c0 - prev0);
		prev0 = c0;
		tmp[p] = delta0;

		if (p < N1) {
			uint8_t c1 = data[in_idx++];
			uint8_t delta1 = (uint8_t)(c1 - prev1);
			prev1 = c1;
			tmp[N0 + p] = (uint8_t)(delta1 - delta0);

			if (p < N2) {
				uint8_t c2 = data[in_idx++];
				uint8_t delta2 = (uint8_t)(c2 - prev2);
				prev2 = c2;
				tmp[N0 + N1 + p] = (uint8_t)(delta2 - delta1);
			}
		}
	}
	memcpy(data, tmp, len);
	free(tmp);
	return ESP_OK;
}

int esp_filter_mm2_decode(uint8_t *data, size_t len)
{
	size_t N0, N1, N2;
	uint8_t *tmp;
	const uint8_t *plane0, *plane1, *plane2;
	uint8_t prev0 = 0, prev1 = 0, prev2 = 0;
	size_t out_idx = 0;
	size_t p;

	if (len == 0)
		return ESP_OK;

	N0 = (len + 2) / 3;
	N1 = (len + 1) / 3;
	N2 = len / 3;

	tmp = (uint8_t *)malloc(len);
	if (!tmp)
		return ESP_ERR_MEM;

	plane0 = data;
	plane1 = data + N0;
	plane2 = data + N0 + N1;

	for (p = 0; p < N0; p++) {
		uint8_t delta0 = plane0[p];
		uint8_t c0 = (uint8_t)(delta0 + prev0);
		prev0 = c0;
		tmp[out_idx++] = c0;

		if (p < N1) {
			uint8_t delta1 = (uint8_t)(plane1[p] + delta0);
			uint8_t c1 = (uint8_t)(delta1 + prev1);
			prev1 = c1;
			tmp[out_idx++] = c1;

			if (p < N2) {
				uint8_t delta2 = (uint8_t)(plane2[p] + delta1);
				uint8_t c2 = (uint8_t)(delta2 + prev2);
				prev2 = c2;
				tmp[out_idx++] = c2;
			}
		}
	}
	memcpy(data, tmp, len);
	free(tmp);
	return ESP_OK;
}

int esp_open_archive_password(const uint8_t *data, size_t len, const char *password, EspArchive *arc)
{
	EspBuf dir;
	int rc;
	memset(arc, 0, sizeof(*arc));
	if (len < 10 || memcmp(data, "ESP>", 4) != 0)
		return ESP_ERR_FORMAT;
	arc->version = data[4];
	if (arc->version != ESP_VERSION_16 && arc->version != ESP_VERSION_17 && arc->version != ESP_VERSION_19)
		return ESP_ERR_FORMAT;
	arc->method = data[5];
	arc->dir_pos = rd_u32(data + 6);
	if (arc->dir_pos > len || arc->dir_pos < 10)
		return ESP_ERR_FORMAT;

	if ((arc->method & ESP_METHOD_PASS) != 0) {
		if (!password || password[0] == '\0')
			return ESP_ERR_PASSWORD;
		if (arc->version == ESP_VERSION_17)
			arc->xor_key = esp_hash_password_me2(password);
		else
			arc->xor_key = esp_hash_password_me1(password);
	} else {
		arc->xor_key = ESP_XOR_KEY;
	}

	arc->file = (uint8_t *)malloc(len);
	if (!arc->file)
		return ESP_ERR_MEM;
	memcpy(arc->file, data, len);
	arc->file_len = len;

	/* Check for archive comment footer (16 bytes ending with 'ESPC') */
	size_t dirstream_len = len - arc->dir_pos;
	if (len >= 26 && memcmp(data + len - 4, "ESPC", 4) == 0) {
		uint32_t cmt_pos = rd_u32(data + len - 16);
		if (cmt_pos >= arc->dir_pos && cmt_pos <= (uint32_t)(len - 16)) {
			arc->has_comment = 1;
			arc->cmt_pos = cmt_pos;
			arc->cmt_type = data[len - 8];
			dirstream_len = arc->cmt_pos - arc->dir_pos;
		}
	}

	rc = esp_lzss_decode_key(data + arc->dir_pos, dirstream_len, &dir, arc->xor_key);
	if (rc != ESP_OK) {
		free(arc->file);
		arc->file = NULL;
		return ((arc->method & ESP_METHOD_PASS) != 0) ? ESP_ERR_PASSWORD : rc;
	}
	parse_dir(dir.data, dir.len, arc);
	esp_buf_free(&dir);

	if (arc->has_comment) {
		size_t cmt_stream_len = (len - 16) - arc->cmt_pos;
		rc = esp_lzss_decode_key(data + arc->cmt_pos, cmt_stream_len, &arc->comment, arc->xor_key);
		if (rc != ESP_OK) {
			esp_close_archive(arc);
			return ((arc->method & ESP_METHOD_PASS) != 0) ? ESP_ERR_PASSWORD : rc;
		}
	}

	{
		EspBuf solid;
		solid.data = NULL;
		solid.len = 0;
		solid.cap = 0;
		if ((arc->method & 7) == ESP_METH_STORE) {
			size_t plen = arc->dir_pos - 10;
			if (esp_buf_init(&solid, plen ? plen : 1) != ESP_OK) {
				esp_close_archive(arc);
				return ESP_ERR_MEM;
			}
			if (plen)
				memcpy(solid.data, data + 10, plen);
			solid.len = plen;
		} else {
			rc = esp_lzss_decode_key(data + 10, arc->dir_pos - 10, &solid, arc->xor_key);
			if (rc != ESP_OK) {
				esp_close_archive(arc);
				return ((arc->method & ESP_METHOD_PASS) != 0) ? ESP_ERR_PASSWORD : rc;
			}
		}
		if (arc->version == ESP_VERSION_19) {
			rc = esp_filter_mm2_decode(solid.data, solid.len);
			if (rc != ESP_OK) {
				esp_buf_free(&solid);
				esp_close_archive(arc);
				return rc;
			}
		} else if ((arc->method & ESP_METHOD_MM) != 0) {
			esp_filter_mm_decode(solid.data, solid.len);
		}
		rc = expand_rep(arc, solid.data, solid.len);
		esp_buf_free(&solid);
		if (rc != ESP_OK) {
			esp_close_archive(arc);
			return rc;
		}
	}
	return ESP_OK;
}

int esp_open_archive(const uint8_t *data, size_t len, EspArchive *arc)
{
	return esp_open_archive_password(data, len, NULL, arc);
}

void esp_close_archive(EspArchive *arc)
{
	free(arc->file);
	free(arc->entries);
	free(arc->unpacked);
	esp_buf_free(&arc->comment);
	memset(arc, 0, sizeof(*arc));
}

int esp_extract_entry(const EspArchive *arc, int index, EspBuf *out)
{
	int i;
	size_t off = 0;
	if (index < 0 || index >= arc->n_entries)
		return ESP_ERR_FORMAT;
	if (arc->entries[index].is_dir) {
		if (esp_buf_init(out, 1) != ESP_OK)
			return ESP_ERR_MEM;
		return ESP_OK;
	}
	for (i = 0; i < index; i++) {
		if (!arc->entries[i].is_dir)
			off += arc->entries[i].size;
	}
	if (off + arc->entries[index].size > arc->unpacked_len)
		return ESP_ERR_DATA;
	if (esp_buf_init(out, arc->entries[index].size) != ESP_OK)
		return ESP_ERR_MEM;
	return esp_buf_append(out, arc->unpacked + off, arc->entries[index].size);
}

static void compute_repsizes(EspInputFile *files, int n, int enable_rep)
{
	int i;
	EspInputFile *prev = NULL;
	for (i = 0; i < n; i++) {
		files[i].repsize = 0;
		if (files[i].is_dir)
			continue;
		if (enable_rep && prev && prev->data && files[i].data) {
			uint32_t m = prev->size < files[i].size ? prev->size : files[i].size;
			uint32_t c = 0;
			while (c < m && prev->data[c] == files[i].data[c])
				c++;
			files[i].repsize = c;
		}
		prev = &files[i];
	}
}

static void make_dir_blob(EspInputFile *files, int n, EspBuf *blob)
{
	char path_mem[ESP_PATH_MEM];
	int path_len = 0;
	int i;
	esp_buf_init(blob, (size_t)n * ESP_DIR_ENTRY);
	path_mem[0] = 0;
	for (i = 0; i < n; i++) {
		uint8_t de[ESP_DIR_ENTRY];
		uint16_t pathi = 0xFFFF;
		char name[13];
		int k;
		memset(de, 0, sizeof(de));
		wr_u32(de + 0, files[i].repsize);
		if (files[i].stored_path[0]) {
			int p = 0;
			while (p < path_len) {
				if (strcmp(path_mem + p, files[i].stored_path) == 0) {
					pathi = (uint16_t)p;
					break;
				}
				p += (int)strlen(path_mem + p) + 1;
			}
			if (pathi == 0xFFFF) {
				int L = (int)strlen(files[i].stored_path);
				if (path_len + L + 1 < ESP_PATH_MEM) {
					memcpy(path_mem + path_len, files[i].stored_path, (size_t)L + 1);
					pathi = (uint16_t)path_len;
					path_len += L + 1;
				}
			}
		}
		wr_u16(de + 4, pathi);
		dos83(files[i].stored_name, name);
		for (k = 0; k < 12; k++)
			de[6 + k] = (uint8_t)name[k];
		de[18] = 0;
		de[19] = files[i].attrib ? files[i].attrib
					 : (uint8_t)(files[i].is_dir ? 0x10 : 0x20);
		wr_u32(de + 20, files[i].datetime);
		wr_u32(de + 24, files[i].is_dir ? 4096u : files[i].size);
		esp_buf_append(blob, de, ESP_DIR_ENTRY);
		if (files[i].is_dir) {
			char full[256];
			int L;
			if (files[i].stored_path[0])
				snprintf(full, sizeof(full), "%s/%s", files[i].stored_path, name);
			else
				snprintf(full, sizeof(full), "%s", name);
			L = (int)strlen(full);
			if (path_len + L + 1 < ESP_PATH_MEM) {
				memcpy(path_mem + path_len, full, (size_t)L + 1);
				path_len += L + 1;
			}
		}
	}
}

int esp_create_archive(EspInputFile *files, int n_files, int method,
		       int enable_rep, int mm_mode, int me_version,
		       const char *password, int cmt_type,
		       const uint8_t *cmt_data, size_t cmt_len,
		       EspBuf *out)
{
	EspBuf solid;
	EspBuf payload;
	EspBuf dirblob;
	EspBuf dirstream;
	EspBuf cmtstream;
	uint8_t hdr[10];
	int i, rc;
	size_t dir_pos;
	uint32_t key;
	int has_pass = (password != NULL && password[0] != '\0');
	int has_cmt = (cmt_data != NULL && cmt_len > 0);

	cmtstream.data = NULL;
	cmtstream.len = cmtstream.cap = 0;

	if (has_pass && mm_mode == ESP_MM_24BIT && me_version == ESP_ME_NEW)
		return ESP_ERR_PARAM; /* Cannot use /ME2 and /MM2 in same time */

	if (has_pass) {
		if (me_version == ESP_ME_LEGACY)
			key = esp_hash_password_me1(password);
		else
			key = esp_hash_password_me2(password);
	} else {
		key = ESP_XOR_KEY;
	}

	compute_repsizes(files, n_files, enable_rep);

	if (esp_buf_init(out, 64) != ESP_OK)
		return ESP_ERR_MEM;
	if (esp_buf_init(&solid, 64) != ESP_OK) {
		esp_buf_free(out);
		return ESP_ERR_MEM;
	}
	for (i = 0; i < n_files; i++) {
		if (!files[i].is_dir && files[i].data && files[i].size > files[i].repsize) {
			rc = esp_buf_append(&solid, files[i].data + files[i].repsize,
					    files[i].size - files[i].repsize);
			if (rc != ESP_OK) {
				esp_buf_free(&solid);
				esp_buf_free(out);
				return rc;
			}
		}
	}

	if (mm_mode == ESP_MM_8BIT) {
		esp_filter_mm_encode(solid.data, solid.len);
	} else if (mm_mode == ESP_MM_24BIT) {
		rc = esp_filter_mm2_encode(solid.data, solid.len);
		if (rc != ESP_OK) {
			esp_buf_free(&solid);
			esp_buf_free(out);
			return rc;
		}
	}

	if ((method & 7) == ESP_METH_STORE) {
		payload = solid;
		solid.data = NULL;
		solid.len = solid.cap = 0;
	} else {
		rc = esp_lzss_encode_key(solid.data, solid.len, &payload, method & 7, key);
		esp_buf_free(&solid);
		if (rc != ESP_OK) {
			esp_buf_free(out);
			return rc;
		}
	}

	make_dir_blob(files, n_files, &dirblob);
	rc = esp_lzss_encode_key(dirblob.data, dirblob.len, &dirstream, ESP_METH_BEST, key);
	esp_buf_free(&dirblob);
	if (rc != ESP_OK) {
		esp_buf_free(&payload);
		esp_buf_free(out);
		return rc;
	}

	if (has_cmt) {
		rc = esp_lzss_encode_key(cmt_data, cmt_len, &cmtstream, ESP_METH_FAST, key);
		if (rc != ESP_OK) {
			esp_buf_free(&payload);
			esp_buf_free(&dirstream);
			esp_buf_free(out);
			return rc;
		}
	}

	dir_pos = 10 + payload.len;
	memcpy(hdr, "ESP>", 4);
	if (has_pass) {
		hdr[4] = (uint8_t)(me_version == ESP_ME_NEW ? ESP_VERSION_17 : ESP_VERSION_16);
		hdr[5] = (uint8_t)((method & 7) | (mm_mode ? ESP_METHOD_MM : 0) | ESP_METHOD_PASS);
	} else {
		hdr[4] = (uint8_t)(mm_mode == ESP_MM_24BIT ? ESP_VERSION_19 : ESP_VERSION_16);
		hdr[5] = (uint8_t)((method & 7) | (mm_mode ? ESP_METHOD_MM : 0));
	}
	wr_u32(hdr + 6, (uint32_t)dir_pos);
	rc = esp_buf_append(out, hdr, 10);
	if (rc == ESP_OK)
		rc = esp_buf_append(out, payload.data, payload.len);
	if (rc == ESP_OK)
		rc = esp_buf_append(out, dirstream.data, dirstream.len);
	if (rc == ESP_OK && has_cmt) {
		uint32_t cmt_pos = 10 + (uint32_t)payload.len + (uint32_t)dirstream.len;
		uint8_t footer[16];
		memset(footer, 0, 16);
		wr_u32(footer + 0, cmt_pos);
		footer[8] = (uint8_t)(cmt_type ? cmt_type : ESP_CMT_TEXT);
		footer[9] = 1;
		memcpy(footer + 12, "ESPC", 4);

		rc = esp_buf_append(out, cmtstream.data, cmtstream.len);
		if (rc == ESP_OK)
			rc = esp_buf_append(out, footer, 16);
	}
	esp_buf_free(&payload);
	esp_buf_free(&dirstream);
	esp_buf_free(&cmtstream);
	if (rc != ESP_OK)
		esp_buf_free(out);
	return rc;
}

int esp_write_archive_volumes(const char *base_path, const EspBuf *arc_buf, uint32_t vol_size)
{
	if (!base_path || !arc_buf || !arc_buf->data)
		return ESP_ERR_PARAM;

	if (vol_size == 0 || arc_buf->len <= vol_size)
		return esp_write_file(base_path, arc_buf->data, arc_buf->len);

	if (vol_size < 32)
		return ESP_ERR_PARAM;

	uint32_t total_len = (uint32_t)arc_buf->len;
	uint32_t orig_dirpos = rd_u32(arc_buf->data + 6);
	uint8_t version = arc_buf->data[4];
	uint8_t method = arc_buf->data[5];

	/* Volume 1 */
	char vpath[1024];
	esp_format_volume_path(base_path, 1, vpath, sizeof(vpath));

	FILE *f1 = fopen(vpath, "wb");
	if (!f1)
		return ESP_ERR_IO;

	uint8_t hdr1[10];
	memcpy(hdr1, "ESP>", 4);
	hdr1[4] = version;
	hdr1[5] = method;
	wr_u32(hdr1 + 6, 0xFFFFFFFF);
	if (fwrite(hdr1, 1, 10, f1) != 10) {
		fclose(f1);
		return ESP_ERR_IO;
	}
	size_t p1_len = vol_size - 10;
	if (p1_len > total_len - 10)
		p1_len = total_len - 10;
	if (fwrite(arc_buf->data + 10, 1, p1_len, f1) != p1_len) {
		fclose(f1);
		return ESP_ERR_IO;
	}
	fclose(f1);

	/* Continuation volumes */
	uint32_t unfrag_offset = vol_size;
	int vol_num = 2;
	while (unfrag_offset < total_len) {
		uint32_t rem = total_len - unfrag_offset;
		int is_last = (14 + rem <= vol_size);
		uint32_t slice_len = is_last ? rem : (vol_size - 14);

		esp_format_volume_path(base_path, vol_num, vpath, sizeof(vpath));
		printf("Creating next volume... disk No.: #%d\n\n", vol_num);

		FILE *fk = fopen(vpath, "wb");
		if (!fk)
			return ESP_ERR_IO;

		uint8_t hdrk[14];
		memcpy(hdrk, "ESP>", 4);
		hdrk[4] = version;
		hdrk[5] = (uint8_t)(method | ESP_METHOD_CONT | (is_last ? ESP_METHOD_LAST : 0));
		wr_u32(hdrk + 6, is_last ? orig_dirpos : 0xFFFFFFFF);
		wr_u32(hdrk + 10, unfrag_offset);

		if (fwrite(hdrk, 1, 14, fk) != 14) {
			fclose(fk);
			return ESP_ERR_IO;
		}
		if (fwrite(arc_buf->data + unfrag_offset, 1, slice_len, fk) != slice_len) {
			fclose(fk);
			return ESP_ERR_IO;
		}
		fclose(fk);

		unfrag_offset += slice_len;
		vol_num++;
	}

	return ESP_OK;
}

int esp_read_archive_volumes(const char *first_path, EspBuf *out_buf)
{
	EspBuf first;
	char actual_path[1024];
	snprintf(actual_path, sizeof(actual_path), "%s", first_path);

	int rc = esp_read_file(actual_path, &first);
	if (rc != ESP_OK) {
		char alt[1024];
		snprintf(alt, sizeof(alt), "%s.esp", first_path);
		rc = esp_read_file(alt, &first);
		if (rc != ESP_OK) {
			snprintf(alt, sizeof(alt), "%s.ESP", first_path);
			rc = esp_read_file(alt, &first);
			if (rc != ESP_OK)
				return rc;
		}
		snprintf(actual_path, sizeof(actual_path), "%s", alt);
	}

	if (first.len < 10 || memcmp(first.data, "ESP>", 4) != 0) {
		esp_buf_free(&first);
		return ESP_ERR_FORMAT;
	}

	uint32_t dir_pos = rd_u32(first.data + 6);
	if (dir_pos != 0xFFFFFFFF && (first.data[5] & ESP_METHOD_CONT) == 0) {
		*out_buf = first;
		return ESP_OK;
	}

	if ((first.data[5] & ESP_METHOD_CONT) != 0) {
		esp_buf_free(&first);
		return ESP_ERR_FORMAT;
	}

	EspBuf assembled;
	if (esp_buf_init(&assembled, first.len * 2) != ESP_OK) {
		esp_buf_free(&first);
		return ESP_ERR_MEM;
	}

	rc = esp_buf_append(&assembled, first.data, first.len);
	esp_buf_free(&first);
	if (rc != ESP_OK) {
		esp_buf_free(&assembled);
		return rc;
	}

	int vol_num = 2;
	uint32_t final_dirpos = 0xFFFFFFFF;
	while (1) {
		char vpath[1024];
		esp_format_volume_path(actual_path, vol_num, vpath, sizeof(vpath));

		EspBuf vk;
		rc = esp_read_file(vpath, &vk);
		if (rc != ESP_OK) {
			char alt_vpath[1024];
			char *dot = strrchr(vpath, '.');
			if (dot && dot[1]) {
				snprintf(alt_vpath, sizeof(alt_vpath), "%s", vpath);
				char *alt_dot = strrchr(alt_vpath, '.');
				alt_dot[1] = isupper((unsigned char)alt_dot[1]) ? (char)tolower((unsigned char)alt_dot[1]) : (char)toupper((unsigned char)alt_dot[1]);
				rc = esp_read_file(alt_vpath, &vk);
			}
			if (rc != ESP_OK) {
				esp_buf_free(&assembled);
				return ESP_ERR_VOLUME;
			}
		}

		if (vk.len < 14 || memcmp(vk.data, "ESP>", 4) != 0) {
			esp_buf_free(&vk);
			esp_buf_free(&assembled);
			return ESP_ERR_FORMAT;
		}

		uint8_t method = vk.data[5];
		if ((method & ESP_METHOD_CONT) == 0) {
			esp_buf_free(&vk);
			esp_buf_free(&assembled);
			return ESP_ERR_FORMAT;
		}

		uint32_t vk_dirpos = rd_u32(vk.data + 6);
		uint32_t vk_offset = rd_u32(vk.data + 10);

		if (vk_offset != assembled.len) {
			esp_buf_free(&vk);
			esp_buf_free(&assembled);
			return ESP_ERR_FORMAT;
		}

		rc = esp_buf_append(&assembled, vk.data + 14, vk.len - 14);
		int is_last = ((method & ESP_METHOD_LAST) != 0);
		if (vk_dirpos != 0xFFFFFFFF)
			final_dirpos = vk_dirpos;
		esp_buf_free(&vk);
		if (rc != ESP_OK) {
			esp_buf_free(&assembled);
			return rc;
		}

		if (is_last)
			break;
		vol_num++;
	}

	if (final_dirpos == 0xFFFFFFFF) {
		esp_buf_free(&assembled);
		return ESP_ERR_FORMAT;
	}

	wr_u32(assembled.data + 6, final_dirpos);
	assembled.data[5] &= 0x5F;

	*out_buf = assembled;
	return ESP_OK;
}

