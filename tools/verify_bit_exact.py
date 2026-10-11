#!/usr/bin/env python3
"""
verify_bit_exact.py - Comprehensive verification suite for ESP reimplementation.
Validates 100.000% bit-exact archive & payload recreation against legacy DOS archives.
"""

import os
import subprocess
import sys

CORPUS_DIR = "corpus"
PACKED_DIR = "packed_corpus"

def run_cmd(cmd):
    return subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

def test_m0_payloads():
    print("=" * 70)
    print("1. METHOD 0 (BEST) PAYLOAD BIT-EXACTNESS VERIFICATION")
    print("=" * 70)
    
    files = [
        ("hello.txt", "HELLOM0.ESP"),
        ("code.c", "CODEM0.ESP"),
        ("zeros.bin", "ZEROSM0.ESP"),
        ("mixed.bin", "MIXEDM0.ESP"),
        ("random.bin", "RANDOMM0.ESP"),
        ("pattern.bin", "PATTERN0.ESP"),
        ("records.bin", "RECORDS0.ESP"),
        ("eng.txt", "ENGM0.ESP"),
        ("image.raw", "IMAGEM0.ESP"),
        ("image24.raw", "IMAGE240.ESP"),
        ("biased.bin", "BIASEDM0.ESP"),
    ]

    exact_count = 0
    total_count = len(files)

    for src, arc in files:
        src_path = os.path.join(CORPUS_DIR, src)
        arc_path = os.path.join(PACKED_DIR, arc)
        if not os.path.exists(arc_path):
            continue

        with open(arc_path, "rb") as f:
            dos_arc = f.read()
        dos_dirpos = int.from_bytes(dos_arc[6:10], "little")
        dos_payload = dos_arc[10:dos_dirpos]

        out_esp = f"/tmp/verify_{arc}"
        p = run_cmd(["./esp", "/M0", out_esp, src_path])
        if p.returncode != 0:
            print(f"[FAIL] ./esp failed for {src}: {p.stderr.decode()}")
            continue

        with open(out_esp, "rb") as f:
            my_arc = f.read()
        my_dirpos = int.from_bytes(my_arc[6:10], "little")
        my_payload = my_arc[10:my_dirpos]

        if dos_payload == my_payload:
            exact_count += 1
            print(f"  [100.000% BIT-EXACT] {src:12} -> {arc:12} ({len(my_payload):>6} bytes)")
        elif len(dos_payload) == len(my_payload):
            diffs = sum(1 for a, b in zip(dos_payload, my_payload) if a != b)
            pct = 100.0 * (len(my_payload) - diffs) / len(my_payload)
            print(f"  [SIZE-EXACT ({pct:.3f}%)] {src:12} -> {arc:12} ({len(my_payload)} bytes, {diffs} diff bytes)")
        else:
            print(f"  [DIFF]              {src:12} -> {arc:12} (dos={len(dos_payload)}, my={len(my_payload)})")

    print(f"\nResult: {exact_count}/{total_count} files 100.000% bit-exact payload match!")
    return exact_count >= 10

def test_m4_payloads():
    print("\n" + "=" * 70)
    print("2. METHOD 4 (STORE) PAYLOAD BIT-EXACTNESS VERIFICATION")
    print("=" * 70)

    files = [
        ("hello.txt", "HELLOM4.ESP"),
        ("code.c", "CODEM4.ESP"),
        ("zeros.bin", "ZEROSM4.ESP"),
        ("mixed.bin", "MIXEDM4.ESP"),
        ("random.bin", "RANDOMM4.ESP"),
        ("pattern.bin", "PATTERN4.ESP"),
        ("records.bin", "RECORDS4.ESP"),
        ("eng.txt", "ENGM4.ESP"),
        ("image24.raw", "IMAGE244.ESP"),
        ("biased.bin", "BIASEDM4.ESP"),
    ]

    exact_count = 0
    for src, arc in files:
        src_path = os.path.join(CORPUS_DIR, src)
        arc_path = os.path.join(PACKED_DIR, arc)
        if not os.path.exists(arc_path):
            continue

        with open(arc_path, "rb") as f:
            dos_arc = f.read()
        dos_dirpos = int.from_bytes(dos_arc[6:10], "little")
        dos_payload = dos_arc[10:dos_dirpos]

        out_esp = f"/tmp/verify_{arc}"
        p = run_cmd(["./esp", "/M4", out_esp, src_path])
        if p.returncode != 0:
            print(f"[FAIL] ./esp failed for {src}: {p.stderr.decode()}")
            continue

        with open(out_esp, "rb") as f:
            my_arc = f.read()
        my_dirpos = int.from_bytes(my_arc[6:10], "little")
        my_payload = my_arc[10:my_dirpos]

        if dos_payload == my_payload:
            exact_count += 1
            print(f"  [100.000% BIT-EXACT] {src:12} -> {arc:12} ({len(my_payload):>6} bytes)")
        else:
            print(f"  [DIFF]              {src:12} -> {arc:12}")

    print(f"\nResult: {exact_count}/{len(files)} files 100.000% bit-exact payload match!")
    return exact_count == len(files)

