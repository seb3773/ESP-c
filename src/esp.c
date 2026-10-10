#include "esp.h"

#include <dirent.h>
#include <errno.h>
#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_FILES 4096

static EspInputFile files[MAX_FILES];
static int nfiles;
static int opt_method = 0;
static int opt_recursive = 0;
static int opt_nopath = 0;
static int opt_rep = 1;
static int opt_mm = ESP_MM_NONE;
static int opt_me = ESP_ME_NEW;
static const char *opt_password = NULL;
static int opt_sort_prio[4] = {1, 2, 4, 0};
static int opt_use_ext_table = 1;
static uint32_t opt_vol_size = 0;
static int opt_cmt_type = 0;
static const char *opt_cmt_path = NULL;
static int opt_always_yes = 0;
static int opt_skip_cmt = 0;
static int opt_bare = 0;
static int opt_unix = 0;

static void print_banner(void)
{
	static int banner_printed = 0;
	if (banner_printed || opt_bare)
		return;
	printf("ESP v1.92 Extension-Sort Packer\n"
	       "(GyikSoft - 2005)\n"
	       "pure C reimplementation by seb3773\n"
	       "https://github.com/seb3773/ESP-c\n\n");
	banner_printed = 1;
}

static char **masks = NULL;
static int nmasks = 0;
static int masks_cap = 0;

static const char *ESP_EXT_TABLE[161] = {
	"DB  ", "DW  ", "DD  ", "TBL ", "DMP ", "HEX ", "DBG ", "1   ",
	"1A  ", "1B  ", "2   ", "3   ", "ASS ", "ASM ", "BPO ", "AS_ ",
	"INC ", "IN_ ", "IN1 ", "IN2 ", "IN3 ", "BLK ", "BL1 ", "BL2 ",
	"MAC ", "DB0 ", "VAR ", "BAP ", "BP2 ", "C-- ", "PML ", "LIB ",
	"H   ", "C   ", "CPP ", "C++ ", "HPP ", "SRC ", "PAS ", "BAS ",
	"EXT ", "MNU ", "BAK ", "OLD ", "TXT ", "TX_ ", "TYT ", "TWT ",
	"FMT ", "ASC ", "CTX ", "LAY ", "VAL ", "    ", "FAQ ", "ME  ",
	"EL  ", "NEW ", "NEU ", "ION ", "DIZ ", "ENG ", "HUN ", "GER ",
	"ID  ", "ANS ", "ERR ", "LOG ", "LST ", "DIR ", "BAT ", "MAK ",
	"BAV ", "INI ", "SET ", "CFG ", "X   ", "!!! ", "DOC ", "WRI ",
	"WP  ", "WP5 ", "NG  ", "XLS ", "DBF ", "PCB ", "CAD ", "SCH ",
	"PRG ", "COM ", "CO_ ", "CO1 ", "CO2 ", "OBJ ", "SYS ", "DRV ",
	"DVR ", "EXE ", "EX_ ", "OVL ", "OVR ", "OV1 ", "OV2 ", "OV3 ",
	"TPU ", "TPP ", "BGI ", "DLL ", "NLM ", "PAL ", "ICO ", "RAW ",
	"SCI ", "BMP ", "TGA ", "WAD ", "PCX ", "LBM ", "DL  ", "GL  ",
	"FLI ", "FLC ", "PCL ", "TIF ", "AVI ", "GIF ", "PCD ", "FIF ",
	"JPG ", "MPG ", "MP2 ", "MP3 ", "ZIP ", "ZOO ", "LHA ", "ARJ ",
	"A01 ", "A02 ", "A03 ", "ARK ", "RAR ", "AIN ", "ESP ", "UC2 ",
	"ZIV ", "BTC ", "BIT ", "REP ", "PCY ", "FP3 ", "FP2 ", "FP1 ",
	"WAV ", "VOC ", "MOD ", "S3M ", "XM  ", "669 ", "AMF ", "PAT ",
	"SND "
};

static void to_dos_name(const char *in, char *out)
{
	int i;
	memset(out, 0, 13);
	for (i = 0; i < 12 && in[i]; i++) {
		char c = in[i];
		if (c >= 'a' && c <= 'z')
			c = (char)(c - 'a' + 'A');
		if (c == '/')
			c = '\\';
		out[i] = c;
	}
}

static void get_file_ext(const char *name, char ext[5])
{
	const char *dot = strrchr(name, '.');
	int i;
	memset(ext, ' ', 4);
	ext[4] = '\0';
	if (dot) {
		dot++;
		for (i = 0; i < 3 && dot[i]; i++) {
			char c = dot[i];
			if (c >= 'a' && c <= 'z')
				c = (char)(c - 'a' + 'A');
			ext[i] = c;
		}
	}
}

static int prio_compare(const EspInputFile *a, const EspInputFile *b, const int prio[4])
{
	int i;
	for (i = 0; i < 4; i++) {
		int diff = 0;
		switch (prio[i]) {
		case 0: /* Name */
			diff = strcmp(a->stored_name, b->stored_name);
			break;
		case 1: /* Size */
			if (a->size < b->size) diff = -1;
			else if (a->size > b->size) diff = 1;
			break;
		case 2: /* Path */
			diff = strcmp(a->stored_path, b->stored_path);
			break;
		case 3: /* Date/Time */
			if (a->datetime < b->datetime) diff = -1;
			else if (a->datetime > b->datetime) diff = 1;
			break;
		case 4: /* Attribute */
			if (a->attrib < b->attrib) diff = -1;
			else if (a->attrib > b->attrib) diff = 1;
			break;
		default:
			break;
		}
		if (diff != 0)
			return diff;
	}
	return 0;
}

