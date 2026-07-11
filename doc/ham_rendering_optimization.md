# HAM Rendering Optimization Analysis

## 1. Overview

Hold-And-Modify (HAM) is the Amiga's most complex display mode. It enables
up to 4096 (OCS/ECS) or 262,144 (AGA/HAM-8) simultaneous colors on screen
by modifying only one color channel per pixel relative to the previous pixel.
This sequential dependency makes it fundamentally incompatible with naive
parallelization and is a known performance bottleneck in all Amiga emulators.

This document analyzes the current HAM rendering pipeline in quaesar-ng's
UAE core, identifies inefficiencies, and proposes optimization strategies
ranging from cross-platform algorithmic improvements to SIMD-specific
implementations per host CPU architecture.

### Profiling Context

Profiler data was captured using `sample` (macOS collapsed format) during
AGA HAM demo scene playback in a Debug build (`-O0`):

| Function                     | Samples | % of Total | Role                          |
|------------------------------|---------|------------|-------------------------------|
| `fill_icache040`             | 29,984  | 16.4%      | 68040 instruction cache sim   |
| `m68k_run_3ce`               | 13,115  | 7.2%       | Main CPU emulation loop       |
| `decide_line`                | 8,314   | 4.6%       | Per-scanline copper/DMA       |
| `linetoscr_32_stretch1_aga`  | 815     | 0.45%      | HAM line-to-screen (pass 3)   |
| `pfield_doline32_n3`         | 97      | 0.05%      | Bitplane fetch (pass 1)       |
| `decode_ham_pixel`           | <10     | <0.01%     | HAM decode (pass 2)           |

While HAM rendering is under 1% of CPU in this profile, the per-frame
timing measurements show **consistent over-budget frames during HAM scenes
even in RelWithDebInfo (`-O2`) builds**. The HAM rendering cost is amplified
by:

- **6-8 bitplanes** consuming more DMA bandwidth per scanline, reducing CPU
  cycles available to emulation
- **Complex copper lists** commonly paired with HAM display in demos
- **Color register changes per scanline** triggering `do_color_changes` overhead
- The **sequential pixel dependency** preventing compiler auto-vectorization

Even if the absolute rendering percentage is small, reducing per-scanline
rendering latency directly affects how many frames stay within the 20ms PAL
budget when combined with heavy CPU/DMA activity.

## 2. HAM Rendering Pipeline Architecture

HAM rendering executes three sequential passes per scanline, each iterating
over every visible pixel. The pipeline is driven by `pfield_draw_line()`
(`drawing.cpp` ~line 3865) and orchestrated by `do_color_changes()` (~line 3649).

### Pipeline Overview

```
  ┌─────────────────────┐
  │  pfield_draw_line   │
  │   (per scanline)    │
  └──────────┬──────────┘
             │
             ▼
  ┌──────────────────────────────┐    6 bitplanes from Amiga RAM
  │  Pass 1: pfield_doline32_n6  │    → pixdata.apixels[] (6-bit indices)
  │  (bitplane → pixel indices)  │    Uses MERGE macro network
  └──────────┬───────────────────┘
             │
             ▼
  ┌──────────────────────┐    pixdata.apixels[] input
  │  Pass 2: decode_ham  │    → ham_linebuf[] (24-bit RGB values)
  │  (HAM pixel decode)  │    Sequential dependency on ham_lastcolor
  └──────────┬───────────┘
             │
             ▼
  ┌──────────────────────┐    ham_linebuf[] input
  │  Pass 3: linetoscr   │    → display buffer (16/32-bit xcolnr)
  │  (color LUT + write) │    p_xcolors[] table lookup per pixel
  └──────────────────────┘
```

### Pass 1: Bitplane Fetch (`pfield_doline`)

**Source:** `drawing.cpp` ~line 3026 (`pfield_doline32_1`)
**Called from:** `drawing.cpp` ~line 3952 (`pfield_doline(lineno)`)

Reads N bitplane pointers (`real_bplpt[0..N-1]`) and uses a bit-merge network
to convert bitplane data into per-pixel color indices stored in
`pixdata.apixels[]` (type: `uae_u8`, max 2304 pixels).

The MERGE network performs 5 stages of XOR-based bit permutation per 32-pixel
word:

```c
#define MERGE(a,b,mask,shift) do {            \
    uae_u32 tmp = mask & (a ^ (b >> shift));   \
    a ^= tmp;                                  \
    b ^= (tmp << shift);                       \
} while (0)
```

For 6-plane HAM, each word requires: 6 loads + 24 MERGE calls + 12 stores.
This pass is bit-parallel and well-optimized — the compiler can vectorize the
MERGE operations to some degree.

