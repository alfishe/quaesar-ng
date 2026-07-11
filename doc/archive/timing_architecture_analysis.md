# Timing Architecture Analysis: Why the Emulator Polls the Clock Excessively

> **Related:** [profiling_hotspots.md](profiling_hotspots.md)  
> **Affected file:** `libs/uae_lib/custom.cpp` — `framewait()`  
> **Affected file:** `src/uae_lib_imp/time.cpp` — timing implementation  
> **Affected file:** `src/uae_lib_imp/dummy.cpp` — `show_screen()` stub

---

## Executive Summary

The emulator's frame synchronization architecture assumes that
`show_screen()` blocks for vertical refresh (vsync). On macOS, `show_screen()`
is a no-op stub. This causes `framewait()` to fall into aggressive busy-wait
spin loops, resulting in **~1 million `mach_absolute_time` calls per second**
and **~40% CPU overhead** on the emulator thread.

This document traces the full call chain, explains why each poll exists, and
proposes concrete fixes.

---

## 1. The Call Chain

```
m68k_go()
  └─ vsync_handler_post()
       └─ framewait()                          ← frame sync entry
            ├─ read_processor_time()           ← 6× per frame (redundant)
            ├─ rpt_vsync() → read_processor_time()  ← per spin-loop iteration
            └─ show_screen()                   ← no-op on macOS
```

Each `read_processor_time()` call resolves as:

```
read_processor_time()        [inline, uae/time.h:21]
  └─ uae_time()              [time.cpp]
       └─ time_us()          [time.cpp]
            └─ clock_gettime(CLOCK_MONOTONIC)   ← POSIX wrapper
                 └─ mach_absolute_time()         ← actual hardware read
```

On Apple Silicon, `mach_absolute_time()` executes a single `MRS cntvct_el0`
instruction (~5ns). The overhead comes from the wrapper chain and the sheer
**frequency** of calls.

---

## 2. The Active Code Path

### 2.1 Configuration Routing

The default configuration determines which `framewait()` branch is active:

| Setting | Default | Source |
|---------|---------|--------|
| `m68k_speed` | `0` | `main.cpp:361` |
| `cpu_cycle_exact` | `true` | `uae_server_thread.cpp:82` |
| `cpu_memory_cycle_exact` | `true` | `uae_server_thread.cpp:83` |
| `gfx_vsync` (windowed) | `0` | macOS: no vsync in windowed SDL |
| `gfx_vsyncmode` | `0` | `main.cpp:207` |

With `m68k_speed = 0` (not `< 0`), `cpu_memory_cycle_exact = true`, and
`cpu_sleepmode` active, `isvsync_chipset()` returns `0` in windowed mode,
routing `framewait()` into the `else` branch at **`custom.cpp:12041`**.

### 2.2 The Two Spin Loops

```c
// custom.cpp:12050-12066  (simplified)

// LOOP 1: Bounded wait with 1ms sleep
while (!currprefs.turbo_emulation) {
    float v = rpt_vsync(clockadjust) / (syncbase / 1000.0f);  // reads clock
    if (v >= -FRAMEWAIT_MIN_MS)   // FRAMEWAIT_MIN_MS = 2
        break;
    rtg_vsynccheck();
    maybe_process_pull_audio();
    if (cpu_sleep_millis(1) < 0)  // ← sleeps 1ms per iteration
        break;
}

// LOOP 2: Pure spin — NO sleep, NO backoff
while (rpt_vsync(clockadjust) < 0) {  // reads clock EVERY iteration
    rtg_vsynccheck();
    if (audio_is_pull_event()) {
        maybe_process_pull_audio();
        break;
    }
}
```

### 2.3 Why Loop 2 Is Catastrophic

| Property | Loop 1 | Loop 2 |
|----------|--------|--------|
| Sleep per iteration | 1ms (`cpu_sleep_millis`) | **None** |
| Iterations per frame | ~18 | **~20,000** |
| Clock reads per frame | ~18 | **~20,000** |
| Clock reads per second (50Hz) | ~900 | **~1,000,000** |
| CPU usage per frame | ~0% (sleeping) | **~85% (spinning)** |

Loop 2 is intended as a "final precision wait" — the last 2ms before vsync,
where a 1ms sleep would overshoot. In the original WinUAE architecture, this
spin is bounded because `show_screen()` at line 12073 blocks until the actual
display vertical refresh, naturally throttling the loop.

On macOS, `show_screen()` is a no-op:

