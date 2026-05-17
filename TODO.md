# Compiler Warning Elimination TODO

**Original Warnings:** 5,620
**Current Warnings:** 3,966
**Eliminated:** 1,654 (29.4% reduction)
**Files Remaining:** 145

---

## Warning Categories (by count)

| Category | Count | Strategy |
|----------|-------|----------|
| missing-field-initializers | 1,449 | Add `{}` or `{0}` to struct initializers |
| missing-braces | 561 | Add braces around sub-object initialization |
| unused-but-set-variable | 806 | Comment out or add `(void)` casts |
| unused-parameter | 559 | Add `(void)param;` to function bodies |
| sign-compare | 220 | Cast to matching types |
| unused-variable | 181 | Comment out or use |
| unused-function | 149 | Comment out or `__attribute__((unused))` |
| format-mismatch | 28 | Fix printf format specifiers |
| null-conversion | 3 | Use `0` for integers, `false` for bools |
| pointer-bool-conversion | 1 | Check `array[0] != 0` instead of `array` |
| other | 32 | Case-by-case fixes |

---

## Phase 1: Quick Wins (~2,500 warnings)

### 1. rommgr.cpp - 1,042 warnings

**Path:** `uae_src/rommgr.cpp`

| Type | Count |
|------|-------|
| missing-braces | 560 |
| missing field 'configname' | 342 |
| missing field 'sortpriority' | 127 |
| sign-compare | 7 |
| unused-parameter | 2 |
| unused-variable | 1 |
| other | 3 |

**Strategy:** Fix board_data struct initialization pattern

---

### 2. expansion.cpp - 372 warnings

**Path:** `uae_src/expansion.cpp`

| Type | Count |
|------|-------|
| missing field 'type' | 75 |
| missing field 'configname' | 68 |
| missing field 'memory_mid' | 49 |
| missing field 'autoconfig' | 32 |
| missing field 'sub_banks' | 18 |
| missing field 'invert' | 16 |
| missing field 'e8' | 11 |
| missing field 'z3extra' | 10 |
| missing field 'memory_after' | 9 |
| unused-function | 48 |
| unused-variable | 20 |
| unused-parameter | 10 |
| sign-compare | 5 |
| other | 1 |

**Strategy:** Fix struct initialization, comment out unused functions/vars

---

### 3. identify.cpp - 250 warnings

**Path:** `uae_src/identify.cpp`

| Type | Count |
|------|-------|
| missing field 'mask' | 247 |
| missing field 'special' | 1 |
| missing field 'adr' | 1 |
| unused-variable | 1 |

**Strategy:** Fix ide_data_type array initialization

---

### 4. inputevents.def - 249 warnings

**Path:** `uae_src/inputevents.def`

| Type | Count |
|------|-------|
| missing field 'data2' | 249 |

**Strategy:** Fix macro or struct definition

---

### 5. cpuemu_*.cpp files - ~800 warnings

**Files:** cpuemu_0.cpp, cpuemu_11.cpp, cpuemu_13.cpp, cpuemu_20.cpp, cpuemu_21.cpp, cpuemu_22.cpp, cpuemu_23.cpp, cpuemu_24.cpp, cpuemu_31.cpp, cpuemu_32.cpp, cpuemu_33.cpp, cpuemu_34.cpp, cpuemu_35.cpp, cpuemu_40.cpp, cpuemu_50.cpp

| Variable | Total Count |
|----------|-------------|
| dummy | 221 |
| count_cycles | 204 |
| pcadjust | 160 |
| tmp_newv | 127 |

**Strategy:** Comment out unused variables or add `(void)` casts (generated CPU emulation code)

---

## Phase 2: Systematic Fixes (~780 warnings)

### 6. Unused Parameters - 559 warnings

**Top patterns:**
- 27x `addr`
- 22x `ncr`
- 19x `v`
- 18x `b`
- 17x `hpos`
- 17x `ctx`
- 16x `pcibs`
- 16x `board`

**Strategy:** Add `(void)param;` at start of function bodies across 60+ files

---

### 7. Sign-Compare Issues - 220 warnings