static const int *g_sort_prio = NULL;

static int sort_file_cmp(const void *p1, const void *p2)
{
	const EspInputFile *a = (const EspInputFile *)p1;
	const EspInputFile *b = (const EspInputFile *)p2;
	int diff = prio_compare(a, b, g_sort_prio);
	if (diff != 0)
		return diff;
	return a->orig_idx - b->orig_idx;
}

static int dyn_ext_cmp(const void *p1, const void *p2)
{
	const char *s1 = (const char *)p1;
	const char *s2 = (const char *)p2;
	return strcmp(s1, s2);
}

static void sort_input_files(EspInputFile *in_files, int count, const int prio[4], int use_ext_table)
{
	EspInputFile *out_files;
	EspInputFile *dirs;
	EspInputFile *reg_files;
	int ndirs = 0, nregs = 0;
	int i, out_idx = 0;
	char dynamic_exts[MAX_FILES][5];
	int ndyn = 0;

	if (count <= 1)
		return;

	out_files = (EspInputFile *)malloc(sizeof(EspInputFile) * (size_t)count);
	dirs = (EspInputFile *)malloc(sizeof(EspInputFile) * (size_t)count);
	reg_files = (EspInputFile *)malloc(sizeof(EspInputFile) * (size_t)count);
	if (!out_files || !dirs || !reg_files) {
		free(out_files);
		free(dirs);
		free(reg_files);
		return;
	}

	for (i = 0; i < count; i++) {
		if (in_files[i].is_dir) {
			dirs[ndirs++] = in_files[i];
		} else {
			reg_files[nregs++] = in_files[i];
		}
	}

	/* Copy directory entries first (hierarchical discovery order) */
	for (i = 0; i < ndirs; i++) {
		out_files[out_idx++] = dirs[i];
	}

	/* Identify dynamic extensions */
	for (i = 0; i < nregs; i++) {
		char ext[5];
		int in_table = 0;
		int k;
		get_file_ext(reg_files[i].stored_name, ext);
		if (use_ext_table) {
			for (k = 0; k < 161; k++) {
				if (memcmp(ext, ESP_EXT_TABLE[k], 4) == 0) {
					in_table = 1;
					break;
				}
			}
		}
		if (!in_table) {
			int found = 0;
			for (k = 0; k < ndyn; k++) {
				if (memcmp(dynamic_exts[k], ext, 4) == 0) {
					found = 1;
					break;
				}
			}
			if (!found && ndyn < MAX_FILES) {
				memcpy(dynamic_exts[ndyn++], ext, 5);
			}
		}
	}

	/* Sort dynamic extensions in ASCII ascending order if more than 1 */
	if (ndyn > 1) {
		qsort(dynamic_exts, (size_t)ndyn, sizeof(dynamic_exts[0]), dyn_ext_cmp);
	}

	g_sort_prio = prio;

	/* If table enabled, iterate through the 161 table extensions */
	if (use_ext_table) {
		int t;
		for (t = 0; t < 161; t++) {
			int group_start = out_idx;
			for (i = 0; i < nregs; i++) {
				char ext[5];
				get_file_ext(reg_files[i].stored_name, ext);
				if (memcmp(ext, ESP_EXT_TABLE[t], 4) == 0) {
					out_files[out_idx++] = reg_files[i];
				}
			}
			if (out_idx - group_start > 1) {
				qsort(&out_files[group_start], (size_t)(out_idx - group_start),
				      sizeof(EspInputFile), sort_file_cmp);
			}
		}
	}

	/* Iterate through dynamic extensions */
	{
		int d;
		for (d = 0; d < ndyn; d++) {
			int group_start = out_idx;
			for (i = 0; i < nregs; i++) {
				char ext[5];
				get_file_ext(reg_files[i].stored_name, ext);
				if (memcmp(ext, dynamic_exts[d], 4) == 0) {
					out_files[out_idx++] = reg_files[i];
				}
			}
			if (out_idx - group_start > 1) {
				qsort(&out_files[group_start], (size_t)(out_idx - group_start),
				      sizeof(EspInputFile), sort_file_cmp);
			}
		}
	}

	memcpy(in_files, out_files, sizeof(EspInputFile) * (size_t)count);
	free(out_files);
	free(dirs);
	free(reg_files);
}