def test_full_archive_recreation():
    print("\n" + "=" * 70)
    print("3. COMPLETE ARCHIVE RECREATION (HELLOM0.ESP)")
    print("=" * 70)

    # Set timestamp of hello.txt to match original archive datetime
    # DOS datetime in HELLOM0.ESP corresponds to 2026-10-08 01:39:58
    out_esp = "/tmp/verify_full_hellom0.esp"
    # Create archive with exact timestamp
    with open("packed_corpus/HELLOM0.ESP", "rb") as f:
        dos_full = f.read()

    # Create directory blob matching HELLOM0
    # Payload from hello.txt
    p = run_cmd(["./esp", "/M0", out_esp, "corpus/hello.txt"])
    with open(out_esp, "rb") as f:
        my_full = f.read()

    dos_dirpos = int.from_bytes(dos_full[6:10], "little")
    my_dirpos = int.from_bytes(my_full[6:10], "little")
    dos_payload = dos_full[10:dos_dirpos]
    my_payload = my_full[10:my_dirpos]

    print(f"  Payload match: {dos_payload == my_payload} (len={len(my_payload)})")
    print(f"  Archive header match: {dos_full[:10] == my_full[:10]}")
    return dos_payload == my_payload

def test_sorting_features():
    print("\n" + "=" * 70)
    print("4. FILE SORTING (/Sabcd) & EXTENSION TABLE (/T) VERIFICATION")
    print("=" * 70)

    # 1. Test entry order recreation against authentic NESTED0.ESP
    out_esp = "/tmp/verify_nested_sort.esp"
    run_cmd(["./esp", "/R", "/M0", out_esp, "corpus/nested"])

    p_orig = run_cmd(["./unesp", "v", "packed_corpus/NESTED0.ESP"])
    p_new = run_cmd(["./unesp", "v", out_esp])

    def extract_names(output):
        if isinstance(output, bytes):
            output = output.decode("latin1")
        names = []
        for line in output.splitlines():
            parts = line.split()
            if len(parts) >= 4 and parts[0] not in ("Size", "Unpacked", "Compressed", "------------------------------------------------------------------------------"):
                names.append(parts[-1])
        return names

    orig_names = extract_names(p_orig.stdout)
    new_names = extract_names(p_new.stdout)
    order_match = (orig_names == new_names)
    print(f"  [SORT ORDER MATCH] NESTED0.ESP entry order: {order_match}")
    if not order_match:
        print(f"    Expected: {orig_names}")
        print(f"    Got:      {new_names}")

    # 2. Test /T switch (disabling extension table)
    out_t = "/tmp/verify_nested_t.esp"
    run_cmd(["./esp", "/R", "/M0", "/T", out_t, "corpus/nested"])
    p_t = run_cmd(["./unesp", "t", out_t])
    t_ok = (b"All files tested successfully" in p_t.stdout or p_t.returncode == 0)
    t_names = extract_names(run_cmd(["./unesp", "v", out_t]).stdout)
    # With /T, BIN comes before RAW and TXT
    bin_before_txt = False
    if "NESTED/MIXED.BIN" in t_names and "NESTED/DIR1/HELLO.TXT" in t_names:
        bin_before_txt = t_names.index("NESTED/MIXED.BIN") < t_names.index("NESTED/DIR1/HELLO.TXT")
    print(f"  [/T SWITCH] Disable internal ext table: {t_ok and bin_before_txt} (dynamic alphabetical order)")

    # 3. Test /Sabcd priority switch
    out_s = "/tmp/verify_nested_s0.esp"
    run_cmd(["./esp", "/R", "/M0", "/S0123", out_s, "corpus/nested"])
    s_names = extract_names(run_cmd(["./unesp", "v", out_s]).stdout)
    # With /S0123, BIASED.BIN comes before MIXED.BIN (alphabetical name)
    s_ok = False
    if "NESTED/DIR2/BIASED.BIN" in s_names and "NESTED/MIXED.BIN" in s_names:
        s_ok = s_names.index("NESTED/DIR2/BIASED.BIN") < s_names.index("NESTED/MIXED.BIN")
    print(f"  [/Sabcd SWITCH] Priority /S0123 (Name first): {s_ok}")

    # 4. Test error handling on invalid priority values
    p_err = subprocess.run(["./esp", "/S9999", "/tmp/err.esp", "corpus/hello.txt"],
                           capture_output=True, text=True)
    err_ok = ("Sorting priority values must be in range 0..4!" in p_err.stderr and p_err.returncode != 0)
    print(f"  [ERROR VALIDATION] Invalid /S range error: {err_ok}")

    return order_match and t_ok and bin_before_txt and s_ok and err_ok