| Pattern | Count |
|---------|-------|
| 'uae_u32' vs 'int' | 49 |
| 'int' vs 'uae_u32' | 44 |
| 'int' vs 'unsigned long' | 23 |
| 'uint64_t' vs 'int' | 13 |
| 'uaecptr' vs 'int' | 13 |
| 'int' vs 'size_t' | 13 |
| 'unsigned int' vs 'int' | 13 |
| 'int' vs 'unsigned int' | 10 |
| Other | 42 |

**Strategy:** Cast to matching unsigned/signed types

---

## Phase 3: Cleanup (~400 warnings)

### 8. Unused Variables - 181 warnings

Scattered across 40+ files. Comment out or use appropriately.

---

### 9. Unused Functions - 149 warnings

**Top files:**
- expansion.cpp: 48
- custom.cpp: 11
- drawing.cpp: 13
- linetoscr.cpp: 16
- gayle.cpp: 10
- memory.cpp: 7

**Strategy:** Comment out with `// Unused function - kept for future use` or add `__attribute__((unused))`

---

### 10. Other Warnings - 64 warnings

| Type | Count |
|------|-------|
| format-mismatch | 28 |
| other (precedence, comma, etc.) | 32 |
| null-conversion | 3 |
| pointer-bool-conversion | 1 |

---

## Complete File List (145 files remaining)

| Warnings | File |
|----------|------|
| 1,042 | uae_src/rommgr.cpp |
| 372 | uae_src/expansion.cpp |
| 250 | uae_src/identify.cpp |
| 249 | uae_src/inputevents.def |
| 132 | uae_src/cpuemu_40.cpp |
| 91 | uae_src/cpuemu_21.cpp |
| 91 | uae_src/cpuemu_23.cpp |
| 90 | uae_src/sndboard.cpp |
| 89 | uae_src/filesys.cpp |
| 86 | uae_src/custom.cpp |
| 73 | uae_src/drawing.cpp |
| 71 | uae_src/memory.cpp |
| 68 | uae_src/cpuemu_0.cpp |
| 68 | uae_src/cpuemu_50.cpp |
| 63 | uae_src/cpuemu_35.cpp |
| 63 | uae_src/inputdevice.cpp |
| 62 | uae_src/cpuemu_24.cpp |
| 58 | uae_src/cfgfile.cpp |
| 57 | uae_src/debug.cpp |
| 55 | uae_src/scsi.cpp |
| 52 | uae_src/keybuf.cpp |
| 46 | uae_src/debugmem.cpp |
| 38 | uae_src/newcpu.cpp |
| 34 | uae_src/idecontrollers.cpp |
| 32 | uae_src/gayle.cpp |
| 27 | uae_src/hardfile.cpp |
| 26 | uae_src/cpuemu_20.cpp |
| 26 | uae_src/cpuemu_32.cpp |
| 26 | uae_src/cpuemu_34.cpp |
| 25 | uae_src/cpuemu_22.cpp |
| 25 | uae_src/cpuemu_31.cpp |
| 25 | uae_src/cpuemu_33.cpp |
| 24 | uae_src/isofs.cpp |
| 21 | uae_src/zfile.cpp |
| 20 | uae_src/disk.cpp |
| 17 | uae_src/fdi2raw.cpp |
| 17 | uae_src/uaenative.cpp |
| 16 | uae_src/linetoscr.cpp |
| 16 | uae_src/savestate.cpp |
| 16 | uae_src/scsiemul.cpp |
| 16 | uae_src/uaeserial.cpp |
| 15 | uae_src/blkdev.cpp |
| 15 | uae_src/traps.cpp |
| 15 | uae_src/zfile_archive.cpp |
| 15 | uae_src/gfxutil.cpp |
| 13 | uae_src/newcpu_common.cpp |
| 13 | uae_src/tabletlibrary.cpp |
| 12 | uae_src/cpummu.cpp |
| 12 | uae_src/fdi2raw.cpp |
| 12 | uae_src/sound.cpp (src/sounddep/) |
| 11 | uae_src/disasm.cpp |
| 10 | uae_src/fpp_native.cpp |
| 10 | uae_src/softfloat/softfloat-specialize.h |
| 9 | uae_src/cdtvcr.cpp |
| 9 | uae_src/softfloat/softfloat.cpp |
| 8 | uae_src/cia.cpp |
| 8 | uae_src/cpummu30.cpp |
| 7 | uae_src/flashrom.cpp |
| 7 | uae_src/fpp.cpp |
| 7 | uae_src/scsitape.cpp |
| 6 | uae_src/ide.cpp |
| 5 | uae_src/enforcer.cpp |
| 5 | uae_src/ethernet.cpp |
| 5 | uae_src/statusline.cpp |
| 4 | uae_src/include/cpu_prefetch.h |
| 4 | uae_src/main.cpp |
| 3 | uae_src/autoconf.cpp |
| 3 | uae_src/fsdb.cpp |
| 3 | uae_src/gfx.cpp (src/) |
| 3 | uae_src/serial.cpp |
| 3 | uae_src/softfloat/SOFTFLOAT-MACROS.H |
| 3 | uae_src/softfloat/softfloat_fpsp.cpp |
| 2 | uae_src/consolehook.cpp |
| 2 | uae_src/dongle.cpp |
| 2 | uae_src/readcpu.cpp |
| 2 | uae_src/uaelib.cpp |
| 1 | uae_src/blitter.cpp |
| 1 | uae_src/cd32_fmv.cpp |
| 1 | uae_src/cpuboard.cpp |
| 1 | uae_src/diskutil.cpp |
| 1 | uae_src/driveclick.cpp |
| 1 | uae_src/events.cpp |
| 1 | uae_src/file.cpp |
| 1 | uae_src/hrtmon.rom.cpp |
| 1 | uae_src/logging.cpp (src/) |
| 1 | uae_src/mman.cpp (uae_src/od-win32/) |
| 1 | uae_src/native2amiga.cpp |
| 1 | uae_src/rommgr.cpp (command line) |
| 1 | uae_src/rp.cpp (src/) |
| 1 | uae_src/rtc.cpp |
| 1 | uae_src/sampler.cpp |
| 1 | uae_src/winuaeboot.cpp |
| ... | (145 files total) |