### Pass 2: HAM Pixel Decode (`decode_ham` / `decode_ham_pixel`)

**Source:** `drawing.cpp` ~line 2716 (`decode_ham_pixel`)
**Called from:** `drawing.cpp` ~line 2797 (`decode_ham`)

This is the core HAM bottleneck. For each pixel, the 6-bit index from
`pixdata.apixels[]` is decoded into a 24-bit RGB value in `ham_lastcolor`,
which is then written to `ham_linebuf[]`.

**Critical constraint:** `ham_lastcolor` carries state from one pixel to the
next. Pixel N+1's output depends on pixel N's output. This creates a
**sequential dependency chain** that prevents instruction-level parallelism.

#### OCS/ECS HAM-6 decode (per pixel):

```c
int pv = pixdata.apixels[ham_decode_pixel];
switch (pv & 0x30) {          // top 2 bits select mode
    case 0x00: ham_lastcolor = colors_for_drawing.color_regs_ecs[pv] & 0xfff; break;
    case 0x10: ham_lastcolor &= 0xFF0; ham_lastcolor |= (pv & 0xF); break;       // modify Blue
    case 0x20: ham_lastcolor &= 0x0FF; ham_lastcolor |= (pv & 0xF) << 8; break;  // modify Red
    case 0x30: ham_lastcolor &= 0xF0F; ham_lastcolor |= (pv & 0xF) << 4; break;  // modify Green
}
ham_linebuf[ham_decode_pixel++] = ham_lastcolor;
```

#### AGA HAM-8 decode (per pixel):

```c
int pw = pixdata.apixels[hdp];
int pv = pw ^ bplxor;
int pc = pv >> 2;
switch (pv & 0x3) {
    case 0x0: ham_lastcolor = colors_for_drawing.color_regs_aga[pc] & 0xffffff; break;
    case 0x1: ham_lastcolor &= 0xFFFF03; ham_lastcolor |= (pw & 0xFC); break;       // modify Blue (6 bits)
    case 0x2: ham_lastcolor &= 0x03FFFF; ham_lastcolor |= (pw & 0xFC) << 16; break; // modify Red  (6 bits)
    case 0x3: ham_lastcolor &= 0xFF03FF; ham_lastcolor |= (pw & 0xFC) << 8; break;  // modify Green(6 bits)
}
```

**Per-pixel cost:** 1 branch (switch), 1-2 ALU operations, 1 memory write.
The branch is data-dependent and unpredictable — demos frequently alternate
between direct-color and modify modes, causing branch misprediction penalties.

### Pass 3: Line-to-Screen (`linetoscr_*`)

**Source:** `linetoscr.cpp` (auto-generated, 21,000+ lines)
**Called from:** `drawing.cpp` ~line 4013-4016 via `do_color_changes`

For HAM mode (`CMODE_HAM` case), each pixel:
1. Reads `ham_linebuf[spix]` (32-bit RGB value)
2. Looks up `p_xcolors[spix_val]` (table converting 12-bit/24-bit color to native display format)
3. Writes result to `xlinebuffer[dpix]` (16 or 32 bits per pixel)

```c
// HAM path in linetoscr_16_spr:
spix_val = ham_linebuf[spix];
dpix_val = p_xcolors[spix_val];
buf[dpix++] = dpix_val;
```

This pass is memory-bound (two reads + one write per pixel) but has **no
sequential dependency** — it could theoretically be vectorized. However, the
`p_xcolors[]` lookup is a gather operation with unpredictable access patterns.

### Orchestration: `do_color_changes`

**Source:** `drawing.cpp` ~line 3649

This function iterates over recorded color register changes within the scanline.
For each segment between changes, it calls either the border worker or the
playfield worker. The playfield worker for HAM is `decode_ham` (pass 2),
which then feeds into the `linetoscr_*` functions (pass 3).

**Key inefficiency:** `do_color_changes` splits each scanline into segments
at every copper-triggered register change. For HAM scenes with per-line color
changes (very common in demos), this means multiple calls to `decode_ham` and
`linetoscr_*` per scanline, with function-call overhead for each segment.

### Buffer Sizes

| Buffer                | Type        | Size                              |
|-----------------------|-------------|-----------------------------------|
| `pixdata.apixels`     | `uae_u8[]`  | `MAX_PIXELS_PER_LINE * 2` = 4608  |
| `ham_linebuf`         | `uae_u32[]` | `MAX_PIXELS_PER_LINE * 2` = 4608  |
| `xlinebuffer`         | `uae_u8*`   | Display row (variable, up to ~9216 bytes at 32bpp) |
| `color_regs_ecs`      | `uae_u16[]` | 32 entries                        |
| `color_regs_aga`      | `uae_u32[]` | 256 entries                       |
| `xcolors` (LUT)       | `xcolnr[]`  | 4096 entries (ECS) or via hash    |