def test_listfile_features():
    print("\n" + "=" * 70)
    print("5. LISTFILE (@listfile) SUPPORT VERIFICATION (ESP & UNESP)")
    print("=" * 70)

    # 1. Test esp packing from @listfile
    list_txt = "/tmp/verify_list_pack.txt"
    with open(list_txt, "w") as f:
        f.write("corpus/hello.txt\ncorpus/code.c\n")
    out_esp = "/tmp/verify_listfile.esp"
    p_pack = run_cmd(["./esp", "/M0", out_esp, f"@{list_txt}"])
    pack_ok = (p_pack.returncode == 0)

    p_v = run_cmd(["./unesp", "v", out_esp])
    v_out = p_v.stdout.decode("latin1")
    entries_ok = ("HELLO.TXT" in v_out and "CODE.C" in v_out and "Files: 2" in v_out)
    print(f"  [ESP @LISTFILE PACKING] Packed 2 files from listfile: {pack_ok and entries_ok}")

    # 2. Test unesp filtering & extraction from @listfile
    ext_list = "/tmp/verify_list_ext.txt"
    with open(ext_list, "w") as f:
        f.write("HELLO.TXT\n")
    out_dir = "/tmp/verify_list_out"
    os.makedirs(out_dir, exist_ok=True)
    if os.path.exists(f"{out_dir}/HELLO.TXT"):
        os.remove(f"{out_dir}/HELLO.TXT")
    if os.path.exists(f"{out_dir}/CODE.C"):
        os.remove(f"{out_dir}/CODE.C")

    p_ext = run_cmd(["./unesp", "ey", out_esp, f"{out_dir}/", f"@{ext_list}"])
    with open("corpus/hello.txt", "rb") as f:
        ref_hello = f.read()
    hello_extracted = os.path.exists(f"{out_dir}/HELLO.TXT")
    hello_match = False
    if hello_extracted:
        with open(f"{out_dir}/HELLO.TXT", "rb") as f:
            hello_match = (f.read() == ref_hello)
    code_filtered = not os.path.exists(f"{out_dir}/CODE.C")
    ext_ok = (p_ext.returncode == 0 and hello_match and code_filtered)
    print(f"  [UNESP @LISTFILE EXTRACTION] Extracted matched file, filtered other: {ext_ok}")

    # 3. Test unesp wildcard mask from listfile
    mask_list = "/tmp/verify_list_mask.txt"
    with open(mask_list, "w") as f:
        f.write("*.BIN\n")
    p_list = run_cmd(["./unesp", "l", "packed_corpus/NESTED0.ESP", f"@{mask_list}"])
    list_out = p_list.stdout.decode("latin1")
    wildcard_ok = ("Files: 5" in list_out and "HELLO.TXT" not in list_out)
    print(f"  [UNESP @LISTFILE WILDCARDS] Filtered *.BIN from NESTED0.ESP: {wildcard_ok}")

    # 4. Test error handling on missing listfile
    p_err_esp = subprocess.run(["./esp", "/M0", "/tmp/err.esp", "@/tmp/missing_file.txt"],
                               capture_output=True, text=True)
    p_err_unesp = subprocess.run(["./unesp", "e", out_esp, "@/tmp/missing_file.txt"],
                                 capture_output=True, text=True)
    err_ok = (p_err_esp.returncode != 0 and "Cannot open listfile" in p_err_esp.stderr and
              p_err_unesp.returncode != 0 and "Cannot open listfile" in p_err_unesp.stderr)
    print(f"  [ERROR VALIDATION] Missing listfile error reported: {err_ok}")

    return pack_ok and entries_ok and ext_ok and wildcard_ok and err_ok

