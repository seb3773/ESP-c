# ESP (C reimplementation)

Pure-C decoder and encoder for the historical DOS **ESP** archiver (v1.6 / v1.92, magic `ESP>`) by GyikSoft / A'rpi (1995–1997).

The original Unix UnESP 0.1beta (1999) does not work on 64-bit platforms (because `dword` was typed as `unsigned long`, breaking the 16-bit Huffman Kraft inequality check `d != 65536`). This repository is a complete, modern C99/C11 implementation with standard libc only (no external dependencies), achieving **100% bit-exact archive and payload recreation** matching the historical DOS binary `ESP.EXE v1.92`.

## Build & Test

```bash
make clean
make test
python3 tools/verify_bit_exact.py
```

Produces a single unified standalone executable:
- `./esp` : Unified Archiver / Extractor / Verifier
- `./unesp` : Symlink to `./esp` for historical compatibility

---

## Usage

The unified `esp` executable supports both standard archiver commands (`a`, `x`, `e`, `l`, `v`, `t`) and historical DOS `ESP.EXE` syntax:

```bash
# General syntax
esp <command> [options] <archive[.esp]> [files... | dest/ | masks...] [@listfile]

# Historical compression syntax (default command is 'a' / add)
esp [options] <archive[.esp]> [files...] [@listfile]
```

### Commands
- `a`, `c` : **Add / create** archive (default if command letter omitted)
- `x` : **eXtract** files with full directory tree
- `e` : **Extract flat** (without directories) into current directory or destination `dest/`
- `l` : **List** archive contents
- `v` : **Verbose list** archive contents
- `t` : **Test** archive integrity

### Compression Options (Archiving)
- `/M0` : Best compression (default)
- `/M3` : Fast compression
- `/M4` : Store mode (uncompressed payload; directory remains LZSS-compressed)
- `/MM` : MultiMedia filter for 8-bit audio and 256-color images (delta filter)
- `/MM2` : MultiMedia filter for 24-bit images (Szymon Grabowski planar RGB filter)
- `/G<pass>`, `/P<pass>` : Encrypt archive with password (prompts if omitted)
- `/ME1` : Use legacy v1.6 password hashing algorithm (container version `0x16`)
- `/ME2` : Use modern v1.92+ password hashing algorithm (default when encrypted, container version `0x17`)
- `/Cn=file` : Add archive comment from file:
  - `/C=file` or `/C1=file` : Plain ASCII text comment
  - `/C2=file` : Color ANSI screen binary dump (80x25 memory dump)
  - `/C4=file` : Old PIC VGA graphics (320x200x256)
  - `/C5=file` : New PIC VGA graphics (320x200)
  - `/C6=file` : New PIC VGA graphics (640x480)
- `/V<size>` : Split archive into multiple volumes (e.g. `/V1440K`, `/V10M`, `/V100000B`)
- `/VV<disk>` : Slicing presets for standard DOS floppies (`/VV1440`, `/VV1200`, `/VV720`, `/VV360`)
- `/R` : Recurse subdirectories
- `/P` : Do not store paths (flatten archive)
- `/J` : Disable prefix-repeat redundancy mode (`repsize = 0`)
- `/Sabcd` : Sorting priority (default `/S1240` = Size, Path, Attrib, Name)
- `/T` : Disable internal 161-entry extension table (fall back to alphabetical extension grouping)

### General, Extraction & Listing Options
- `-1`, `--bare`, modifier `1` : Bare output format (one path per line, script/pipe friendly; suppresses banner and table headers)
- `-u`, `--unix`, modifier `u` : Format paths as Unix relative paths (`/` forward slashes, lowercase filenames and directories)
- `/Y`, `-y`, modifier `y` : Always overwrite / Yes to all prompts (e.g. `esp xy arc.esp`)
- `/S`, `-s`, modifier `s` : Skip archive comment display (e.g. `esp ls arc.esp`)
- `/P<pass>`, `/G<pass>`, modifier `p` : Supply password (e.g. `esp xpsecret arc.esp` or `esp x -psecret arc.esp`)
- `@listfile` : Read list of files to pack or filter masks to extract/list

---

## Features & Historical Innovations

### 1. Exceptional Multimedia Compression: Szymon Grabowski's Algorithm (`/MM2`)

Introduced in ESP v1.9 beta 7 (1997), `/MM2` is one of the most remarkable innovations of the ESP archiver. At a time when mainstream DOS archivers (PKZIP, ARJ, LHA) treated all uncompressed files as flat byte streams, ESP incorporated an advanced truecolor pre-filter designed by Szymon Grabowski.

