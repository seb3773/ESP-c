#!/bin/sh
set -e
cd "$(dirname "$0")/.."

fail=0
ok() { echo "OK $*"; }
bad() { echo "FAIL $*"; fail=$((fail + 1)); }

for f in packed_corpus/*.ESP; do
	if ./unesp t "$f" >/dev/null; then
		ok "decode $(basename "$f")"
	else
		bad "decode $(basename "$f")"
	fi
done

rt() {
	src=$1
	stored=$2
	meth=$3
	arc=/tmp/esp_rt_${stored}_${meth}.esp
	out=/tmp/esp_rt_${stored}_${meth}_x
	mkdir -p "$out"
	./esp "/M${meth}" "$arc" "corpus/${src}" >/dev/null
	./unesp ey "$arc" "$out/" >/dev/null
	if cmp -s "$out/$stored" "corpus/${src}"; then
		ok "roundtrip ${src} M${meth}"
	else
		bad "roundtrip ${src} M${meth}"
	fi
}

rt hello.txt HELLO.TXT 0
rt hello.txt HELLO.TXT 3
rt hello.txt HELLO.TXT 4
rt empty.dat EMPTY.DAT 0
rt empty.dat EMPTY.DAT 4
rt zeros.bin ZEROS.BIN 0
rt zeros.bin ZEROS.BIN 3
rt code.c CODE.C 0
rt code.c CODE.C 3
rt pattern.bin PATTERN.BIN 0
rt mixed.bin MIXED.BIN 0
rt eng.txt ENG.TXT 0
rt biased.bin BIASED.BIN 0

./esp /R /M0 /tmp/esp_nested.esp corpus/nested >/dev/null
if ./unesp t /tmp/esp_nested.esp >/dev/null; then
	ok nested-test
else
	bad nested-test
fi
mkdir -p /tmp/esp_nested_x
./unesp xy /tmp/esp_nested.esp /tmp/esp_nested_x/ >/dev/null
if cmp -s /tmp/esp_nested_x/NESTED/DIR1/HELLO.TXT corpus/nested/dir1/hello.txt &&
   cmp -s /tmp/esp_nested_x/NESTED/MIXED.BIN corpus/nested/mixed.bin; then
	ok nested-extract
else
	bad nested-extract
fi

mkdir -p /tmp/esp_orig_x
./unesp ey packed_corpus/HELLOM0.ESP /tmp/esp_orig_x/ >/dev/null
if cmp -s /tmp/esp_orig_x/HELLO.TXT corpus/hello.txt; then
	ok orig-hello
else
	bad orig-hello
fi
./unesp ey packed_corpus/ZEROSM0.ESP /tmp/esp_orig_x/ >/dev/null
if cmp -s /tmp/esp_orig_x/ZEROS.BIN corpus/zeros.bin; then
	ok orig-zeros
else
	bad orig-zeros
fi
./unesp ey packed_corpus/CODEM0.ESP /tmp/esp_orig_x/ >/dev/null
if cmp -s /tmp/esp_orig_x/CODE.C corpus/code.c; then
	ok orig-code
else
	bad orig-code
fi

mkdir -p /tmp/esp_rep_in /tmp/esp_rep_x /tmp/esp_j_x
printf 'HEADER----aaaa' > /tmp/esp_rep_in/a.txt
printf 'HEADER----bbbb' > /tmp/esp_rep_in/b.txt
./esp /M0 /tmp/esp_rep.esp /tmp/esp_rep_in/a.txt /tmp/esp_rep_in/b.txt >/dev/null
./unesp ey /tmp/esp_rep.esp /tmp/esp_rep_x/ >/dev/null
if cmp -s /tmp/esp_rep_x/A.TXT /tmp/esp_rep_in/a.txt &&
   cmp -s /tmp/esp_rep_x/B.TXT /tmp/esp_rep_in/b.txt; then
	ok prefix-rep
else
	bad prefix-rep
fi
./esp /J /M4 /tmp/esp_j.esp /tmp/esp_rep_in/a.txt /tmp/esp_rep_in/b.txt >/dev/null
./unesp ey /tmp/esp_j.esp /tmp/esp_j_x/ >/dev/null
if cmp -s /tmp/esp_j_x/A.TXT /tmp/esp_rep_in/a.txt &&
   cmp -s /tmp/esp_j_x/B.TXT /tmp/esp_rep_in/b.txt; then
	ok prefix-rep-off
else
	bad prefix-rep-off
fi

if [ "$fail" -ne 0 ]; then
	echo "$fail tests failed"
	exit 1
fi
echo "all tests passed"