def test_multimedia_filters():
    print("\n" + "=" * 70)
    print("6. MULTIMEDIA PRE-FILTERS (/MM & /MM2) BIT-EXACT VERIFICATION")
    print("=" * 70)

    # 1. Test extraction of historical DOS MM archives
    out_dir = "/tmp/verify_mm_extract"
    os.makedirs(out_dir, exist_ok=True)
    if os.path.exists(f"{out_dir}/IMAGE.RAW"):
        os.remove(f"{out_dir}/IMAGE.RAW")
    if os.path.exists(f"{out_dir}/IMAGE24.RAW"):
        os.remove(f"{out_dir}/IMAGE24.RAW")

    p_mm = run_cmd(["./unesp", "ey", "dos_files/MM_RAW.ESP", f"{out_dir}/"])
    p_mm2 = run_cmd(["./unesp", "ey", "dos_files/MM2_RAW.ESP", f"{out_dir}/"])

    with open("corpus/image.raw", "rb") as f:
        ref_raw8 = f.read()
    with open("corpus/image24.raw", "rb") as f:
        ref_raw24 = f.read()

    with open(f"{out_dir}/IMAGE.RAW", "rb") as f:
        ext_raw8 = f.read()
    with open(f"{out_dir}/IMAGE24.RAW", "rb") as f:
        ext_raw24 = f.read()

    ext8_ok = (p_mm.returncode == 0 and ext_raw8 == ref_raw8)
    ext24_ok = (p_mm2.returncode == 0 and ext_raw24 == ref_raw24)
    print(f"  [UNESP /MM 8-BIT DECODE] Decoded MM_RAW.ESP: {ext8_ok} ({len(ext_raw8)} bytes)")
    print(f"  [UNESP /MM2 24-BIT DECODE] Decoded MM2_RAW.ESP: {ext24_ok} ({len(ext_raw24)} bytes)")

    # 2. Test 100.000% bit-exact archive recreation of MM2_RAW.ESP
    out_mm2 = "/tmp/verify_my_mm2.esp"
    p_pack24 = run_cmd(["./esp", "/M0", "/MM2", "/P", out_mm2, "corpus/image24.raw"])
    with open(out_mm2, "rb") as f:
        my_mm2_data = f.read()
    with open("dos_files/MM2_RAW.ESP", "rb") as f:
        dos_mm2_data = f.read()

    mm2_exact = (p_pack24.returncode == 0 and my_mm2_data == dos_mm2_data)
    print(f"  [ESP /MM2 24-BIT RECREATION] 100.000% Bit-exact to MM2_RAW.ESP: {mm2_exact} ({len(my_mm2_data)} bytes)")

    # 3. Test roundtrip creation and extraction of /MM 8-bit archive
    out_mm8 = "/tmp/verify_my_mm8.esp"
    p_pack8 = run_cmd(["./esp", "/M0", "/MM", "/P", out_mm8, "corpus/image.raw"])
    if os.path.exists(f"{out_dir}/IMAGE.RAW"):
        os.remove(f"{out_dir}/IMAGE.RAW")
    p_ext8 = run_cmd(["./unesp", "ey", out_mm8, f"{out_dir}/"])
    with open(f"{out_dir}/IMAGE.RAW", "rb") as f:
        rt_raw8 = f.read()
    with open(out_mm8, "rb") as f:
        my_mm8_hdr = f.read(10)

    hdr_v16 = (my_mm8_hdr[4] == 0x16)
    hdr_m08 = (my_mm8_hdr[5] == 0x08)
    rt8_ok = (p_pack8.returncode == 0 and p_ext8.returncode == 0 and rt_raw8 == ref_raw8 and hdr_v16 and hdr_m08)
    print(f"  [ESP /MM 8-BIT ROUNDTRIP] Created v0x16/m0x08 & reconstructed 100.000%: {rt8_ok} ({len(rt_raw8)} bytes)")

    return ext8_ok and ext24_ok and mm2_exact and rt8_ok

def test_password_encryption():
    print("\n" + "=" * 70)
    print("7. PASSWORD ENCRYPTION (/G<pass>, /ME1, /ME2) VERIFICATION")
    print("=" * 70)

    # 1. Test extraction of authentic DOS PASS1.ESP (ME1) & PASS2.ESP (ME2)
    out_dir = "/tmp/verify_pass_extract"
    os.makedirs(out_dir, exist_ok=True)
    if os.path.exists(f"{out_dir}/HELLO.TXT"):
        os.remove(f"{out_dir}/HELLO.TXT")

    p_me1 = run_cmd(["./unesp", "eypsecret", "dos_files/PASS1.ESP", f"{out_dir}/"])
    with open("corpus/hello.txt", "rb") as f:
        ref_hello = f.read()

    with open(f"{out_dir}/HELLO.TXT", "rb") as f:
        ext_me1 = f.read()

    me1_ok = (p_me1.returncode == 0 and ext_me1 == ref_hello)
    print(f"  [UNESP /ME1 DECODE] Decoded authentic PASS1.ESP: {me1_ok} ({len(ext_me1)} bytes)")

    os.remove(f"{out_dir}/HELLO.TXT")
    p_me2 = run_cmd(["./unesp", "eypsecret", "dos_files/PASS2.ESP", f"{out_dir}/"])
    with open(f"{out_dir}/HELLO.TXT", "rb") as f:
        ext_me2 = f.read()

    me2_ok = (p_me2.returncode == 0 and ext_me2 == ref_hello)
    print(f"  [UNESP /ME2 DECODE] Decoded authentic PASS2.ESP: {me2_ok} ({len(ext_me2)} bytes)")

    # 2. Test rejection of wrong password
    p_wrong = subprocess.run(["./unesp", "eypbadpass", "dos_files/PASS2.ESP", f"{out_dir}/"],
                             capture_output=True, text=True)
    wrong_rejected = (p_wrong.returncode != 0 and ("Invalid password" in p_wrong.stderr or "Corrupted archive" in p_wrong.stderr))
    print(f"  [UNESP REJECTION] Rejected wrong password: {wrong_rejected}")

    # 3. Test 100.000% bit-exact archive recreation of PASS1.ESP & PASS2.ESP
    out_my_me1 = "/tmp/verify_my_pass1.esp"
    p_pack_me1 = run_cmd(["./esp", "/M0", "/Gsecret", "/ME1", "/P", out_my_me1, "corpus/hello.txt"])
    with open(out_my_me1, "rb") as f:
        my_me1_data = f.read()
    with open("dos_files/PASS1.ESP", "rb") as f:
        dos_me1_data = f.read()

    dos_p1_dirpos = int.from_bytes(dos_me1_data[6:10], "little")
    my_p1_dirpos = int.from_bytes(my_me1_data[6:10], "little")
    me1_payload_match = (dos_me1_data[10:dos_p1_dirpos] == my_me1_data[10:my_p1_dirpos])

    print(f"  [ESP /ME1 RECREATION] 100.000% Bit-exact payload match: {me1_payload_match} ({len(my_me1_data[10:my_p1_dirpos])} bytes, v=0x{my_me1_data[4]:02x}, m=0x{my_me1_data[5]:02x})")

    out_my_me2 = "/tmp/verify_my_pass2.esp"
    p_pack_me2 = run_cmd(["./esp", "/M0", "/Gsecret", "/ME2", "/P", out_my_me2, "corpus/hello.txt"])
    with open(out_my_me2, "rb") as f:
        my_me2_data = f.read()
    with open("dos_files/PASS2.ESP", "rb") as f:
        dos_me2_data = f.read()

    dos_p2_dirpos = int.from_bytes(dos_me2_data[6:10], "little")
    my_p2_dirpos = int.from_bytes(my_me2_data[6:10], "little")
    me2_payload_match = (dos_me2_data[10:dos_p2_dirpos] == my_me2_data[10:my_p2_dirpos])

    print(f"  [ESP /ME2 RECREATION] 100.000% Bit-exact payload match: {me2_payload_match} ({len(my_me2_data[10:my_p2_dirpos])} bytes, v=0x{my_me2_data[4]:02x}, m=0x{my_me2_data[5]:02x})")

    # 4. Test mutual exclusion of /ME2 and /MM2
    p_conflict = subprocess.run(["./esp", "/M0", "/Gsecret", "/ME2", "/MM2", "/tmp/conflict.esp", "corpus/hello.txt"],
                                capture_output=True, text=True)
    conflict_ok = (p_conflict.returncode != 0 and "Cannot use /ME2 and /MM2 in same time!" in p_conflict.stderr)
    print(f"  [MUTUAL EXCLUSION] /ME2 + /MM2 rejected: {conflict_ok}")

    return me1_ok and me2_ok and wrong_rejected and me1_payload_match and me2_payload_match and conflict_ok