static int add_file(const char *src, const char *stored_path, const char *stored_name, int is_dir)
{
	struct stat st;
	EspInputFile *f;
	if (nfiles >= MAX_FILES) {
		fprintf(stderr, "Too many files\n");
		return -1;
	}
	if (stat(src, &st) != 0) {
		fprintf(stderr, "Cannot stat %s\n", src);
		return -1;
	}
	f = &files[nfiles++];
	memset(f, 0, sizeof(*f));
	f->orig_idx = nfiles - 1;
	f->src_path = strdup(src);
	to_dos_name(stored_name, f->stored_name);
	{
		int i;
		memset(f->stored_path, 0, sizeof(f->stored_path));
		for (i = 0; stored_path[i] && i < 255; i++) {
			char c = stored_path[i];
			if (c >= 'a' && c <= 'z')
				c = (char)(c - 'a' + 'A');
			if (c == '\\')
				c = '/';
			f->stored_path[i] = c;
		}
	}
	f->is_dir = is_dir;
	f->attrib = (uint8_t)(is_dir ? 0x10 : 0x20);
	f->datetime = esp_unix_to_dos_datetime((uint32_t)st.st_mtime);
	if (is_dir) {
		f->size = 4096;
		return 0;
	}
	f->size = (uint32_t)st.st_size;
	if (f->size) {
		EspBuf b;
		if (esp_read_file(src, &b) != ESP_OK) {
			fprintf(stderr, "Cannot read %s\n", src);
			return -1;
		}
		f->data = b.data;
		f->size = (uint32_t)b.len;
	}
	return 0;
}

static int scan_dir_entry_cmp(const void *a, const void *b)
{
	const char *na = *(const char * const *)a;
	const char *nb = *(const char * const *)b;
	return strcasecmp(na, nb);
}

static int scan_dir(const char *dir, const char *stored)
{
	DIR *d;
	struct dirent *de;
	char **entries = NULL;
	int count = 0, cap = 0;
	int i;

	d = opendir(dir);
	if (!d)
		return -1;

	while ((de = readdir(d))) {
		if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
			continue;
		if (count >= cap) {
			int ncap = cap ? cap * 2 : 64;
			char **ne = (char **)realloc(entries, (size_t)ncap * sizeof(char *));
			if (!ne) {
				closedir(d);
				for (i = 0; i < count; i++)
					free(entries[i]);
				free(entries);
				return -1;
			}
			entries = ne;
			cap = ncap;
		}
		entries[count++] = strdup(de->d_name);
	}
	closedir(d);

	if (count > 1) {
		qsort(entries, (size_t)count, sizeof(char *), scan_dir_entry_cmp);
	}

	/* First pass: subdirectories (in alphabetical order) */
	for (i = 0; i < count; i++) {
		char src[1024];
		char next_stored[512];
		struct stat st;

		snprintf(src, sizeof(src), "%s/%s", dir, entries[i]);
		if (stat(src, &st) != 0)
			continue;

		if (S_ISDIR(st.st_mode)) {
			if (stored[0])
				snprintf(next_stored, sizeof(next_stored), "%s/%s", stored, entries[i]);
			else
				snprintf(next_stored, sizeof(next_stored), "%s", entries[i]);

			if (!opt_nopath)
				add_file(src, stored, entries[i], 1);
			if (opt_recursive)
				scan_dir(src, opt_nopath ? "" : next_stored);
		}
	}

	/* Second pass: regular files (in alphabetical order) */
	for (i = 0; i < count; i++) {
		char src[1024];
		struct stat st;

		snprintf(src, sizeof(src), "%s/%s", dir, entries[i]);
		if (stat(src, &st) != 0)
			continue;

		if (S_ISREG(st.st_mode)) {
			add_file(src, opt_nopath ? "" : stored, entries[i], 0);
		}
	}

	for (i = 0; i < count; i++)
		free(entries[i]);
	free(entries);

	return 0;
}

static void add_mask(const char *mask)
{
	if (nmasks >= masks_cap) {
		int ncap = masks_cap ? masks_cap * 2 : 16;
		char **nm = (char **)realloc(masks, (size_t)ncap * sizeof(char *));
		if (!nm)
			return;
		masks = nm;
		masks_cap = ncap;
	}
	masks[nmasks++] = strdup(mask);
}

static int load_listfile_masks(const char *path)
{
	FILE *fp = fopen(path, "r");
	char line[1024];
	if (!fp) {
		fprintf(stderr, "Cannot open listfile: %s\n", path);
		return -1;
	}
	while (fgets(line, sizeof(line), fp)) {
		char *p = line;
		char *end;
		while (*p == ' ' || *p == '\t')
			p++;
		end = p + strlen(p);
		while (end > p && (end[-1] == '\r' || end[-1] == '\n' || end[-1] == ' ' || end[-1] == '\t')) {
			end--;
			*end = '\0';
		}
		if (*p == '\0' || *p == '#' || *p == ';')
			continue;
		{
			char *tok = strtok(p, " \t");
			while (tok) {
				add_mask(tok);
				tok = strtok(NULL, " \t");
			}
		}
	}
	fclose(fp);
	return 0;
}

static int wildcard_match_part(const char *pat, const char *str)
{
	while (*pat) {
		if (*pat == '*') {
			pat++;
			if (!*pat)
				return 1;
			while (*str) {
				if (wildcard_match_part(pat, str))
					return 1;
				str++;
			}
			return 0;
		}
		if (!*str)
			return 0;
		if (*pat != '?') {
			char cp = *pat;
			char cs = *str;
			if (cp >= 'a' && cp <= 'z')
				cp = (char)(cp - 'a' + 'A');
			if (cs >= 'a' && cs <= 'z')
				cs = (char)(cs - 'a' + 'A');
			if (cp == '\\')
				cp = '/';
			if (cs == '\\')
				cs = '/';
			if (cp != cs)
				return 0;
		}
		pat++;
		str++;
	}
	return *str == '\0';
}