#### The 98,304 -> 333 Bytes Phenomenon
On uncompressed truecolor images (BMP, TGA, TIFF, RAW), standard sliding-window LZSS suffers from severe limitations: because adjacent RGB triplets are interleaved ($R, G, B, R, G, B\dots$), continuous color gradients produce constantly varying numbers without identical substrings of length $\ge 3$. On `corpus/image24.raw` (98,304 bytes), pure LZSS produces a bloated **79,634 bytes** payload.

With `/MM2`, Szymon Grabowski's algorithm transforms the data before LZSS:
1. **Planar Decomposition**: Separates interleaved RGB bytes into 3 distinct contiguous planes ($N_0, N_1, N_2$).
2. **Spatial Delta**: Computes spatial derivatives along each plane: $\Delta_R = c_R(x) - c_R(x-1)$.
3. **Cross-Channel Differential**: Cancels mutual inter-channel correlation:
   $$\text{Plane } 0 = \Delta_R$$
   $$\text{Plane } 1 = \Delta_G - \Delta_R$$
   $$\text{Plane } 2 = \Delta_B - \Delta_G$$

On smooth color gradients, **99.9% of the bytes in all three planes evaluate to the exact constant byte `0x01`**. LZSS run-length matches this stream at distance 1 and length 256 over just ~380 tokens, reducing the payload to **275 bytes** and the complete archive to **333 bytes**!

Both DOS `ESP.EXE v1.92` and our reimplementation produce **byte-for-byte identical 333-byte archives** (`cmp` returns 0).

### 2. Reverse-Engineered Match Finder & Bit-Exact Architecture
Through deep reverse-engineering of `tools/esp_cs.bin`, the entire compression pipeline matches historical DOS `ESP.EXE v1.92` with 100.000% fidelity:
- 4,096-entry hash table with custom hash formula and 32,768-entry sliding dictionary.
- Longest Common Suffix jump acceleration (`0x7CF`).
- Multi-block intermediate buffer splitting at offset `0x4DF4` with stack batching.
- Canonical Huffman tree generation via 2-pass linked-list radix sort and Kraft sum depth adjustment.
- Prefix-repeat redundancy elimination (`repsize`, switch `/J`).

### 3. File Sorting & Internal Extension Table (`/Sabcd` & `/T`)
- **Internal 161-entry extension table** extracted from `esp_cs.bin`: prioritizes code, text, system files, graphics, audio, and binaries in historical GyikSoft order.
- Deterministic hierarchical scanning (subdirectories cataloged first, then files).
- Configurable 4-level priority comparison (`/Sabcd`).

### 4. Full Listfile & Wildcard Support (`@listfile`)
- Both `esp` and `unesp` accept `@listfile` references.
- Supports whitespace or newline separation, comment lines (`#`, `;`), and path wildcards (`*`, `?`).

### 5. Symmetric Password Encryption (`/G`, `/ME1`, `/ME2`, `p<pass>`)
- **Container Flag**: Bit 6 of method byte (`0x40`) indicates password protection.
- **Symmetric Stream Cipher**: Repeating 32-bit XOR cipher derived from `key ^ 0x4B697947` applied to compressed payload and directory streams.
- **Dual Hashing Support**:
  - `/ME1`: Legacy v1.6 hashing (reproduces authentic historical aliasing behavior where passwords such as `ABCD` and `ABCE` yield identical keys `0xFDFBE79E`, container version `0x16`).
  - `/ME2`: Modern v1.9+ hashing (multi-byte cross additions eliminating collisions, container version `0x17`).
- **Interactive or CLI Entry**: Password can be specified directly via CLI (`/Gsecret`, `unesp eypsecret`) or entered interactively via secure terminal prompt (`getpass`).
- **Mutual Exclusion**: Conforms to DOS ESP constraint forbidding `/ME2` and `/MM2` simultaneously.

### 6. Multi-Volume Splitting & Transparent Extraction (`/V`, `/VV`)
- **Volume Header Architecture**:
  - Volume 1 (`.ESP`) retains 10-byte header with `dir_pos = 0xFFFFFFFF` when fragmented.
  - Continuation volumes (`.E01`, `.E02`...) feature 14-byte headers with bit 7 (`0x80`) set, unfragmented start offset in bytes 10..13, and bit 5 (`0x20`) on the terminating volume with original `dir_pos`.
- **Diskette Presets**:
  - `/VV1440` (1,456,400 bytes), `/VV1200` (1,212,400 bytes), `/VV720` (729,400 bytes), `/VV360` (361,400 bytes).