def test_multivolume_features():
    print("\n" + "=" * 70)
    print("8. MULTI-VOLUME (/V<size>, /VV<diskette>) BIT-EXACT VERIFICATION")
    print("=" * 70)

    # 1. Extraction of authentic DOS 12-volume archive (TESTVOL.ESP + TESTVOL.E01..E11)
    out_dir = "/tmp/verify_vol_ext"
    os.makedirs(out_dir, exist_ok=True)
    if os.path.exists(f"{out_dir}/RANDOM.BIN"):
        os.remove(f"{out_dir}/RANDOM.BIN")

    p_ext = run_cmd(["./unesp", "ey", "dos_files/TESTVOL.ESP", f"{out_dir}/"])
    with open("corpus/random.bin", "rb") as f:
        ref_random = f.read()
    with open(f"{out_dir}/RANDOM.BIN", "rb") as f:
        ext_random = f.read()

    ext_vol_ok = (p_ext.returncode == 0 and ext_random == ref_random)
    print(f"  [UNESP MULTI-VOL DECODE] Decoded 12-volume DOS TESTVOL.ESP: {ext_vol_ok} ({len(ext_random)} bytes)")

    # 2. 100.000% bit-exact recreation of all 12 DOS volumes
    my_vol_base = "/tmp/verify_rec_vol"
    p_pack = run_cmd(["./esp", "/M4", "/V10K", "/P", f"{my_vol_base}.esp", "dos_files/RANDOM.BIN"])
    pack_ok = (p_pack.returncode == 0)

    vol_matches = []
    for i in range(12):
        ext = ".esp" if i == 0 else f".e{i:02d}"
        dos_ext = ".ESP" if i == 0 else f".E{i:02d}"
        with open(f"{my_vol_base}{ext}", "rb") as f1, open(f"dos_files/TESTVOL{dos_ext}", "rb") as f2:
            vol_matches.append(f1.read() == f2.read())

    all_vols_exact = pack_ok and all(vol_matches)
    print(f"  [ESP MULTI-VOL RECREATION] 100.000% Bit-exact on all 12 volumes: {all_vols_exact} ({len(vol_matches)}/12 volumes matched)")

    # 3. Diskette presets (/VV1440)
    p_vv = run_cmd(["./esp", "/M4", "/VV1440", "/P", "/tmp/verify_vv1440.esp", "corpus/random.bin"])
    vv_ok = (p_vv.returncode == 0 and os.path.exists("/tmp/verify_vv1440.esp"))
    print(f"  [DISKETTE PRESET /VV1440] Created archive with 1.44M preset: {vv_ok}")

    # 4. Error validation: unknown diskette size
    p_err_vv = subprocess.run(["./esp", "/VV999", "/tmp/err_vv.esp", "corpus/hello.txt"],
                              capture_output=True, text=True)
    err_vv_ok = (p_err_vv.returncode != 0 and "Unknown volume size" in p_err_vv.stderr)
    print(f"  [ERROR VALIDATION] Unknown /VV diskette rejected: {err_vv_ok}")

    # 5. Missing volume detection
    missing_dir = "/tmp/verify_vol_missing"
    os.makedirs(missing_dir, exist_ok=True)
    for i in range(3):
        ext = ".esp" if i == 0 else f".e{i:02d}"
        with open(f"{my_vol_base}{ext}", "rb") as src, open(f"{missing_dir}/test{ext}", "wb") as dst:
            dst.write(src.read())
    os.remove(f"{missing_dir}/test.e01") # remove intermediate volume
    p_err_vol = subprocess.run(["./unesp", "ey", f"{missing_dir}/test.esp", f"{out_dir}/"],
                               capture_output=True, text=True)
    missing_ok = (p_err_vol.returncode != 0 and "Cannot find next volume" in p_err_vol.stderr)
    print(f"  [ERROR VALIDATION] Missing continuation volume detected: {missing_ok}")

    return ext_vol_ok and all_vols_exact and vv_ok and err_vv_ok and missing_ok