## 3. Identified Inefficiencies

### 3.1 Branch-Heavy Per-Pixel Decode (Pass 2)

`decode_ham_pixel` uses a data-dependent `switch` statement per pixel.
On modern CPUs with deep pipelines (14+ stages on ARM, 14-19 on x86),
branch mispredictions cost ~15-20 cycles each. In demo scenes, the mode
bits (top 2 of 6 for OCS, bottom 2 for AGA) change unpredictably between
adjacent pixels, making prediction ineffective.

**Cost model (per pixel):**
- Best case (predicted): ~3 cycles (load + ALU + store)
- Worst case (mispredicted): ~18-23 cycles
- Average in mixed demo content: ~8-12 cycles

### 3.2 Triple-Pass Memory Traffic

The pipeline reads and writes each pixel's data three times through
different buffers:

```
apixels[] → [Pass 2] → ham_linebuf[] → [Pass 3] → xlinebuffer[]
```

For a 320-pixel HAM line at 32bpp:
- Pass 2: 320 reads from `apixels` + 320 writes to `ham_linebuf` (1280 bytes)
- Pass 3: 320 reads from `ham_linebuf` + 320 reads from `p_xcolors` + 320 writes to `xlinebuffer` (2560 bytes)
- Total intermediate: ~4 KB of unnecessary memory traffic per scanline

Passes 2 and 3 could be fused: decode HAM pixel → immediately do the
`p_xcolors[]` lookup → write directly to display buffer, eliminating the
`ham_linebuf[]` intermediate entirely.

### 3.3 `ham_linebuf` Wastes Cache

`ham_linebuf` is `uae_u32[MAX_PIXELS_PER_LINE * 2]` = 4608 × 4 = **18,432 bytes**.
This exceeds L1 cache line capacity on most architectures (typically 32-64 KB
total L1D). Each scanline's pass 2 writes the entire buffer, evicting useful
data from L1. Pass 3 then re-reads it, likely causing cache misses.

The `ham_linebuf` values are 24-bit RGB packed in 32-bit words. Storing them
more compactly (e.g., 16-bit 5:6:5 RGB) would halve the buffer size and reduce
cache pressure, though this would require adjusting the `p_xcolors` lookup.

### 3.4 Redundant Border Decode

`init_ham_decoding()` (~line 2783) decodes HAM pixels for the invisible left
border region to establish the correct `ham_lastcolor` when visible pixels
begin. This is necessary for correctness, but the current implementation
decodes each border pixel individually with the full `decode_ham_pixel()`
switch+branch overhead, even though border pixels often don't affect the
final visible output.

### 3.5 No Exploitation of Run-Length Patterns

HAM images typically contain runs of identical mode bits (e.g., long stretches
of "modify blue" for sky gradients). The current per-pixel loop doesn't detect
or exploit these runs. A run-length-aware decoder could:

1. Scan ahead to find the next "direct color" pixel (mode 0x00)
2. For "modify" runs, apply the modification to all pixels in the run without
   branching per pixel
3. Only reset `ham_lastcolor` at direct-color boundaries

### 3.6 `linetoscr.cpp` Code Bloat

The auto-generated `linetoscr.cpp` (21,000+ lines) contains separate
functions for every combination of: bit depth (16/32), stretch factor
(1x/2x), AGA/ECS, sprite/no-sprite, genlock/no-genlock, and color mode
(NORMAL/HAM/DUALPF/EXTRAHB). This results in hundreds of near-identical
functions. The HAM code paths are duplicated ~15 times with only the
color mode branch differing.

This causes:
- **I-cache pressure**: Only a few variants are hot at a time, but the sheer
  file size prevents the compiler from keeping relevant code in cache
- **Missed inlining**: The `NOINLINE` annotation on all variants prevents
  the compiler from optimizing across function boundaries

### 3.7 Function Pointer Dispatch Overhead

`pfield_do_linetoscr_normal` and friends are function pointers set in
`pfield_set_linetoscr()` (~line 2361). While this avoids re-checking the
mode per scanline, it introduces an indirect call per segment within
`do_color_changes`. On ARM, indirect calls also defeat the branch target
predictor (BTB) unless the target is stable across calls.

### 3.8 `do_color_changes` Segment Granularity

