#!/usr/bin/env python3
"""
esp_trace.py - Bitstream inspector and token tracer for ESP archives.
Allows dissecting and comparing compressed streams token-by-token.
"""

import sys
import struct

XOR_KEY = b"GyiK"

def unxor_gyik(data: bytes) -> bytes:
    out = bytearray(len(data))
    for i, b in enumerate(data):
        out[i] = b ^ XOR_KEY[i % 4]
    return bytes(out)

class BitReader:
    def __init__(self, data: bytes):
        self.data = data
        self.pos = 0
        self.bits = 0
        self.acc = 0

    def getbits(self, n: int) -> int:
        while self.bits < n:
            b = self.data[self.pos] if self.pos < len(self.data) else 0
            self.pos += 1
            self.acc |= b << self.bits
            self.bits += 8
        val = self.acc & ((1 << n) - 1)
        self.acc >>= n
        self.bits -= n
        return val

    def get_bit_offset(self) -> int:
        return (self.pos * 8) - self.bits

class HuffNode:
    def __init__(self):
        self.symbol = -1
        self.left = None
        self.right = None

class HuffTree:
    def __init__(self, lengths: list[int]):
        self.lengths = lengths
        self.root = HuffNode()
        self.build_tree(lengths)

    def build_tree(self, lengths: list[int]):
        # ESP canonical tree builder (matching huffman_bits_2_tree in C)
        first = [-1] * 17
        next_sym = [-1] * len(lengths)
        huff_bits_counter = [0] * 17

        for i, b in enumerate(lengths):
            if b < 0 or b > 16:
                raise ValueError(f"Invalid bit length {b}")
            huff_bits_counter[b] += 1
            next_sym[i] = first[b]
            first[b] = i

        d = 0
        c = 15
        s = 1
        while c >= 0:
            d += (huff_bits_counter[s] << c)
            s += 1
            c -= 1
        if d != 65536:
            raise ValueError(f"Kraft sum check failed: d={d} != 65536")

        sorted_syms = []
        for i in range(1, 17):
            cur = first[i]
            while cur >= 0:
                sorted_syms.append(cur)
                cur = next_sym[cur]

        if not sorted_syms:
            raise ValueError("All codes are 0")

        # Recursive tree builder matching huff_make_node
        huff_bits_counter[0] = 0
        idx = [0]

        def make_node(node: HuffNode, depth: int):
            huff_bits_counter[depth] -= 1
            if huff_bits_counter[depth] >= 0:
                node.symbol = sorted_syms[idx[0]]
                idx[0] += 1
                return

            node.left = HuffNode()
            node.right = HuffNode()
            make_node(node.left, depth + 1)
            make_node(node.right, depth + 1)

        make_node(self.root, 0)

    def decode_symbol(self, br: BitReader) -> int:
        node = self.root
        while node.symbol < 0:
            bit = br.getbits(1)
            node = node.right if bit else node.left
            if node is None:
                raise ValueError("Corrupted Huffman stream")
        return node.symbol

def read_huff_table(br: BitReader, size: int) -> tuple[HuffTree, list[int]]:
    # Step 1: Read mini Huffman tree
    n = 0x13 - br.getbits(5)
    if n < 0:
        n = 0
    minibits = [0] * 0x13
    i = 0
    while i < n:
        a = br.getbits(3)
        if a == 7:
            while br.getbits(1):
                a += 1
        minibits[i] = a
        i += 1

    mini_tree = HuffTree(minibits)

    # Step 2: Read real table lengths
    n = size - br.getbits(9)
    if n < 0:
        n = 0
    real_bits = [0] * size
    i = 0
    while i < n:
        a = mini_tree.decode_symbol(br)
        if a > 2:
            real_bits[i] = a - 2
            i += 1
            continue
        if a == 0:
            real_bits[i] = 0
            i += 1
            continue
        if a == 1:
            run = br.getbits(4) + 3
        else:
            run = br.getbits(9) + 0x14
        while run > 0 and i < size:
            real_bits[i] = 0
            i += 1
            run -= 1

    tree = HuffTree(real_bits)
    return tree, real_bits