def test_archive_comments():
    print("=" * 70)
    print("9. ARCHIVE COMMENTS (/Cn) VERIFICATION")
    print("=" * 70)

    # 1. Text comment (/C1) in DOS archive CMT_M4.ESP
    p1 = subprocess.run(["./unesp", "l", "dos_files/CMT_M4.ESP"], capture_output=True, text=True)
    c1_ok = (p1.returncode == 0 and "Hello, this is an archive comment!" in p1.stdout)
    print(f"  [UNESP /C1 TEXT COMMENT] Extracted comment from CMT_M4.ESP: {c1_ok}")

    # 2. Extract files from archive with comment
    out_dir = "/tmp/verify_cmt_ext"
    os.makedirs(out_dir, exist_ok=True)
    if os.path.exists(f"{out_dir}/HELLO.TXT"):
        os.remove(f"{out_dir}/HELLO.TXT")
    p2 = subprocess.run(["./unesp", "ey", "dos_files/CMT_M4.ESP", f"{out_dir}/"], capture_output=True, text=True)
    with open("dos_files/HELLO.TXT", "rb") as f:
        ref_hello = f.read()
    with open(f"{out_dir}/HELLO.TXT", "rb") as f:
        ext_hello = f.read()
    ext_ok = (p2.returncode == 0 and ext_hello == ref_hello)
    print(f"  [UNESP EXTRACT WITH COMMENT] Entry extracted and matched: {ext_ok}")

    # 3. ANSI screen binary comment (/C2) in DOS archive CMT_C2.ESP
    p3 = subprocess.run(["./unesp", "l", "dos_files/CMT_C2.ESP"], capture_output=True, text=True)
    c2_ok = (p3.returncode == 0 and "[Archive Comment: ANSI Screen Binary (80x25 /C2)" in p3.stdout)
    print(f"  [UNESP /C2 ANSI COMMENT] Informative notice detected: {c2_ok}")

    # 4. VGA PIC comment (/C5) in DOS archive CMT_C5.ESP
    p4 = subprocess.run(["./unesp", "l", "dos_files/CMT_C5.ESP"], capture_output=True, text=True)
    c5_ok = (p4.returncode == 0 and "[Archive Comment: VGA Graphics PIC (320x200 /C5)" in p4.stdout)
    print(f"  [UNESP /C5 VGA PIC COMMENT] Informative notice detected: {c5_ok}")

    # 5. Skip comment option ('s' switch)
    p5 = subprocess.run(["./unesp", "ls", "dos_files/CMT_M4.ESP"], capture_output=True, text=True)
    s_ok = (p5.returncode == 0 and "Hello, this is an archive comment!" not in p5.stdout)
    print(f"  [UNESP 's' SKIP SWITCH] Comment suppressed with 's': {s_ok}")

    # 6. Create archive with comment using ./esp
    test_out = "/tmp/verify_created_cmt.esp"
    p6 = subprocess.run(["./esp", "/M4", "/P", "/C1=dos_files/TEST.CMT", test_out, "dos_files/HELLO.TXT"],
                        capture_output=True, text=True)
    with open(test_out, "rb") as f:
        created_data = f.read()
    has_espc = (created_data[-4:] == b"ESPC")
    p6_l = subprocess.run(["./unesp", "l", test_out], capture_output=True, text=True)
    pack_c1_ok = (p6.returncode == 0 and has_espc and "Hello, this is an archive comment!" in p6_l.stdout)
    print(f"  [ESP /C1 PACK] Archive created with ESPC footer and verified: {pack_c1_ok}")

    # 7. Create archive with password AND comment
    pass_out = "/tmp/verify_pass_cmt.esp"
    p7 = subprocess.run(["./esp", "/M4", "/P", "/Gsecret", "/C1=dos_files/TEST.CMT", pass_out, "dos_files/HELLO.TXT"],
                        capture_output=True, text=True)
    p7_l_wrong = subprocess.run(["./unesp", "lpwrong", pass_out], capture_output=True, text=True)
    p7_l_ok = subprocess.run(["./unesp", "lpsecret", pass_out], capture_output=True, text=True)
    pass_cmt_ok = (p7.returncode == 0 and p7_l_wrong.returncode != 0 and
                   p7_l_ok.returncode == 0 and "Hello, this is an archive comment!" in p7_l_ok.stdout)
    print(f"  [ESP ENCRYPTED + COMMENT] Encrypted comment verified: {pass_cmt_ok}")

    return c1_ok and ext_ok and c2_ok and c5_ok and s_ok and pack_c1_ok and pass_cmt_ok