For HAM demo scenes with copper-based color changes every 16 pixels,
`do_color_changes` creates ~20 segments per scanline. Each segment incurs:
- Function call to `decode_ham` (pass 2)
- Function call to `pfield_do_linetoscr` → `linetoscr_*` (pass 3)
- Loop setup overhead

This means ~40 function calls per scanline × 256 scanlines = ~10,240 calls
per frame, each with register save/restore overhead.

## 4. Cross-Platform Generic Optimization Strategies

These strategies are portable C/C++ improvements that benefit all host
architectures without SIMD intrinsics.

### 4.1 Branchless HAM Decode

Replace the data-dependent `switch` with arithmetic operations. The key
insight: for "modify" modes, the operation is always "mask off one channel,
OR in new bits." We can compute the mask and value without branching.

#### Branchless OCS/ECS HAM-6:

```c
static inline uae_u32 decode_ham_pixel_branchless(
    uae_u8 pv, uae_u32 ham_lastcolor,
    const uae_u16 *color_regs_ecs)
{
    // Mode: top 2 bits (0x00=direct, 0x10=blue, 0x20=red, 0x30=green)
    int mode = (pv >> 4) & 3;

    // Channel masks: which bits to KEEP (not modify)
    static const uae_u32 keep_mask[4] = {
        0x000,   // 0x00: direct color (replace all — special cased below)
        0xFF0,   // 0x10: keep R+G, modify B
        0x0FF,   // 0x20: keep G+B, modify R
        0xF0F,   // 0x30: keep R+B, modify G
    };
    static const int shift_table[4] = { 0, 0, 8, 4 };

    // Direct color: load from palette
    uae_u32 direct_val = color_regs_ecs[pv] & 0xfff;

    // Modify: keep other channels, insert new channel value
    uae_u32 modify_val = (ham_lastcolor & keep_mask[mode])
                       | ((uae_u32)(pv & 0xF) << shift_table[mode]);

    // Select: mode 0 → direct, mode 1-3 → modify
    // Use arithmetic: (mode != 0) selects modify_val
    uae_u32 result = (mode == 0) ? direct_val : modify_val;
    return result;
}
```

The final ternary still branches, but modern compilers convert it to a
conditional move (CMOV on x86, CSEL on ARM), which is branch-predictor-free.

**Expected improvement:** Eliminates 1 misprediction per pixel on average.
At ~12 cycles per misprediction and ~320 pixels/line × 256 lines = ~1M
pixels/frame, this saves up to ~12M cycles/frame in worst-case content.

### 4.2 Pass Fusion: Merge Decode + Linetoscr

Eliminate `ham_linebuf` as an intermediate by fusing pass 2 and pass 3.
The fused function reads `apixels[]`, decodes HAM to get the RGB value,
immediately looks up `p_xcolors[rgb_val]`, and writes to `xlinebuffer`.

```c
static void decode_ham_to_screen(
    int pix, int stoppos,
    uae_u32 *ham_lastcolor_inout,
    int *ham_decode_pixel_inout,
    uae_u16 *buf, int dpix_start)
{
    uae_u32 ham_lastcolor = *ham_lastcolor_inout;
    int ham_decode_pixel = *ham_decode_pixel_inout;
    int todraw = res_shift_from_window(stoppos - pix);
    int dpix = dpix_start;

    while (todraw-- > 0) {
        // --- Pass 2: HAM decode (branchless) ---
        uae_u8 pv = pixdata.apixels[ham_decode_pixel];
        int mode = (pv >> 4) & 3;
        // ... branchless decode as above ...
        ham_linebuf_local = ham_lastcolor;

        // --- Pass 3: immediate color lookup + write ---
        buf[dpix++] = p_xcolors[ham_lastcolor];

        ham_decode_pixel++;
    }

    *ham_lastcolor_inout = ham_lastcolor;
    *ham_decode_pixel_inout = ham_decode_pixel;
}
```