```c
// src/uae_lib_imp/dummy.cpp:101
void show_screen(int /*monid*/, int /*mode*/) {
    TRACE();   // does nothing
}
```

Since nothing blocks, the CPU emulation finishes in ~2-3ms, and then **the
entire remaining frame budget (~17-18ms out of 20ms) is burned in the spin
loop at 100% CPU**.

---

## 3. Redundant Per-Frame Clock Reads

Beyond the spin loops, `framewait()` reads the clock **6 times per frame** in
the `else` branch, where only 2 are strictly necessary:

| Line | Code | Purpose | Necessary? |
|------|------|---------|:----------:|
| 12045 | `start = read_processor_time()` | Begin timing | **Yes** |
| 12048 | `t = read_processor_time() - start` | Measure `crender_screen()` cost | No¹ |
| 12068 | `idletime += read_processor_time() - start` | Track idle time | No² |
| 12069 | `curr_time = read_processor_time()` | Current time for sync | **Yes** |
| 12074 | `t += read_processor_time() - curr_time` | Measure `show_screen()` cost | No¹ |

¹ `show_screen()` is a no-op on macOS, so `t ≈ 0` always.  
² `idletime` could reuse `curr_time` from line 12069 instead of reading again.

At 50 Hz this is only 300 extra reads/sec — negligible compared to the spin
loop, but it's still wasteful and easy to fix.

---

## 4. What's NOT the Problem

### `do_cycles_slow` / `event_check_vsync`

These functions are in the CPU instruction dispatch inner loop, so they're
called millions of times per second. However, they only read the clock when
`is_syncline` equals `-10` or `< -10`, which are **never set** on macOS.
The values actually used on this platform are `-1`, `-2`, `-3`, and positive
scanline numbers — none of which trigger the `read_processor_time()` branches
in `event_check_vsync()`.

### `fill_icache040`

This is the 68040 instruction cache simulation — the second-largest hot-spot
at 5,993 self-samples. It's expensive but **inherent to cycle-exact 68040
emulation**. Not a polling issue; not optimizable without sacrificing
emulation accuracy.

### `decide_line`, `decide_sprites_fetch`, `do_cycles_ce020`

These are Amiga chipset cycle-exact emulation functions. They're inherently
expensive per-scanline but will become relatively cheaper once the clock
polling overhead is removed (since they compete for CPU time with the spin
loop).

---

## 5. Implemented Fix (P0)

### Direct `mach_absolute_time` Access

File: `src/uae_lib_imp/time.cpp`

**Before:**
```c
static int64_t time_ns() {
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return (uint64_t)(tp.tv_sec * 1e9 + tp.tv_nsec);
}
static int64_t time_us() {
    return time_ns() / 1000;
}
```

**After:**
```c
static double s_mach_ticks_to_us;

static void mach_time_init(void) {
    mach_timebase_info_data_t info;
    mach_timebase_info(&info);
    s_mach_ticks_to_us = (double)info.numer / ((double)info.denom * 1000.0);
}

static inline int64_t time_us(void) {
    return (int64_t)(mach_absolute_time() * s_mach_ticks_to_us);
}
```

**What this eliminates:**
- `clock_gettime()` function call overhead (parameter validation, errno setup)
- `timespec` struct construction on the stack
- Two multiplications and one addition for ns conversion
- Division by 1000 for us conversion

**What this does NOT eliminate:** The core problem — ~1M calls/sec. This fix
reduces the *per-call cost* by roughly 3-5×, turning a ~100ns operation into
a ~20-25ns operation. The *call frequency* remains the same.

---

## 6. Proposed Fix (P1): Eliminate the Spin Loop

The architectural fix is to ensure Loop 2 never runs at 100% CPU when
`show_screen()` doesn't block.

### Option A: Hybrid Spin-Then-Sleep (Minimal Change)

```c
// Replace Loop 2 with a hybrid approach:
int spin_iterations = 0;
while (rpt_vsync(clockadjust) < 0) {
    rtg_vsynccheck();
    if (audio_is_pull_event()) {
        maybe_process_pull_audio();
        break;
    }
    // After 1000 spin iterations (~0.1ms), fall back to 1ms sleep
    if (++spin_iterations > 1000) {
        cpu_sleep_millis(1);
        spin_iterations = 0;
    }
}
```

**Pros:** Minimal change, preserves timing precision for first ~0.1ms of spin.  
**Cons:** Still wastes some CPU on spinning; 1ms sleep granularity may cause
occasional frame jitter.