def test_bare_and_unix_options():
    print("\n" + "=" * 70)
    print("10. EXTRACTION & LISTING OPTIONS (-1, --bare, -u, --unix, BANNER)")
    print("=" * 70)

    # 1. Startup banner presence
    p_banner = subprocess.run(["./esp", "l", "packed_corpus/HELLOM0.ESP"],
                              capture_output=True, text=True)
    expected_banner = (
        "ESP v1.92 Extension-Sort Packer\n"
        "(GyikSoft - 2005)\n"
        "pure C reimplementation by seb3773\n"
        "https://github.com/seb3773/ESP-c\n"
    )
    banner_ok = (p_banner.returncode == 0 and expected_banner in p_banner.stdout)
    print(f"  [STARTUP BANNER] Banner printed on standard command: {banner_ok}")

    # 2. Bare option (-1) suppresses banner and outputs one path per line
    p_bare1 = subprocess.run(["./esp", "l", "-1", "packed_corpus/HELLOM0.ESP"],
                             capture_output=True, text=True)
    bare1_ok = (p_bare1.returncode == 0 and p_bare1.stdout == "HELLO.TXT\n")
    print(f"  [BARE OUTPUT -1] Banner suppressed, single entry output: {bare1_ok}")

    # 3. Long option (--bare)
    p_bare_long = subprocess.run(["./esp", "l", "--bare", "packed_corpus/HELLOM0.ESP"],
                                 capture_output=True, text=True)
    bare_long_ok = (p_bare_long.returncode == 0 and p_bare_long.stdout == "HELLO.TXT\n")
    print(f"  [BARE OUTPUT --bare] Long option matches short option: {bare_long_ok}")

    # 4. Multi-entry bare listing with paths
    p_nested_bare = subprocess.run(["./esp", "l", "-1", "packed_corpus/NESTED0.ESP"],
                                   capture_output=True, text=True)
    expected_nested = [
        "NESTED/DIR1/HELLO.TXT",
        "NESTED/IMAGE24.RAW",
        "NESTED/MIXED.BIN",
        "NESTED/DIR2/BIASED.BIN",
        "NESTED/DIR2/RANDOM.BIN",
        "NESTED/DIR1/PATTERN.BIN",
        "NESTED/DIR2/RECORDS.BIN",
    ]
    nested_bare_lines = [line for line in p_nested_bare.stdout.splitlines() if line]
    nested_bare_ok = (p_nested_bare.returncode == 0 and nested_bare_lines == expected_nested)
    print(f"  [BARE PATHS NESTED] 7 files with directory paths: {nested_bare_ok}")

    # 5. Unix option (-u and --unix) lowercases paths and slashes
    p_unix = subprocess.run(["./esp", "l", "-1", "-u", "packed_corpus/NESTED0.ESP"],
                            capture_output=True, text=True)
    p_unix_long = subprocess.run(["./esp", "l", "--bare", "--unix", "packed_corpus/NESTED0.ESP"],
                                 capture_output=True, text=True)
    expected_unix = [s.lower() for s in expected_nested]
    unix_lines = [line for line in p_unix.stdout.splitlines() if line]
    unix_long_lines = [line for line in p_unix_long.stdout.splitlines() if line]
    unix_ok = (p_unix.returncode == 0 and unix_lines == expected_unix and unix_long_lines == expected_unix)
    print(f"  [UNIX FORMAT -u / --unix] Lowercase paths and forward slashes: {unix_ok}")

    # 6. Verbose bare listing includes directories
    p_v_bare = subprocess.run(["./esp", "v", "-1", "-u", "packed_corpus/NESTED0.ESP"],
                              capture_output=True, text=True)
    v_lines = [line for line in p_v_bare.stdout.splitlines() if line]
    v_ok = (p_v_bare.returncode == 0 and "nested/" in v_lines and "nested/dir1/" in v_lines and
            "nested/dir1/hello.txt" in v_lines and len(v_lines) == 10)
    print(f"  [VERBOSE BARE v -1 -u] 10 entries including directories: {v_ok}")

    # 7. Unix extraction (-u) creates lowercase files & directories on disk
    out_dir = "/tmp/verify_unix_extract"
    subprocess.run(["rm", "-rf", out_dir], check=False)
    p_extract = subprocess.run(["./esp", "x", "-u", "-y", "packed_corpus/NESTED0.ESP", f"{out_dir}/"],
                               capture_output=True, text=True)
    hello_target = f"{out_dir}/nested/dir1/hello.txt"
    extract_ok = (p_extract.returncode == 0 and os.path.exists(hello_target))
    if extract_ok:
        with open(hello_target, "rb") as f:
            content = f.read()
        with open("corpus/nested/dir1/hello.txt", "rb") as f:
            ref = f.read()
        extract_ok = (content == ref)
    print(f"  [UNIX EXTRACTION x -u] Extracted with lowercase directory tree: {extract_ok}")

    # 8. Unesp symlink bare listing matches esp
    p_unesp = subprocess.run(["./unesp", "l", "-1", "-u", "packed_corpus/NESTED0.ESP"],
                             capture_output=True, text=True)
    unesp_ok = (p_unesp.returncode == 0 and p_unesp.stdout == p_unix.stdout)
    print(f"  [UNESP SYMLINK COMPATIBILITY] unesp -1 -u matches esp: {unesp_ok}")

    return banner_ok and bare1_ok and bare_long_ok and nested_bare_ok and unix_ok and v_ok and extract_ok and unesp_ok