**Benefits:**
- Eliminates 1280+ bytes of memory writes per scanline (ham_linebuf)
- Eliminates 1280+ bytes of memory reads per scanline (pass 3's input)
- Reduces L1 cache pressure significantly
- Keeps the hot working set in registers

**Constraint:** Sprite compositing currently needs `ham_linebuf` for the
sprite path in `linetoscr_*_spr`. For lines with sprites, the unfused
path must be retained. Sprite + HAM is rare in most content, so a fast
path for sprite-free HAM lines covers the majority of cases.

### 4.3 Run-Length Acceleration for Modify-Only Segments

Between "direct color" (mode 0x00) pixels, the mode is always one of the
three "modify" variants. When a run of same-mode "modify" pixels occurs,
we can process them without per-pixel mode extraction:

```c
// Fast path: all pixels in [start, end) are "modify blue"
while (ham_decode_pixel < end) {
    uae_u8 pv = pixdata.apixels[ham_decode_pixel];
    if ((pv & 0x30) != 0x10) break;  // mode changed
    ham_lastcolor &= 0xFF0;
    ham_lastcolor |= (pv & 0xF);
    buf[dpix++] = p_xcolors[ham_lastcolor];
    ham_decode_pixel++;
}
```

This is particularly effective for gradient skies, color bars, and other
demo effects that use long HAM modify runs. The run-scan adds one branch
per pixel but eliminates the mode extraction and branchless select overhead.

### 4.4 Direct Color Pre-Computation

For "direct color" pixels (mode 0x00), the result is simply
`color_regs[pv]`. We can pre-compute a lookup table mapping the 16 (OCS)
or 64 (AGA) direct-color pixel values to their `p_xcolors[]` output:

```c
// Rebuilt when color registers change (once per do_color_changes segment)
static uae_u32 direct_color_lut[64];  // AGA: 64 entries, OCS: 16

// In hot loop:
if (mode == 0) {
    buf[dpix] = direct_color_lut[pv & 0x3F];
    ham_lastcolor = color_regs[pv]; // still needed for next modify
}
```

This avoids the two-step `color_regs[] → ham_lastcolor → p_xcolors[]` chain
for direct-color pixels, replacing it with a single table read.

### 4.5 Segment Batching in `do_color_changes`

Instead of calling `decode_ham` and `linetoscr_*` separately for each
copper-change segment, batch consecutive segments that share the same
color register set:

1. Scan ahead in `curr_color_changes[]` to find the longest run with no
   `BPLCON0` or color register changes
2. Process the entire run in one `decode_ham` + `linetoscr` call
3. Only split when actual register changes occur

This reduces function-call overhead from ~40 calls/scanline to ~2-4 in
typical content.

### 4.6 Reduce `ham_linebuf` to 16-bit

If pass fusion (4.2) is not feasible for all code paths, reducing
`ham_linebuf` from `uae_u32[]` to `uae_u16[]` (5:6:5 RGB) would:

- Halve buffer size: 18,432 → 9,216 bytes (fits in L1 on most CPUs)
- Halve memory bandwidth for pass 2→3 handoff
- Require a 16-bit-aware `p_xcolors[]` lookup table

This is a compatibility tradeoff — AGA HAM-8 uses 18-bit color (6 bits per
channel), which doesn't fit in 16-bit 5:6:5. A 6:6:6 packed in 18 bits
within a 32-bit word (with 14 bits unused) is an alternative that preserves
precision while reducing from 32 to 24 effective bits per entry.

### 4.7 Inlining the Hot Path

The `NOINLINE` annotations on `linetoscr_*` variants were originally added
to prevent code bloat. But for HAM mode specifically, inlining the
`decode_ham_pixel` body directly into the `linetoscr` loop eliminates:

- Function call overhead (~5-10 cycles per call)
- Register save/restore
- Prevents the compiler from keeping `ham_lastcolor` in a register across
  the call boundary

A `STATIC_INLINE` (or `__attribute__((always_inline))`) for the HAM-only
path would be beneficial, especially if combined with pass fusion (4.2).

## 5. SIMD-Specific Optimization Strategies

### 5.1 The Sequential Dependency Challenge

The fundamental obstacle to SIMD-parallelizing HAM decode is that each pixel's
output depends on the previous pixel's `ham_lastcolor`. This creates a
carry-chain that prevents processing multiple pixels in parallel.

However, the dependency is **resolvable with prefix computation**: if we know
the `ham_lastcolor` entering a group of N pixels, and we know which channels
each pixel modifies, we can compute all N outputs using a prefix-scan style
algorithm.

#### Key Observation

Within a run of "modify" pixels (no "direct color" resets), the HAM operation
is **commutative within a channel**: if pixel A modifies blue and pixel B
also modifies blue, only B's value matters (A's is overwritten). This means
we can process modify-runs in parallel — we just need the last modify of
each channel within the run.

### 5.2 SIMD Decomposition: Segment + Parallel Modify

The strategy splits each scanline into segments at "direct color" (mode 0x00)
boundaries, then processes the modify-only runs between them in parallel.

#### Algorithm:

```
1. Scan apixels[] to find positions of all "direct color" pixels
2. Each direct-color pixel establishes a new ham_lastcolor seed
3. Between direct-color pixels, we have a modify-only run
4. For each modify run:
   a. Load 4-8 pixel indices into a SIMD register
   b. Extract mode bits (which channel to modify)
   c. Extract value bits
   d. Apply branchless mask+OR using vector select operations
   e. Prefix-propagate the running color through the run
5. Write decoded values to output buffer
```