static int entry_matches_masks(const EspEntry *e)
{
	int k;
	char full[512];
	if (nmasks == 0)
		return 1;

	if (e->path[0])
		snprintf(full, sizeof(full), "%s/%s", e->path, e->name);
	else
		snprintf(full, sizeof(full), "%s", e->name);

	for (k = 0; k < nmasks; k++) {
		const char *m = masks[k];
		if (!strchr(m, '/') && !strchr(m, '\\')) {
			if (wildcard_match_part(m, e->name))
				return 1;
		} else {
			const char *sub = full;
			if (wildcard_match_part(m, full))
				return 1;
			while ((sub = strchr(sub, '/')) != NULL) {
				sub++;
				if (wildcard_match_part(m, sub))
					return 1;
			}
		}
	}
	return 0;
}

static int mkdir_p(const char *path)
{
	char tmp[1024];
	size_t i, n;
	n = strlen(path);
	if (n >= sizeof(tmp))
		return -1;
	memcpy(tmp, path, n + 1);
	for (i = 1; i < n; i++) {
		if (tmp[i] == '/') {
			tmp[i] = 0;
			if (mkdir(tmp, 0755) != 0 && errno != EEXIST)
				return -1;
			tmp[i] = '/';
		}
	}
	if (mkdir(tmp, 0755) != 0 && errno != EEXIST)
		return -1;
	return 0;
}

static void print_entry(const EspEntry *e, int verbose)
{
	int y, m, d, hh, mm, ss;
	esp_dos_datetime_to_parts(e->datetime, &y, &m, &d, &hh, &mm, &ss);
	char name[256];
	char path[512];
	size_t i;
	snprintf(name, sizeof(name), "%s", e->name);
	snprintf(path, sizeof(path), "%s", e->path);
	for (i = 0; path[i]; i++) {
		if (path[i] == '\\')
			path[i] = '/';
		if (opt_unix && path[i] >= 'A' && path[i] <= 'Z')
			path[i] = (char)(path[i] + 32);
	}
	for (i = 0; name[i]; i++) {
		if (opt_unix && name[i] >= 'A' && name[i] <= 'Z')
			name[i] = (char)(name[i] + 32);
	}

	if (e->is_dir) {
		if (verbose)
			printf("%9s  %02d/%02d/%02d %02d:%02d:%02d  %s%s%s/\n", "<DIR>",
			       m, d, y % 100, hh, mm, ss,
			       path[0] ? path : "", path[0] ? "/" : "", name);
		return;
	}
	printf("%9u  %02d/%02d/%02d %02d:%02d:%02d  ", e->size, m, d, y % 100, hh, mm, ss);
	if (verbose && path[0])
		printf("%s/", path);
	printf("%s\n", name);
}

static int is_command_arg(const char *s)
{
	if (!s || !s[0])
		return 0;
	if (strchr(s, '.') || strchr(s, '/') || strchr(s, '\\'))
		return 0;
	char c = s[0];
	if (c >= 'A' && c <= 'Z')
		c += 32;
	if (c != 'a' && c != 'c' && c != 'e' && c != 'x' &&
	    c != 'l' && c != 'v' && c != 't')
		return 0;
	if (s[1] == '\0')
		return 1;
	size_t i = 1;
	while (s[i]) {
		char m = s[i];
		if (m >= 'A' && m <= 'Z')
			m += 32;
		if (m == 'y' || m == 's' || m == '1' || m == 'u') {
			i++;
		} else if (m == 'p') {
			return 1;
		} else {
			return 0;
		}
	}
	return 1;
}

static void usage(void)
{
	fprintf(stderr,
		"Usage: esp <command> [options] <archive[.esp]> [files... | dest/ | masks...] [@listfile]\n"
		"   or: esp [options] <archive[.esp]> [files...] [@listfile]  (default: add)\n\n"
		"Commands:\n"
		"  a, c  Add / create archive (default if omitted)\n"
		"  x     Extract files with full directory tree\n"
		"  e     Extract files flat (without directories)\n"
		"  l     List archive contents\n"
		"  v     Verbose list archive contents\n"
		"  t     Test archive integrity\n\n"
		"Compression Options:\n"
		"  /M0 best  /M3 fast  /M4 store\n"
		"  /MM multimedia 8-bit  /MM2 multimedia 24-bit\n"
		"  /G<pass>, /P<pass> garble with password  /ME1 legacy encryption  /ME2 new encryption\n"
		"  /Vn[k|K|m|M] volume size (e.g. /V1440K, /V10M)  /VVdisk diskette preset (/VV1440, ...)\n"
		"  /Cn=file archive comment (1=text, 2=ANSI screen, 4=old PIC, 5=320x200 PIC, 6=640x480 PIC)\n"
		"  /R recursive  /P don't store path  /J disable prefix-rep\n"
		"  /Sabcd sorting priority: 0=name 1=size 2=path 3=datetime 4=attrib (default: 1240)\n"
		"  /T disable internal extension table\n\n"
		"Extraction & Listing Options:\n"
		"  -1, --bare  Bare output format (one path per line, script/pipe friendly)\n"
		"  -u, --unix  Format paths as Unix relative paths (/ slashes, lowercase)\n"
		"  /Y, -y      Always yes (overwrite existing files)\n"
		"  /S, -s      Skip comment display\n"
		"  @listfile   Read file list or filter masks from text file\n");
}

