#!/usr/bin/env python3
"""
sim_lzss.py - Precise simulation of the reverse-engineered GyikSoft LZSS engine.
"""

import sys

def simulate_engine(data: bytes, method=0, max_tokens=12288):
    # Setup parameters
    max_chain_map = {0: 256, 1: 128, 2: 32, 3: 5}
    max_chain = max_chain_map.get(method, 256)
    min_match = 3

    # Sliding window & tables
    # Buffer has 256 zeros prefix (positions 0x0000..0x00FF)
    # Input data starts at 0x0100
    # Window size: 32768
    buf = bytearray(b"\x00" * 256 + data)
    n = len(buf)

    head = [0x8100] * 4096
    prev = [0x8100] * 65536

    h = 0

    def hash_byte(curr_h, b):
        bl = ((curr_h << 3) + b) & 0xFF
        bh = ((curr_h << 3) >> 8) & 0x0F
        return (bh << 8) | bl

    def is_valid(bp, cand):
        # In x86: cmp bp, bx; js -> (int16_t)(bp - cand) < 0
        diff = (bp - cand) & 0xFFFF
        return diff < 0x8000 and cand != 0x8100

    def get_word(pos):
        if pos + 1 < n:
            return buf[pos] | (buf[pos + 1] << 8)
        elif pos < n:
            return buf[pos]
        return 0

    def find_match_64c(bp, cur_pos):
        best_len = 1
        best_pos = 0
        cand = prev[cur_pos]
        chain = max_chain
        cur_w = get_word(cur_pos)

        # 0x668
        while chain > 0 and is_valid(bp, cand):
            chain -= 1
            if best_len <= 1:
                # test first 2 bytes
                cand_w = get_word(cand)
                if cand_w == cur_w:
                    # jump 0x698
                    # repe cmpsw
                    ml = 0
                    while cur_pos + ml < n and cand + ml < n and ml < 256 and buf[cur_pos + ml] == buf[cand + ml]:
                        ml += 1
                    if ml > best_len:
                        best_len = ml
                        best_pos = cand
                        # Hop check at 0x704
                        res = hop_check(bp, cur_pos, best_len, best_pos, chain)
                        if res is not None:
                            return res
                    # else continue
            else:
                # best_len > 1
                # test candidate at best_len
                # 0x66B: cmp [bx + si], ax
                if get_word(cand + best_len) == get_word(cur_pos + best_len):
                    # jump 0x698
                    ml = 0
                    while cur_pos + ml < n and cand + ml < n and ml < 256 and buf[cur_pos + ml] == buf[cand + ml]:
                        ml += 1
                    if ml > best_len:
                        best_len = ml
                        best_pos = cand
                        res = hop_check(bp, cur_pos, best_len, best_pos, chain)
                        if res is not None:
                            return res

            cand = prev[cand]

        return best_len, best_pos

    def hop_check(bp, cur_pos, best_len, best_pos, chain):
        # 0x704:
        target = cur_pos + best_len
        cx = prev[target] if target < len(prev) else 0x8100
        if is_valid(bp, cx):
            # cmp cx, bx; js 0x744 -> cx < best_pos
            diff = (cx - best_pos) & 0xFFFF
            if diff >= 0x8000: # cx < best_pos
                # jump 0x744 with ax = 2
                return suffix_chain(bp, cur_pos, best_len + 1, cx, 2, chain)
        # 0x720: ax = 3
        cx2 = prev[best_pos + best_len - 2] if (best_pos + best_len - 2) < len(prev) else 0x8100
        nxt = prev[best_pos]
        if is_valid(bp, cx2) and is_valid(bp, nxt):
            diff = (cx2 - nxt) & 0xFFFF
            if diff >= 0x8000: # cx2 < nxt
                return suffix_chain(bp, cur_pos, best_len + 1, cx2, 3, chain)
        return None

    def suffix_chain(bp, cur_pos, best_len, bx, ax, chain):
        # 0x744:
        # si = ax - L - 1 where L was best_len before inc?
        # In asm: si = match_len + 1; sub si, ax; neg si;
        # so si = -( (best_len) - ax ) = ax - best_len.
        si = ax - best_len
        cur_w = get_word(cur_pos)

        # 0x758:
        while chain > 0 and is_valid(bp, bx):
            chain -= 1
            cand_prefix = bx + si
            if is_valid(bp, cand_prefix) and cand_prefix >= 0:
                if get_word(cand_prefix) == cur_w:
                    # 0x7A0: full compare
                    ml = 0
                    while cur_pos + ml < n and cand_prefix + ml < n and ml < 256 and buf[cur_pos + ml] == buf[cand_prefix + ml]:
                        ml += 1
                    if ml > best_len:
                        best_len = ml
                        best_pos = cand_prefix
                        # Try to hop again at 0x7E0:
                        target = cur_pos + best_len - 1
                        cx = prev[target] if target < len(prev) else 0x8100
                        if is_valid(bp, cx) and ((cx - bx) & 0xFFFF) >= 0x8000:
                            # 0x82C: ax = 2
                            return suffix_chain(bp, cur_pos, best_len, cx, 2, chain)
                        cx2 = prev[bx + (si + best_len) - 3] if 0 <= (bx + si + best_len - 3) < len(prev) else 0x8100
                        nxt = prev[bx]
                        if is_valid(bp, cx2) and is_valid(bp, nxt) and ((cx2 - nxt) & 0xFFFF) >= 0x8000:
                            # 0x840: ax = 3
                            return suffix_chain(bp, cur_pos, best_len, cx2, 3, chain)
            bx = prev[bx]
        return best_len, best_pos

    def find_match_5f4(bp, cur_pos, prev_len):
        if prev_len < 3:
            return find_match_64c(bp, cur_pos)
        # 0x5F9:
        # Check prev[] at end of previous match
        si = cur_pos - 1 + prev_len
        cand_list = []
        for i in range(prev_len - 1):
            p = prev[si - i] if (si - i) < len(prev) else 0x8100
            if is_valid(bp, p):
                dist = bp - p
                cand_list.append((dist, p, -(prev_len - 2 - i)))
        if not cand_list:
            return 1, 0
        cand_list.sort(key=lambda x: x[0], reverse=True)
        best_cand = cand_list[0]
        bx = best_cand[1]
        cx = best_cand[2]
        if cx == 0:
            return find_match_64c(bp, cur_pos)
        else:
            return suffix_chain(bp, cur_pos, prev_len, bx, cx + prev_len, max_chain)

    # Initial pre-fill
    prev[0xFF] = 0x80FF
    bp = 0x00FD
    for _ in range(0x103):
        bp += 1
        b = buf[bp + 2] if bp + 2 < n else 0
        h = hash_byte(h, b)
        old_h = head[h]
        head[h] = bp
        prev[bp] = old_h if bp >= old_h else (bp - 0x8000)

    # First match find at cur_pos = 0x100
    cur_pos = 0x100
    curr_len, curr_pos = find_match_64c(bp, cur_pos)
    bp += 1

    tokens = []
    # Main compression loop
    while cur_pos < n:
        # Hash byte at bp+2
        b = buf[bp + 2] if bp + 2 < n else 0
        h = hash_byte(h, b)
        old_h = head[h]
        head[h] = bp
        prev[bp] = old_h if bp >= old_h else (bp - 0x8000)

        prev_match_len = curr_len
        prev_match_pos = curr_pos

        # Find match at cur_pos + 1
        curr_len, curr_pos = find_match_5f4(bp, cur_pos + 1, prev_match_len)

        if prev_match_len < min_match:
            tokens.append(("LIT", buf[cur_pos]))
            cur_pos += 1
            bp += 1
        elif prev_match_len >= curr_len:
            # Emit match
            dist = cur_pos - prev_match_pos
            tokens.append(("MATCH", dist, prev_match_len))
            # Hash remaining bytes
            for _ in range(prev_match_len - 1):
                bp += 1
                b = buf[bp + 2] if bp + 2 < n else 0
                h = hash_byte(h, b)
                old_h = head[h]
                head[h] = bp
                prev[bp] = old_h if bp >= old_h else (bp - 0x8000)
            cur_pos += prev_match_len
            curr_len, curr_pos = find_match_64c(bp, cur_pos)
            bp += 1
        else:
            # Lazy eval: emit literal
            tokens.append(("LIT", buf[cur_pos]))
            cur_pos += 1
            bp += 1

    return tokens

if __name__ == "__main__":
    data = open("corpus/eng.txt", "rb").read()
    tokens = simulate_engine(data, method=0)
    print(f"Generated {len(tokens)} tokens")
    for t in tokens[:20]:
        print(t)