The prefix propagation (step e) is the tricky part. For a run where all
pixels modify the **same channel**, it simplifies to "each pixel's output
is its own value in that channel + the carry from the last direct-color."
This is trivially parallel.

For **mixed-channel** modify runs (e.g., blue, blue, red, green, blue),
the channels are independent — we can compute all three channel modifications
in parallel, then combine:

```
channel_r[i] = (mode[i] == RED)  ? value[i] : carry_r
channel_g[i] = (mode[i] == GREEN)? value[i] : carry_g
channel_b[i] = (mode[i] == BLUE) ? value[i] : carry_b
output[i] = (channel_r[i], channel_g[i], channel_b[i])
```

Where `carry_*` is the last value written to that channel before position i.
This is a conditional-move prefix scan, which SIMD handles efficiently.

---

### 5.3 ARM NEON Implementation (Apple Silicon, ARM64)

Apple M-series CPUs have wide NEON units (128-bit, 4× 32-bit lanes) with
excellent throughput. Key NEON primitives for HAM:

| Operation        | NEON Intrinsic           | Use                           |
|------------------|--------------------------|-------------------------------|
| Vector compare   | `vceq_u8` / `vcgt_u8`    | Mode detection (== 0x00?)     |
| Bitwise select   | `vbsl_u8` / `vbit_u8`    | Branchless mask+OR            |
| Table lookup     | `vtbl1_u8` / `vtbl2_u8`  | Color register LUT (≤8 bits)  |
| Zip/unzip        | `vzip_u8` / `vuzp_u8`    | Channel deinterleave          |
| Prefix scan      | Manual (2-3 steps)       | Channel carry propagation     |

#### NEON HAM-6 Decode (4 pixels at a time):

```cpp
#if defined(__ARM_NEON) || defined(__aarch64__)
#include <arm_neon.h>

// Decode 4 OCS HAM-6 pixels in parallel.
// Precondition: no direct-color (mode 0x00) pixels in this group.
// seed = ham_lastcolor entering this group.
static inline uint32x4_t decode_ham6_modify_quad(
    uint8x8_t pixels,      // 4 pixel indices in low bytes
    uint32_t seed,         // ham_lastcolor at start
    const uint16_t *color_regs)
{
    // Extract mode bits: (pv >> 4) & 3
    uint8x8_t mode = vshr_n_u8(pixels, 4);
    mode = vand_u8(mode, vdup_n_u8(3));

    // Extract value bits: pv & 0xF
    uint8x8_t val = vand_u8(pixels, vdup_n_u8(0x0F));

    // Shift values per channel: blue=0, red=8, green=4
    // Build shifted values for all 3 channels
    uint16x8_t val_w = vmovl_u8(val);        // widen to 16-bit
    uint32x4_t val_blue  = vmovl_u16(vget_low_u16(val_w));
    uint32x4_t val_red   = vshl_n_u32(val_blue, vdupq_n_u32(8));
    uint32x4_t val_green = vshl_n_u32(val_blue, vdupq_n_u32(4));

    // Channel masks for "keep" (which bits to preserve from previous)
    uint32x4_t seed_v = vdupq_n_u32(seed);
    uint32x4_t keep_blue  = vdupq_n_u32(0xFF0);  // keep R+G
    uint32x4_t keep_red   = vdupq_n_u32(0x0FF);  // keep G+B
    uint32x4_t keep_green = vdupq_n_u32(0xF0F);  // keep R+B

    // For each channel: if mode matches, write new value; else keep seed
    // mode 1 = blue, mode 2 = red, mode 3 = green
    uint32x4_t mode_v = vmovl_u16(vget_low_u16(vmovl_u8(mode)));

    uint32x4_t is_blue  = vceqq_u32(mode_v, vdupq_n_u32(1));
    uint32x4_t is_red   = vceqq_u32(mode_v, vdupq_n_u32(2));
    uint32x4_t is_green = vceqq_u32(mode_v, vdupq_n_u32(3));

    // Apply modifications
    uint32x4_t result = seed_v;
    result = vbslq_u32(is_blue,  (seed_v & keep_blue)  | val_blue,  result);
    result = vbslq_u32(is_red,   (seed_v & keep_red)   | val_red,   result);
    result = vbslq_u32(is_green, (seed_v & keep_green) | val_green, result);

    return result;
}
#endif
```