static int process_input_path(const char *path)
{
	struct stat st;
	if (strchr(path, '*') || strchr(path, '?')) {
		glob_t g;
		if (glob(path, 0, NULL, &g) == 0) {
			size_t k;
			for (k = 0; k < g.gl_pathc; k++) {
				process_input_path(g.gl_pathv[k]);
			}
			globfree(&g);
			return 0;
		}
		fprintf(stderr, "No match: %s\n", path);
		return -1;
	}
	if (stat(path, &st) != 0) {
		fprintf(stderr, "Not found: %s\n", path);
		return -1;
	}
	if (S_ISDIR(st.st_mode)) {
		char base[256];
		const char *slash = strrchr(path, '/');
		snprintf(base, sizeof(base), "%s", slash ? slash + 1 : path);
		if (!opt_nopath)
			add_file(path, "", base, 1);
		scan_dir(path, opt_nopath ? "" : base);
	} else if (S_ISREG(st.st_mode)) {
		const char *slash = strrchr(path, '/');
		add_file(path, "", slash ? slash + 1 : path, 0);
	}
	return 0;
}

static int process_listfile(const char *path)
{
	FILE *fp = fopen(path, "r");
	char line[1024];
	if (!fp) {
		fprintf(stderr, "Cannot open listfile: %s\n", path);
		return -1;
	}
	while (fgets(line, sizeof(line), fp)) {
		char *p = line;
		char *end;
		while (*p == ' ' || *p == '\t')
			p++;
		end = p + strlen(p);
		while (end > p && (end[-1] == '\r' || end[-1] == '\n' || end[-1] == ' ' || end[-1] == '\t')) {
			end--;
			*end = '\0';
		}
		if (*p == '\0' || *p == '#' || *p == ';')
			continue;
		struct stat st;
		if (strchr(p, '*') || strchr(p, '?') || stat(p, &st) == 0) {
			process_input_path(p);
		} else {
			char *tok = strtok(p, " \t\r\n");
			while (tok) {
				process_input_path(tok);
				tok = strtok(NULL, " \t\r\n");
			}
		}
	}
	fclose(fp);
	return 0;
}