### Option B: SDL Vsync Blocking (Correct Fix)

Implement `show_screen()` to actually block on the display refresh using
SDL's renderer vsync:

```c
// src/uae_lib_imp/dummy.cpp (or a new gfx_sdl.cpp)
void show_screen(int monid, int mode) {
    // If SDL renderer has vsync enabled, SDL_RenderPresent blocks
    SDL_RenderPresent(s_renderer);
}
```

**Pros:** Correct architecture — `framewait()` spin loops never execute
because `show_screen()` consumes the frame budget.  
**Cons:** Requires SDL renderer integration; changes the rendering pipeline.

### Option C: Frame-Limit via Sleep (Simplest)

Skip the spin loops entirely and use a single sleep:

```c
// In framewait(), else branch:
frame_time_t elapsed = read_processor_time() - start;
frame_time_t target = vsynctimebase;  // ~20000us at 50Hz
if (elapsed < target) {
    cpu_sleep_millis((int)((target - elapsed) / 1000));
}
// Skip both spin loops
```

**Pros:** Dead simple, eliminates all spinning.  
**Cons:** Loses sub-millisecond precision; `cpu_sleep_millis(1)` has ~1-2ms
jitter on macOS.

---

## 7. Proposed Fix (P2): Reduce Redundant Clock Reads

In `framewait()` else branch, consolidate to 2 reads:

```c
// BEFORE (6 reads):
start = read_processor_time();                        // 12045
if (!frame_rendered && !ad->picasso_on) {
    frame_rendered = crender_screen(0, 1, false);
    t = read_processor_time() - start;                // 12048
}
// ... spin loops ...
idletime += read_processor_time() - start;            // 12068
curr_time = read_processor_time();                    // 12069
vsyncmintime = curr_time;
vsyncmaxtime = vsyncwaittime = curr_time + vstb;
if (frame_rendered) {
    show_screen(0, 0);
    t += read_processor_time() - curr_time;           // 12074
}

// AFTER (2 reads):
start = read_processor_time();                        // single read
if (!frame_rendered && !ad->picasso_on) {
    frame_rendered = crender_screen(0, 1, false);
}
// ... spin loops (with P1 fix) ...
curr_time = read_processor_time();                    // single read
idletime += curr_time - start;                        // reuse curr_time
vsyncmintime = curr_time;
vsyncmaxtime = vsyncwaittime = curr_time + vstb;
if (frame_rendered) {
    show_screen(0, 0);
    // show_screen is no-op, t stays 0
}
t += frameskipt_avg;
```

---

## 8. Expected Impact

| Fix | Estimated CPU Reduction | Status |
|-----|------------------------|--------|
| P0: Direct `mach_absolute_time` | ~25-30% (3-5× per-call speedup) | **Done** |
| P1: Eliminate spin loop | ~10-15% (removes remaining polling) | Proposed |
| P2: Redundant read consolidation | ~1-2% | Proposed |
| **Combined** | **~35-45%** | — |

After all fixes, the emulator's top hot-spots should shift to the expected
distribution: `fill_icache040` and `m68k_run_3ce` as the dominant costs, with
timing overhead reduced to <2% of total CPU.

---

## 9. File Reference

| File | Relevant Code |
|------|---------------|
| `libs/uae_lib/custom.cpp` | `framewait()` at line 11893, spin loops at 12050-12066 |
| `libs/uae_lib/custom.cpp` | `rpt_vsync()` at line 11807 |
| `libs/uae_lib/custom.cpp` | `vsync_display_render()` at line 12272 |
| `libs/uae_lib/include/uae/time.h` | `read_processor_time()` inline at line 21 |
| `libs/uae_lib/events.cpp` | `event_check_vsync()` at line 72 (not active on macOS) |
| `libs/uae_lib/events.cpp` | `do_cycles_slow()` at line 259 |
| `src/uae_lib_imp/time.cpp` | `uae_time()` / `time_us()` — timing implementation (P0 fixed) |
| `src/uae_lib_imp/dummy.cpp` | `show_screen()` no-op stub at line 101 |
| `src/uae_lib_imp/gfx.cpp` | `target_get_display_scanline()` stub at line 27 |
| `src/quasar_app/uae_imp/uae_server_thread.cpp` | Default config at line 82, `syncbase` at line 127 |
| `libs/uae_lib/drawing.cpp` | `isvsync_chipset()` at line 5484 |
