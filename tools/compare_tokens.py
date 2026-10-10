#!/usr/bin/env python3
import struct
import sys
from esp_trace import extract_payload, trace_stream

def compare_c_with_esp(c_bin_path, esp_path):
    all_esp = trace_stream(extract_payload(esp_path))
    tokens_esp = [t for t in all_esp if t.kind in ("LIT", "MATCH")]
    tokens_c = []
    with open(c_bin_path, "rb") as f:
        while True:
            b = f.read(10)
            if not b or len(b) < 10:
                break
            off, kind, val, length = struct.unpack("<IH2H", b)
            tokens_c.append((off, kind, val, length))

    print(f"ESP tokens: {len(tokens_esp)}, C tokens: {len(tokens_c)}")
    diffs = 0
    limit = min(len(tokens_esp), len(tokens_c))
    for i in range(limit):
        e = tokens_esp[i]
        c = tokens_c[i]
        e_kind = 0 if e.kind == "LIT" else 1
        e_val = e.kwargs["val"] if e.kind == "LIT" else e.kwargs["dist"]
        e_len = 0 if e.kind == "LIT" else e.kwargs["len"]
        e_tup = (e.offset, e_kind, e_val, e_len)
        if e_tup != c:
            diffs += 1
            c_kind_str = "LIT" if c[1] == 0 else "MATCH"
            print(f"Diff #{diffs} at index {i}:")
            print(f"  ESP: {e}")
            print(f"  C:   off={c[0]} kind={c_kind_str} val={c[2]} len={c[3]}")
            if diffs >= 20:
                break
    if diffs == 0 and len(tokens_esp) == len(tokens_c):
        print(">>> 100.000% BIT-EXACT TOKEN MATCH! <<<")
        return True
    elif diffs == 0:
        print(f"Common prefix of {limit} tokens matched, but counts differ.")
        return False
    else:
        print(f"Total diffs encountered: {diffs}")
        return False

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: compare_tokens.py <c_tokens.bin> <archive.esp>")
        sys.exit(1)
    res = compare_c_with_esp(sys.argv[1], sys.argv[2])
    sys.exit(0 if res else 1)