static int run_unpack(int cmd, const char *arcpath, const char *dest)
{
	EspBuf file;
	EspArchive arc;
	int i, rc;

	rc = esp_read_archive_volumes(arcpath, &file);
	if (rc != ESP_OK) {
		if (rc == ESP_ERR_VOLUME)
			fprintf(stderr, "Cannot find next volume for archive: %s\n", arcpath);
		else
			fprintf(stderr, "Cannot open archive: %s\n", arcpath);
		return 1;
	}
	if (file.len >= 10 && (file.data[5] & ESP_METHOD_PASS) != 0 && (!opt_password || !opt_password[0])) {
		if (isatty(0)) {
			char *entered = getpass("Enter password: ");
			if (entered && entered[0])
				opt_password = entered;
		}
	}
	rc = esp_open_archive_password(file.data, file.len, opt_password, &arc);
	esp_buf_free(&file);
	if (rc != ESP_OK) {
		if (rc == ESP_ERR_PASSWORD)
			fprintf(stderr, "Invalid password or password required\n");
		else
			fprintf(stderr, "Bad archive (err %d)\n", rc);
		return 1;
	}

	if (arc.has_comment && !opt_skip_cmt && !opt_bare) {
		if (arc.cmt_type == ESP_CMT_TEXT) {
			if (arc.comment.data && arc.comment.len > 0) {
				fwrite(arc.comment.data, 1, arc.comment.len, stdout);
				if (arc.comment.data[arc.comment.len - 1] != '\n')
					printf("\n");
			}
		} else if (arc.cmt_type == ESP_CMT_ANSI) {
			printf("[Archive Comment: ANSI Screen Binary (80x25 /C2) - %zu bytes]\n", arc.comment.len);
		} else if (arc.cmt_type == ESP_CMT_PIC_VGA || arc.cmt_type == 5 || arc.cmt_type == ESP_CMT_PIC_OLD) {
			printf("[Archive Comment: VGA Graphics PIC (320x200 /C5) - %zu bytes]\n", arc.comment.len);
		} else if (arc.cmt_type == ESP_CMT_PIC_HR) {
			printf("[Archive Comment: VGA Graphics PIC (640x480 /C6) - %zu bytes]\n", arc.comment.len);
		} else {
			printf("[Archive Comment: Binary/Unknown (Type %d) - %zu bytes]\n", arc.cmt_type, arc.comment.len);
		}
	}

	if (cmd == 'l' || cmd == 'v') {
		if (opt_bare) {
			for (i = 0; i < arc.n_entries; i++) {
				const EspEntry *e = &arc.entries[i];
				if (!entry_matches_masks(e))
					continue;
				if (e->is_dir && cmd == 'l')
					continue;
				char full[1024];
				char path[512];
				char name[256];
				snprintf(path, sizeof(path), "%s", e->path);
				snprintf(name, sizeof(name), "%s", e->name);
				size_t k;
				for (k = 0; path[k]; k++) {
					if (path[k] == '\\') path[k] = '/';
					if (opt_unix && path[k] >= 'A' && path[k] <= 'Z')
						path[k] = (char)(path[k] + 32);
				}
				for (k = 0; name[k]; k++) {
					if (opt_unix && name[k] >= 'A' && name[k] <= 'Z')
						name[k] = (char)(name[k] + 32);
				}
				if (e->is_dir) {
					if (path[0])
						snprintf(full, sizeof(full), "%s/%s/", path, name);
					else
						snprintf(full, sizeof(full), "%s/", name);
				} else {
					if (path[0])
						snprintf(full, sizeof(full), "%s/%s", path, name);
					else
						snprintf(full, sizeof(full), "%s", name);
				}
				printf("%s\n", full);
			}
			esp_close_archive(&arc);
			for (i = 0; i < nmasks; i++)
				free(masks[i]);
			free(masks);
			return 0;
		}

		uint32_t total = 0;
		int nf = 0;
		printf("------------------------------------------------------------------------------\n");
		printf("     Size    Date     Time    FileName\n");
		printf("------------------------------------------------------------------------------\n");
		for (i = 0; i < arc.n_entries; i++) {
			if (!entry_matches_masks(&arc.entries[i]))
				continue;
			print_entry(&arc.entries[i], cmd == 'v');
			if (!arc.entries[i].is_dir) {
				total += arc.entries[i].size;
				nf++;
			}
		}
		printf("------------------------------------------------------------------------------\n");
		printf("  Unpacked size: %9u\n", total);
		printf("  Compressed size: %zu    Files: %d\n", arc.file_len, nf);
		esp_close_archive(&arc);
		for (i = 0; i < nmasks; i++)
			free(masks[i]);
		free(masks);
		return 0;
	}

	if (cmd == 't') {
		int tested = 0;
		for (i = 0; i < arc.n_entries; i++) {
			if (!entry_matches_masks(&arc.entries[i]))
				continue;
			tested++;
		}
		printf("Archive OK, %d entries, unpacked %zu bytes\n", tested, arc.unpacked_len);
		esp_close_archive(&arc);
		for (i = 0; i < nmasks; i++)
			free(masks[i]);
		free(masks);
		return 0;
	}

	if (cmd != 'e' && cmd != 'x') {
		fprintf(stderr, "Invalid command: %c\n", cmd);
		esp_close_archive(&arc);
		for (i = 0; i < nmasks; i++)
			free(masks[i]);
		free(masks);
		return 1;
	}

	if (dest[0])
		mkdir_p(dest);

	for (i = 0; i < arc.n_entries; i++) {
		EspEntry *e = &arc.entries[i];
		char path[1024];
		EspBuf out;
		path[0] = 0;
		if (e->is_dir) {
			if (cmd == 'x' && entry_matches_masks(e)) {
				char d_path[512], d_name[256];
				snprintf(d_path, sizeof(d_path), "%s", e->path);
				snprintf(d_name, sizeof(d_name), "%s", e->name);
				size_t k;
				for (k = 0; d_path[k]; k++) {
					if (d_path[k] == '\\') d_path[k] = '/';
					if (opt_unix && d_path[k] >= 'A' && d_path[k] <= 'Z')
						d_path[k] = (char)(d_path[k] + 32);
				}
				for (k = 0; d_name[k]; k++) {
					if (opt_unix && d_name[k] >= 'A' && d_name[k] <= 'Z')
						d_name[k] = (char)(d_name[k] + 32);
				}
				if (dest[0]) {
					snprintf(path, sizeof(path), "%s", dest);
					if (path[strlen(path) - 1] != '/')
						strncat(path, "/", sizeof(path) - strlen(path) - 1);
				}
				if (d_path[0]) {
					strncat(path, d_path, sizeof(path) - strlen(path) - 1);
					strncat(path, "/", sizeof(path) - strlen(path) - 1);
				}
				strncat(path, d_name, sizeof(path) - strlen(path) - 1);
				mkdir_p(path);
			}
			continue;
		}
		if (!entry_matches_masks(e))
			continue;

		char f_path[512], f_name[256];
		snprintf(f_path, sizeof(f_path), "%s", e->path);
		snprintf(f_name, sizeof(f_name), "%s", e->name);
		size_t k;
		for (k = 0; f_path[k]; k++) {
			if (f_path[k] == '\\') f_path[k] = '/';
			if (opt_unix && f_path[k] >= 'A' && f_path[k] <= 'Z')
				f_path[k] = (char)(f_path[k] + 32);
		}
		for (k = 0; f_name[k]; k++) {
			if (opt_unix && f_name[k] >= 'A' && f_name[k] <= 'Z')
				f_name[k] = (char)(f_name[k] + 32);
		}

		if (dest[0]) {
			snprintf(path, sizeof(path), "%s", dest);
			if (path[strlen(path) - 1] != '/')
				strncat(path, "/", sizeof(path) - strlen(path) - 1);
		}
		if (cmd == 'x' && f_path[0]) {
			strncat(path, f_path, sizeof(path) - strlen(path) - 1);
			strncat(path, "/", sizeof(path) - strlen(path) - 1);
			mkdir_p(path);
		}
		strncat(path, f_name, sizeof(path) - strlen(path) - 1);
		if (!opt_always_yes) {
			FILE *ex = fopen(path, "rb");
			if (ex) {
				fclose(ex);
				if (!opt_bare)
					printf("Skip existing %s\n", path);
				continue;
			}
		}
		rc = esp_extract_entry(&arc, i, &out);
		if (rc != ESP_OK) {
			fprintf(stderr, "Extract failed: %s\n", e->name);
			continue;
		}
		if (esp_write_file(path, out.data, out.len) != ESP_OK)
			fprintf(stderr, "Write failed: %s\n", path);
		else {
			if (opt_bare)
				printf("%s\n", path);
			else
				printf("Extracting %s  Ok.\n", path);
		}
		esp_buf_free(&out);
	}
	esp_close_archive(&arc);
	for (i = 0; i < nmasks; i++)
		free(masks[i]);
	free(masks);
	return 0;
}