---

## Fix Strategy

1. **missing-field-initializers** → Add `= {}` or `= {0}` to struct/array initializers
2. **missing-braces** → Use `= {{}}` instead of `= {0}` for nested structs
3. **unused-but-set-variable** → Comment out or add `(void)var;` if kept for debugging
4. **unused-parameter** → Add `(void)param;` at function start
5. **sign-compare** → Add explicit casts to matching types
6. **unused-variable** → Remove or comment out
7. **unused-function** → Comment out or add `__attribute__((unused))`
8. **format-mismatch** → Fix printf format specifiers
9. **null-conversion** → Use `0` for integers, `false` for bools
10. **pointer-bool-conversion** → Check `array[0] != 0` instead of `array`

**Rule:** NO pragma suppressions - fix warnings properly with code changes!

---

## Progress Tracking

| Phase | Target | Warnings | Status |
|-------|--------|----------|--------|
| Phase 1.1 | rommgr.cpp | 1,042 | ⏳ Pending |
| Phase 1.2 | expansion.cpp | 372 | ⏳ Pending |
| Phase 1.3 | identify.cpp | 250 | ⏳ Pending |
| Phase 1.4 | inputevents.def | 249 | ⏳ Pending |
| Phase 1.5 | cpuemu_*.cpp files | ~800 | ⏳ Pending |
| Phase 2.1 | Unused parameters | 559 | ⏳ Pending |
| Phase 2.2 | Sign-compare | 220 | ⏳ Pending |
| Phase 3 | Cleanup | ~400 | ⏳ Pending |

**Expected Final Result:** ~0 warnings

---

## How to Rebuild and Analyze

```bash
cd build
make clean
make -j8 2>&1 | tee /tmp/warning_analysis.log
grep "warning:" /tmp/warning_analysis.log | wc -l
```

## How to Regenerate This File

```python
python3 << 'EOF'
import re
from collections import defaultdict

files = defaultdict(int)
categories = defaultdict(int)

with open('/tmp/warning_analysis.log', 'r') as f:
    for line in f:
        if 'warning:' not in line:
            continue
        
        match = re.search(r'(/Volumes/TB4-4Tb/Projects/emulators/github/quaesar-ng/[^\s:]+)', line)
        if match:
            filepath = match.group(1).replace('/Volumes/TB4-4Tb/Projects/emulators/github/quaesar-ng/', '')
            files[filepath] += 1
        
        if 'missing-field-initializers' in line:
            categories['missing-field-initializers'] += 1
        elif 'missing-braces' in line:
            categories['missing-braces'] += 1
        # ... etc
EOF
```