class Token:
    def __init__(self, kind: str, uncompressed_offset: int, **kwargs):
        self.kind = kind
        self.offset = uncompressed_offset
        self.kwargs = kwargs

    def __repr__(self):
        if self.kind == "LIT":
            c = self.kwargs["val"]
            ch = repr(chr(c)) if 32 <= c <= 126 else f"\\x{c:02x}"
            return f"@{self.offset:6d} LIT {c:3d} ({ch})"
        elif self.kind == "MATCH":
            return f"@{self.offset:6d} MATCH dist={self.kwargs['dist']:5d} len={self.kwargs['len']:3d} (slot={self.kwargs['slot']}, extra={self.kwargs['extra']})"
        elif self.kind == "NEWTABLE":
            return f"@{self.offset:6d} NEWTABLE"
        elif self.kind == "END":
            return f"@{self.offset:6d} END"
        return f"@{self.offset:6d} {self.kind} {self.kwargs}"

    def key(self):
        if self.kind == "LIT":
            return ("LIT", self.kwargs["val"])
        if self.kind == "MATCH":
            return ("MATCH", self.kwargs["dist"], self.kwargs["len"])
        return (self.kind,)

def trace_stream(raw_payload: bytes, verbose_tables=False):
    plain = unxor_gyik(raw_payload)
    br = BitReader(plain)
    lead_bit = br.getbits(1)
    
    tokens = []
    block_idx = 0
    uncomp_offset = 0

    while True:
        lit_tree, lit_lens = read_huff_table(br, 0x110)
        len_tree, len_lens = read_huff_table(br, 0xFE)
        block_idx += 1
        if verbose_tables:
            print(f"--- Block {block_idx} Tables ---")
            non_zero_lit = {i: l for i, l in enumerate(lit_lens) if l > 0}
            non_zero_len = {i: l for i, l in enumerate(len_lens) if l > 0}
            print(f"  Lit table non-zeros ({len(non_zero_lit)}): {non_zero_lit}")
            print(f"  Len table non-zeros ({len(non_zero_len)}): {non_zero_len}")

        while True:
            sym = lit_tree.decode_symbol(br)
            if sym < 0x100:
                tokens.append(Token("LIT", uncomp_offset, val=sym))
                uncomp_offset += 1
                continue

            dist = sym - 0x100
            extra = 0
            slot = dist
            if dist > 1:
                n = dist - 1
                extra = br.getbits(n)
                if extra == 0x3fff:
                    cmd = br.getbits(1)
                    if cmd == 0:
                        tokens.append(Token("NEWTABLE", uncomp_offset))
                        break
                    else:
                        tokens.append(Token("END", uncomp_offset))
                        return tokens
                dist = (1 << n) | extra
            dist += 1
            len_sym = len_tree.decode_symbol(br)
            length = 3 + len_sym
            tokens.append(Token("MATCH", uncomp_offset, dist=dist, len=length, slot=slot, extra=extra, len_sym=len_sym))
            uncomp_offset += length

def extract_payload(esp_path: str) -> bytes:
    with open(esp_path, "rb") as f:
        data = f.read()
    if data[:4] != b"ESP>":
        raise ValueError(f"{esp_path} is not an ESP archive")
    dir_pos = struct.unpack("<I", data[6:10])[0]
    return data[10:dir_pos]

def compare_tokens(t1: list[Token], t2: list[Token], label1="Orig", label2="New", max_diffs=30):
    print(f"Total tokens: {label1}={len(t1)}, {label2}={len(t2)}")
    diffs = 0
    limit = min(len(t1), len(t2))
    for i in range(limit):
        k1 = t1[i].key()
        k2 = t2[i].key()
        if k1 != k2 or t1[i].offset != t2[i].offset:
            print(f"Diff #{diffs+1} at index {i}:")
            print(f"  {label1}: {t1[i]}")
            print(f"  {label2}: {t2[i]}")
            diffs += 1
            if diffs >= max_diffs:
                print(f"... stopping after {max_diffs} diffs.")
                break
    if diffs == 0 and len(t1) == len(t2):
        print("ALL TOKENS MATCH PERFECTLY!")
    elif diffs == 0:
        print(f"Common prefix of {limit} tokens matched, but lengths differ.")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: esp_trace.py <archive.esp> [archive2.esp]")
        sys.exit(1)

    p1 = extract_payload(sys.argv[1])
    tokens1 = trace_stream(p1)

    if len(sys.argv) >= 3:
        p2 = extract_payload(sys.argv[2])
        tokens2 = trace_stream(p2)
        compare_tokens(tokens1, tokens2, sys.argv[1], sys.argv[2])
    else:
        for t in tokens1:
            print(t)