static int run_pack(const char *arcname)
{
	EspBuf out;
	int rc;

	if (!arcname || nfiles == 0) {
		usage();
		return 1;
	}
	if (opt_password && opt_password[0] == '\0') {
		if (isatty(0)) {
			char *entered = getpass("Enter password: ");
			if (entered && entered[0])
				opt_password = entered;
		}
	}
	EspBuf cmt_buf;
	cmt_buf.data = NULL;
	cmt_buf.len = cmt_buf.cap = 0;
	if (opt_cmt_path && opt_cmt_path[0]) {
		rc = esp_read_file(opt_cmt_path, &cmt_buf);
		if (rc != ESP_OK) {
			fprintf(stderr, "Cannot open comment file: %s\n", opt_cmt_path);
			return 1;
		}
	}
	printf("ESP packing %d items -> %s  method %d\n", nfiles, arcname, opt_method);
	sort_input_files(files, nfiles, opt_sort_prio, opt_use_ext_table);
	rc = esp_create_archive(files, nfiles, opt_method, opt_rep, opt_mm, opt_me,
				opt_password, opt_cmt_type, cmt_buf.data, cmt_buf.len, &out);
	esp_buf_free(&cmt_buf);
	if (rc != ESP_OK) {
		if (rc == ESP_ERR_PARAM)
			fprintf(stderr, "Cannot use /ME2 and /MM2 in same time!\n");
		else
			fprintf(stderr, "Pack failed (%d)\n", rc);
		return 1;
	}
	{
		char name[1024];
		size_t L = strlen(arcname);
		if (L > 4 && (arcname[L - 4] == '.'))
			snprintf(name, sizeof(name), "%s", arcname);
		else
			snprintf(name, sizeof(name), "%s.esp", arcname);
		if (esp_write_archive_volumes(name, &out, opt_vol_size) != ESP_OK) {
			fprintf(stderr, "Cannot write %s\n", name);
			return 1;
		}
		printf("Wrote %s (%zu bytes)\n", name, out.len);
	}
	esp_buf_free(&out);
	return 0;
}