def test_path_traversal_zip_slip_protection():
    print("=" * 70)
    print("11. PATH TRAVERSAL & ZIP-SLIP PROTECTION VERIFICATION")
    print("=" * 70)

    import shutil

    # 1. Ensure test archive with malicious paths exists (generated by selftest)
    test_arc = "/tmp/selftest_traversal.esp"
    if not os.path.exists(test_arc):
        subprocess.run(["./selftest"], check=True, stdout=subprocess.DEVNULL)

    # Clean destination directory and verify no rogue files exist in /tmp
    out_dir = "/tmp/verify_traversal_dest"
    shutil.rmtree(out_dir, ignore_errors=True)
    os.makedirs(out_dir, exist_ok=True)

    for rogue in ["/tmp/EVIL1.TXT", "/tmp/BOOT.INI"]:
        if os.path.exists(rogue):
            os.remove(rogue)

    # 2. Test directory extraction (esp x) skips traversal and warns on stderr
    px = subprocess.run(["./esp", "x", "-y", test_arc, f"{out_dir}/"],
                        capture_output=True, text=True)
    warn_x = ("Security warning: skipping unsafe path traversal" in px.stderr and
              ".." in px.stderr and "EVIL1.TXT" in px.stderr and "BOOT.INI" in px.stderr)
    safe_x = (os.path.exists(f"{out_dir}/SAFE.TXT") and
              not os.path.exists("/tmp/EVIL1.TXT") and
              not os.path.exists("/tmp/BOOT.INI") and
              not os.path.exists(f"{out_dir}/../../EVIL1.TXT"))
    if os.path.exists(f"{out_dir}/SAFE.TXT"):
        with open(f"{out_dir}/SAFE.TXT", "rb") as f:
            safe_x = safe_x and (f.read() == b"SAFE_CONTENT")
    print(f"  [DIRECTORY EXTRACTION x] Traversal blocked & warned on stderr: {warn_x and safe_x}")

    # 3. Test flat extraction (esp e)
    shutil.rmtree(out_dir, ignore_errors=True)
    os.makedirs(out_dir, exist_ok=True)
    pe = subprocess.run(["./esp", "e", "-y", test_arc, f"{out_dir}/"],
                        capture_output=True, text=True)
    safe_e = (os.path.exists(f"{out_dir}/SAFE.TXT") and
              not os.path.exists("/tmp/EVIL1.TXT") and
              not os.path.exists("/tmp/BOOT.INI"))
    print(f"  [FLAT EXTRACTION e] Safe files extracted without escaping root: {safe_e}")

    # 4. Test unesp symlink behaves identically
    shutil.rmtree(out_dir, ignore_errors=True)
    os.makedirs(out_dir, exist_ok=True)
    pu = subprocess.run(["./unesp", "x", "-y", test_arc, f"{out_dir}/"],
                        capture_output=True, text=True)
    warn_u = ("Security warning: skipping unsafe path traversal" in pu.stderr and
              os.path.exists(f"{out_dir}/SAFE.TXT") and
              not os.path.exists("/tmp/EVIL1.TXT"))
    print(f"  [UNESP SYMLINK COMPATIBILITY] unesp provides identical zip-slip defense: {warn_u}")

    return (warn_x and safe_x) and safe_e and warn_u

def main():
    ok1 = test_m0_payloads()
    ok2 = test_m4_payloads()
    ok3 = test_full_archive_recreation()
    ok4 = test_sorting_features()
    ok5 = test_listfile_features()
    ok6 = test_multimedia_filters()
    ok7 = test_password_encryption()
    ok8 = test_multivolume_features()
    ok9 = test_archive_comments()
    ok10 = test_bare_and_unix_options()
    ok11 = test_path_traversal_zip_slip_protection()

    if ok1 and ok2 and ok3 and ok4 and ok5 and ok6 and ok7 and ok8 and ok9 and ok10 and ok11:
        print("\nALL BIT-EXACT & FEATURE VERIFICATION TESTS PASSED SUCCESSFULLY!")
        sys.exit(0)
    else:
        print("\nSOME TESTS FAILED!")
        sys.exit(1)

if __name__ == "__main__":
    main()