- **Flexible Sizing**:
  - Supports `/VnnnK` (KiB), `/Vnnnk` (KB), `/VnnnM` (MiB), `/Vnnnm` (MB), and automatic KiB conversion when $< 4096$.
- **Seamless Modern Extraction**:
  - Unlike DOS `UNESP.EXE` which refused multi-volume archives with a message requiring external `ESPLINK.COM`, our `unesp` natively verifies volume continuity and extracts or tests multi-volume archives in memory without intermediate disk assembly.

### 7. Archive Comments (/Cn=file, /C=file)
- **Footer Structure**: 16-byte footer at end of archive ending with `"ESPC"`. Stores 32-bit `cmt_pos` and 8-bit `cmt_type` (`1`=Text, `2`=ANSI Screen, `4`=Old PIC, `7`=New PIC 320x200, `6`=New PIC 640x480).
- **Compression**: Comment streams are compressed using Method 3 LZSS (`ESP_METH_FAST`) and encrypted with the XOR stream cipher when password protection is active.
- **Terminal-Safe Presentation**:
  - `/C1` text comments are printed directly to stdout.
  - `/C2` ANSI binary and `/C4..6` VGA graphics PIC comments are decoded and summarized via a clean informative banner (`[Archive Comment: ...]`), preventing binary garbage from polluting modern terminal emulators.
### 8. Path Traversal & Zip-Slip Protection (`esp_is_safe_relpath()`)
Both `esp` and `unesp` strictly validate all extraction paths through `esp_is_safe_relpath()` to protect against malicious archives attempting path traversal or Zip-Slip vulnerabilities:
- **Depth tracking**: Continuously tracks directory nesting depth. Any attempt to ascend above the destination root via `../` or `..\` is intercepted and safely skipped with a warning (`Security warning: skipping unsafe path traversal: <path>`).
- **Absolute & drive path rejection**: Rejects leading absolute slashes (`/`, `\`) and DOS drive identifiers (`C:`).
- **DOS separator normalization**: Automatically normalizes historical DOS backslashes (`DIR\SUBDIR\FILE.TXT`) into forward slashes `/` on POSIX systems, ensuring safe, seamless directory hierarchy recreation.
- **Dual Validation**: Validates both directory creation entries and file extractions across both full extraction (`x`) and flat extraction (`e`).
- **Stream-Safe Diagnostics**: Security warnings are routed to `stderr`, preserving clean Unix piping when operating in bare output mode (`-1` / `--bare`).

---

## Status & Verification

| Component | Status | Verification |
|---|---|---|
| Unpack legacy M0/M3/M4 archives | **100% Complete** | 38/38 archives in `packed_corpus/` |
| Method 0 (Best) LZSS compressor | **100% Bit-Exact** | 100.000% exact payload match on corpus files |
| Method 4 (Store) mode | **100% Bit-Exact** | 100.000% exact payload match on corpus files |
| Complete archive recreation | **100% Bit-Exact** | Header, payload & directory match DOS archives |
| Extension sorting (`/Sabcd`, `/T`) | **100% Bit-Exact** | Matches `NESTED0.ESP` entry order |
| Listfiles (`@listfile`) & wildcards | **100% Complete** | Tested in both archiver and extractor |
| 8-bit Audio/Image Delta (`/MM`) | **100% Bit-Exact** | Decodes `MM_RAW.ESP` and roundtrips 100% |
| 24-bit RGB Filter (`/MM2`) | **100% Bit-Exact** | 100.000% bit-exact recreation of `MM2_RAW.ESP` (333 bytes) |
| Password Encryption (`/G`, `/ME1`, `/ME2`) | **100% Bit-Exact** | Decodes DOS `PASS1`/`PASS2` & 100.000% bit-exact recreation |
| Multi-Volume Splitting (`/V`, `/VV`) | **100% Bit-Exact** | Decodes DOS 12-volume archive & 100.000% bit-exact recreation |
| Archive Comments (`/Cn=file`, `s`) | **100% Bit-Exact & Safe** | Decodes DOS `CMT_M0`, `CMT_M4`, `CMT_C2`, `CMT_C5` + encr/suppr |
| Path Traversal / Zip-Slip Defense | **100% Secure** | Depth tracking, absolute/drive rejection, selftests & suite 11 |

Run the full verification suite anytime with:
```bash
python3 tools/verify_bit_exact.py
```

Detailed technical specifications of the file format and algorithms can be found in [FORMAT.md](FORMAT.md).