**Throughput:** 4 pixels per ~12 NEON instructions ≈ 3 cycles (on Apple M-series
with 2 NEON ALUs). Compared to ~24-48 cycles for 4 scalar pixels with branch
misprediction, this is an **8-16x speedup** for modify-only runs.

**Prefix propagation note:** The code above assumes all 4 pixels start from the
same `seed`. This is correct for the first pixel in each group. For subsequent
pixels within the group, we need a prefix scan to propagate the running color.
This adds ~4-6 more instructions for a 4-element scan, still netting a
**4-8x speedup**.

#### NEON Table Lookup for Color Registers

ARM NEON's `vtbl` (table lookup) instruction can index into an 8-bit → 8-bit
table of up to 64 entries (using 4 registers). For OCS/ECS with 32 color
registers, this fits in a single `vtbl1` instruction:

```cpp
// Look up 8 direct-color pixels simultaneously
uint8x8x2_t color_table;  // 32 entries × 2 bytes = 64 bytes
// ... fill from color_regs_ecs ...
uint8x8x2_t colors = vtbl2_u8(color_table, pixel_indices);
```

---

### 5.4 x86 SSE/AVX Implementation (Intel, AMD)

x86 has wider SIMD: SSE (128-bit), AVX2 (256-bit), AVX-512 (512-bit).

| Operation        | SSE/AVX Intrinsic           | Use                           |
|------------------|------------------------------|-------------------------------|
| Vector compare   | `_mm_cmpeq_epi8`             | Mode detection                |
| Bitwise select   | `_mm_blendv_epi8` (SSE4.1)   | Branchless mask+OR            |
| Gather           | `_mm_i32gather_epi32` (AVX2) | Color register LUT            |
| Shuffle          | `_mm_shuffle_epi8` (SSSE3)   | 16-entry LUT per register     |

#### SSE4 HAM-6 Modify Run (4 pixels):

```cpp
#if defined(__SSE4_1__)
#include <smmintrin.h>

static inline __m128i decode_ham6_modify_quad_sse(
    __m128i pixels,        // 4 pixel indices (bytes, rest zeroed)
    uint32_t seed,
    const uint16_t *color_regs)
{
    // Extract mode and value
    __m128i mode_mask = _mm_set1_epi8(0x30);
    __m128i val_mask  = _mm_set1_epi8(0x0F);
    __m128i modes = _mm_and_si128(pixels, mode_mask);  // 0x00, 0x10, 0x20, 0x30
    __m128i vals  = _mm_and_si128(pixels, val_mask);

    // Per-channel computation using _mm_blendv_epi8 (mask = all-1s → select val)
    // ... similar structure to NEON, using SSE intrinsics ...

    // _mm_blendv_epi8 replaces vbslq for branchless selection
    // _mm_shuffle_epi8 can do 16-entry table lookups (for OCS 16-color palette)
}
#endif
```

#### AVX2 Advantage: 8 Pixels at Once

AVX2 (`__m256i`) doubles throughput to 8 pixels simultaneously. The
`_mm256_i32gather_epi32` instruction can gather 8 color register values in
~5-8 cycles, eliminating the need for shuffle-based table lookups for
AGA's 256-entry palette.

#### SSSE3 PSHUFB Trick for Direct Color

`_mm_shuffle_epi8` (PSHUFB) treats one XMM register as a 16-byte lookup
table and another as indices. For OCS HAM-6 direct color (16 palette
entries), this gives a single-instruction parallel LUT for 16 pixels:

```cpp
// Load 16 pixel indices (direct color mode: pv & 0x0F)
__m128i indices = _mm_loadu_si128((__m128i*)&apixels[start]);
indices = _mm_and_si128(indices, _mm_set1_epi8(0x0F));

// Load 16 color register bytes (packed as appropriate)
__m128i lut = _mm_loadu_si128((__m128i*)packed_color_regs);

// Single-instruction parallel lookup!
__m128i colors = _mm_shuffle_epi8(lut, indices);
```

---

### 5.5 SIMD Pass 3: Parallel Color LUT

Pass 3 (linetoscr) is the easiest to vectorize because it has no sequential
dependency. The `p_xcolors[]` lookup is a gather operation:

```cpp
// AVX2: gather 8 xcolnr values from p_xcolors[]
__m256i indices = _mm256_loadu_si256((__m256i*)&ham_linebuf[start]);
__m256i colors = _mm256_i32gather_epi32(
    (const int*)p_xcolors, indices, 4);
_mm256_storeu_si256((__m256i*)&buf[dpix], colors);
```

On ARM NEON, there's no hardware gather — use `vtbl` for small tables or
fall back to scalar for large ones. For OCS (4096-entry `xcolors`), a
software gather loop is needed.

---