int main(int argc, char **argv)
{
	int i;
	const char *pos_args[MAX_FILES + 16];
	int n_pos = 0;
	int cmd = 0;
	const char *arcname = NULL;
	const char *dest = "";

	int is_unesp = 0;
	if (argv[0]) {
		const char *base = strrchr(argv[0], '/');
		base = base ? base + 1 : argv[0];
		if (strcasecmp(base, "unesp") == 0)
			is_unesp = 1;
	}

	if (argc < 2) {
		print_banner();
		usage();
		return 1;
	}

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--bare") == 0) {
			opt_bare = 1;
			continue;
		}
		if (strcmp(argv[i], "--unix") == 0) {
			opt_unix = 1;
			continue;
		}
		if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0 ||
		    strcmp(argv[i], "-?") == 0 || strcmp(argv[i], "/?") == 0 ||
		    strcasecmp(argv[i], "/h") == 0) {
			print_banner();
			usage();
			return 0;
		}
		if (strcmp(argv[i], "-1") == 0) {
			opt_bare = 1;
			continue;
		}
		if (strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "-U") == 0) {
			opt_unix = 1;
			continue;
		}
		if (strcmp(argv[i], "-1u") == 0 || strcmp(argv[i], "-u1") == 0 ||
		    strcmp(argv[i], "-1U") == 0 || strcmp(argv[i], "-U1") == 0) {
			opt_bare = 1;
			opt_unix = 1;
			continue;
		}

		int is_sw = 0;
		const char *s = NULL;
		if (argv[i][0] == '-' && argv[i][1] && argv[i][1] != '-') {
			is_sw = 1;
			s = argv[i] + 1;
		} else if (argv[i][0] == '/' && argv[i][1] &&
			   ((argv[i][1] >= 'A' && argv[i][1] <= 'Z') ||
			    (argv[i][1] >= 'a' && argv[i][1] <= 'z') ||
			    argv[i][1] == '1' || argv[i][1] == '?') &&
			   argv[i][2] != '/') {
			const char *first_slash = strchr(argv[i] + 1, '/');
			const char *first_eq = strchr(argv[i] + 1, '=');
			if (first_slash == NULL || (first_eq != NULL && first_eq < first_slash)) {
				is_sw = 1;
				s = argv[i] + 1;
			}
		}

		if (is_sw) {
			if (strcmp(s, "1") == 0 || strcasecmp(s, "bare") == 0) {
				opt_bare = 1;
			} else if (strcasecmp(s, "u") == 0 || strcasecmp(s, "unix") == 0) {
				opt_unix = 1;
			} else if (strcasecmp(s, "1u") == 0 || strcasecmp(s, "u1") == 0) {
				opt_bare = 1;
				opt_unix = 1;
			} else if ((s[0] == 'M' || s[0] == 'm') && (s[1] == 'M' || s[1] == 'm')) {
				if (s[2] == '2')
					opt_mm = ESP_MM_24BIT;
				else
					opt_mm = ESP_MM_8BIT;
			} else if ((s[0] == 'M' || s[0] == 'm') && (s[1] == 'E' || s[1] == 'e')) {
				if (s[2] == '1')
					opt_me = ESP_ME_LEGACY;
				else
					opt_me = ESP_ME_NEW;
			} else if ((s[0] == 'M' || s[0] == 'm') && s[1]) {
				opt_method = s[1] - '0';
			} else if (s[0] == 'G' || s[0] == 'g') {
				opt_password = s + 1;
			} else if (s[0] == 'P' || s[0] == 'p') {
				if (s[1] == '\0')
					opt_nopath = 1;
				else
					opt_password = s + 1;
			} else if (s[0] == 'R' || s[0] == 'r') {
				opt_recursive = 1;
			} else if (s[0] == 'J' || s[0] == 'j') {
				opt_rep = 0;
			} else if (s[0] == 'S' || s[0] == 's') {
				if (s[1] == '\0') {
					opt_skip_cmt = 1;
				} else {
					int k;
					if (strlen(s + 1) != 4) {
						fprintf(stderr, "Sorting priority values must be in range 0..4!\n");
						return 1;
					}
					for (k = 0; k < 4; k++) {
						char c = s[1 + k];
						if (c < '0' || c > '4') {
							fprintf(stderr, "Sorting priority values must be in range 0..4!\n");
							return 1;
						}
						opt_sort_prio[k] = c - '0';
					}
				}
			} else if (s[0] == 'T' || s[0] == 't') {
				opt_use_ext_table = 0;
			} else if (s[0] == 'V' || s[0] == 'v') {
				int rc = esp_parse_volume_size(s + 1, &opt_vol_size);
				if (rc != ESP_OK) {
					if (s[1] == 'V' || s[1] == 'v')
						fprintf(stderr, "Unknown volume size\n");
					else
						fprintf(stderr, "Invalid volume size: %s\n", s);
					return 1;
				}
			} else if (s[0] == 'C' || s[0] == 'c') {
				const char *eq = strchr(s, '=');
				if (eq) {
					opt_cmt_path = eq + 1;
					if (eq == s + 1) {
						opt_cmt_type = ESP_CMT_TEXT;
					} else if (s[1] == '1') {
						opt_cmt_type = ESP_CMT_TEXT;
					} else if (s[1] == '2') {
						opt_cmt_type = ESP_CMT_ANSI;
					} else if (s[1] == '4') {
						opt_cmt_type = ESP_CMT_PIC_OLD;
					} else if (s[1] == '5') {
						opt_cmt_type = ESP_CMT_PIC_VGA;
					} else if (s[1] == '6') {
						opt_cmt_type = ESP_CMT_PIC_HR;
					} else {
						opt_cmt_type = s[1] - '0';
					}
				}
			} else if (s[0] == 'Y' || s[0] == 'y') {
				opt_always_yes = 1;
			}
			continue;
		}

		if (n_pos < MAX_FILES + 15)
			pos_args[n_pos++] = argv[i];
	}

	if (n_pos == 0) {
		print_banner();
		usage();
		return 1;
	}

	int first_idx = 0;
	if (is_command_arg(pos_args[0])) {
		cmd = pos_args[0][0];
		if (cmd >= 'A' && cmd <= 'Z')
			cmd += 32;
		for (size_t k = 1; pos_args[0][k]; k++) {
			char m = pos_args[0][k];
			if (m >= 'A' && m <= 'Z')
				m += 32;
			if (m == 'y') {
				opt_always_yes = 1;
			} else if (m == 's') {
				opt_skip_cmt = 1;
			} else if (m == '1') {
				opt_bare = 1;
			} else if (m == 'u') {
				opt_unix = 1;
			} else if (m == 'p') {
				opt_password = &pos_args[0][k + 1];
				break;
			}
		}
		first_idx = 1;
	} else {
		if (opt_bare)
			cmd = 'l';
		else if (is_unesp)
			cmd = 'x';
		else
			cmd = 'a';
		first_idx = 0;
	}

	if (first_idx >= n_pos) {
		print_banner();
		usage();
		return 1;
	}

	print_banner();

	arcname = pos_args[first_idx++];

	if (cmd == 'a' || cmd == 'c') {
		for (i = first_idx; i < n_pos; i++) {
			const char *arg = pos_args[i];
			if (arg[0] == '@') {
				if (process_listfile(arg + 1) != 0)
					return 1;
			} else {
				process_input_path(arg);
			}
		}
		return run_pack(arcname);
	} else {
		for (i = first_idx; i < n_pos; i++) {
			const char *arg = pos_args[i];
			size_t L = strlen(arg);
			if (arg[0] == '@') {
				if (load_listfile_masks(arg + 1) != 0)
					return 1;
			} else if ((cmd == 'e' || cmd == 'x') && dest[0] == '\0' &&
				   (arg[L - 1] == '/' || arg[L - 1] == '\\')) {
				dest = arg;
			} else if ((cmd == 'e' || cmd == 'x') && dest[0] == '\0') {
				struct stat st;
				if (stat(arg, &st) == 0 && S_ISDIR(st.st_mode)) {
					dest = arg;
				} else {
					add_mask(arg);
				}
			} else {
				add_mask(arg);
			}
		}
		return run_unpack(cmd, arcname, dest);
	}
}