### 5.6 Fused SIMD Decode+Render

The ultimate optimization combines decode (pass 2) and render (pass 3) into
a single SIMD inner loop, eliminating `ham_linebuf` entirely:

```
For each modify-run segment:
  1. Load 4-8 pixel indices from apixels[] (SIMD load)
  2. Decode HAM → 4-8 RGB values (SIMD ALU, prefix scan)
  3. Gather p_xcolors[] → 4-8 display-format values (SIMD gather/shuffle)
  4. Store to xlinebuffer (SIMD store)
```

This reduces the pipeline from 3 passes with 2 intermediate buffers to a
single fused pass with zero intermediates. Expected throughput on Apple
M-series: **~1 cycle per pixel** for modify-heavy content, vs ~8-12 cycles
per pixel currently.

## 6. Implementation Roadmap

Strategies ranked by effort vs. impact, targeting the user's primary
platform (Apple Silicon / ARM64 NEON):

### Phase 1: Quick Wins (Low effort, high impact)

| # | Strategy                  | Effort | Impact  | Notes                              |
|---|---------------------------|--------|---------|------------------------------------|
| 1 | Branchless decode (4.1)   | Small  | High    | Drop-in replacement for switch     |
| 2 | Pass fusion (4.2)         | Medium | High    | Eliminates ham_linebuf for common  |
| 3 | Inlining hot path (4.7)   | Small  | Medium  | Remove NOINLINE on HAM path        |

Phase 1 targets the scalar hot loop. Combined, these should yield a **3-5x**
reduction in HAM decode time with zero platform-specific code.

### Phase 2: NEON Acceleration (Medium effort, high impact)

| # | Strategy                           | Effort | Impact  | Notes                          |
|---|------------------------------------|--------|---------|--------------------------------|
| 4 | NEON modify-run decoder (5.3)     | Medium | High    | 4 pixels/cycle on Apple Silicon|
| 5 | NEON table lookup for direct color| Small  | Medium  | vtbl for 32-entry ECS palette  |
| 6 | Fused SIMD decode+render (5.6)    | Large  | High    | Ultimate path, builds on 4+5   |

Phase 2 adds platform-specific code behind `#ifdef __ARM_NEON`. Expected
**4-8x** additional speedup on Apple Silicon.

### Phase 3: x86 SSE/AVX (For completeness)

| # | Strategy                           | Effort | Impact  | Notes                          |
|---|------------------------------------|--------|---------|--------------------------------|
| 7 | SSE4 modify-run decoder (5.4)     | Medium | Medium  | If x86 targets are needed      |
| 8 | AVX2 8-wide decoder               | Large  | Medium  | Intel/AMD specific             |

Phase 3 is only needed if the emulator targets x86 platforms. Apple Silicon
is the primary target.

### Phase 4: Structural Improvements (High effort, systemic)

| # | Strategy                           | Effort | Impact  | Notes                          |
|---|------------------------------------|--------|---------|--------------------------------|
| 9 | Segment batching (4.5)            | Medium | Medium  | Reduces call overhead          |
| 10| Run-length acceleration (4.3)     | Medium | Variable| Great for gradients, ok for mixed |

### Estimated Combined Impact

For a typical AGA HAM demo scene (320×256, 256 lines, ~82K pixels/frame):

| Stage                         | Cycles/frame (est.) | vs Current |
|-------------------------------|---------------------|------------|
| Current (scalar, 3-pass)      | ~820K-1.2M          | 1.0x       |
| Phase 1 (branchless + fusion) | ~200K-350K          | 3-5x       |
| Phase 1+2 (NEON fused)        | ~80K-120K           | 8-12x      |

At 50 fps, the HAM budget is 400ms ÷ 50 = 8ms/frame total. Current HAM
rendering takes ~0.8-1.2ms (debug) or ~0.2-0.4ms (release). A 10x
improvement brings this to ~0.02-0.04ms, making HAM rendering effectively
free relative to CPU emulation.

### Risk Assessment

- **Correctness risk:** HAM mode has many edge cases (dual playfield + HAM,
  color changes mid-line, border color propagation, BPLCON4 XOR). Any
  optimization must be validated against the full WinUAE test suite and
  real-world demo content.

- **Maintenance risk:** SIMD code is platform-specific. Use compile-time
  dispatch (`#ifdef`) and provide a scalar fallback for each SIMD path.
  Consider a function pointer table set once at init based on CPU features.

- **Regression risk:** The auto-generated `linetoscr.cpp` must remain
  unchanged for non-HAM modes. HAM-specific optimizations should be
  isolated in separate functions called only when `bplmode == CMODE_HAM`.
