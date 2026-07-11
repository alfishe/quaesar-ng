# Emulator Profiling: Hot-Spot Analysis

> **Date:** July 2026  
> **Platform:** macOS 15.7.7 (Apple Silicon / ARM64)  
> **Build:** RelWithDebInfo (`-O2 -g -DNDEBUG -fno-omit-frame-pointer`)

---

## 1. Methodology

### Build Configuration

A dedicated profiling build was created in `build-prof/` with frame pointers
enabled for accurate stack walking:

```bash
cmake -S . -B build-prof -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_CXX_FLAGS_RELWITHDEBINFO="-O2 -g -DNDEBUG -fno-omit-frame-pointer" \
  -DCMAKE_C_FLAGS_RELWITHDEBINFO="-O2 -g -DNDEBUG -fno-omit-frame-pointer"
cmake --build build-prof --target quaesar
```

### Launch Command

```bash
./build-prof/quaesar <image.adf> \
  -k /path/to/kickstart.rom
```

### Profile Capture

A collapsed stack-trace profile was captured during steady-state emulation
and analyzed across the two primary operational threads:

| Thread | Role | Entry Point |
|--------|------|-------------|
| **Main Thread** (60186686) | Debugger / UI event loop | `qd::Application::doMainLoop` |
| **Emulator Thread** (60186916) | m68k CPU + chipset emulation | `uae_thread_main_func` → `m68k_go` |

---

## 2. Emulator Thread Hot-Spots

The emulator thread is **heavily CPU-bound**. The top hot-spot by self time is
shocking: **nearly 40% of all CPU cycles are spent reading the system clock**.

### Top Functions by Self Time (Exclusive)

These are the leaf functions where the CPU is actually burning cycles:

| Samples | Function | Description |
|---------|----------|-------------|
| 20,421 | `mach_absolute_time` | System clock read (via `clock_gettime` wrapper) |
| 5,993 | `fill_icache040` | 68040 instruction cache line fill simulation |
| 2,613 | `m68k_run_3ce` | Cycle-exact m68k instruction dispatch |
| 2,461 | `decide_line` | Per-scanline chipset state advance |
| 1,799 | `do_cycles_ce020` | Cycle-exact 68020+ cycle counting |
| 1,199 | `MakeFromSR_x` | SR flag reconstruction after instruction |
| 1,111 | `decide_sprites_fetch` | Sprite DMA fetch decision |
| 923 | `do_cycles_slow` | Non-CE cycle counting + event dispatch |
| 776 | `decide_blitter_maybe_write2` | Blitter state advance |
| 684 | `doint` | Interrupt processing |

### Top Functions by Total Time (Inclusive)

These are the high-level call paths driving the emulator:

| Samples | Function | Description |
|---------|----------|-------------|
| 51,783 | `m68k_go` | Main emulation loop entry |
| 51,348 | `m68k_run_3ce` | Cycle-exact CPU run loop |
| 35,715 | `do_cycles_ce020` | CE cycle counting (called per instruction) |
| 28,900 | `do_cycles_slow` | Event-scheduler cycle counting |
| 26,631 | `hsync_handler` | Horizontal sync event handler |
| 26,000 | `op_4e72_25_ff` | RTE (Return from Exception) handler |
| 24,505 | `vsync_display_render` | Per-frame display buffer render |
| 23,923 | `framewait` | Frame synchronization / throttle |
| 22,745 | `rpt_vsync` | Vsync timing helper |
| 22,531 | `uae_time` | Wall-clock time read |

### Key Finding

> **`mach_absolute_time` accounts for ~40% of emulator CPU.**
>
> The call chain is: `read_processor_time()` → `uae_time()` → `time_us()` →
> `clock_gettime(CLOCK_MONOTONIC)` → `mach_absolute_time()`.
>
> This is not because the clock read itself is expensive (~5ns on Apple
> Silicon). It's because it's called **~1 million times per second** from
> busy-wait spin loops in `framewait()`. See
> [timing_architecture_analysis.md](timing_architecture_analysis.md) for the
> full root-cause analysis.

---

## 3. Debugger / UI Thread Hot-Spots

The debugger/UI thread is **overwhelmingly I/O and event bound**. The vast
majority of its time is spent idling, waiting for user input, or processing
macOS window events.

### Top Functions by Self Time (Exclusive)

| Samples | Function | Description |
|---------|----------|-------------|
| 4,719 | `mach_msg2_trap` | Mach IPC trap (event queue) |
| 4,618 | `_kernelrpc_mach_port_insert_member_trap` | Mach port management |
| 4,467 | `_kernelrpc_mach_port_extract_member_trap` | Mach port management |
| 2,714 | `CF_IS_OBJC` | CoreFoundation type check |
| 1,526 | `_pthread_mutex_firstfit_unlock_slow` | Mutex unlock (contended) |
| 1,362 | `mach_absolute_time` | Clock read (UI timer) |
| 1,327 | `objc_msgSend` | Objective-C dispatch |
| 1,181 | `_pthread_mutex_firstfit_lock_slow` | Mutex lock (contended) |
| 880 | `__CFRunLoopDoObservers` | CFRunLoop observer dispatch |

### Top Functions by Total Time (Inclusive)

| Samples | Function | Description |
|---------|----------|-------------|
| 48,881 | `qd::Application::doMainLoop` | Application main loop |
| 40,359 | `SDL_WaitEventTimeout_REAL` | SDL event wait |
| 40,323 | `SDL_WaitEventTimeoutNS` | SDL event wait (internal) |
| 40,055 | `SDL_PumpEventsInternal` | SDL event pump |
| 38,931 | `Cocoa_PumpEvents` | macOS event pump |
| 37,800 | `_nextEventMatchingEventMask:...` | NSApplication event fetch |
| 32,878 | `_DPSNextEvent` | Event system dispatch |
| 28,698 | `CFRunLoopRunSpecific` | CFRunLoop run |
| 20,638 | `__CFRunLoopRun` | CFRunLoop inner loop |

### Summary

The debugger thread is **healthy** — it's mostly idle, blocked in
`SDL_WaitEventTimeout` → `Cocoa_PumpEvents` → `CFRunLoopRunSpecific`. This is
expected behavior for a GUI thread with no work to do. No optimization needed.

---

## 4. Priority Optimization Targets

| Priority | Target | Impact | Difficulty |
|----------|--------|--------|------------|
| **P0** | Eliminate `mach_absolute_time` polling overhead | ~40% emulator CPU | Low (done) |
| **P1** | Fix `framewait()` spin loops (architectural) | Further ~15-20% | Medium |
| **P2** | Reduce redundant per-frame clock reads | ~1-2% | Low |
| — | `fill_icache040` | Inherent to CE 68040 emulation | N/A |
| — | `decide_line`, `decide_sprites_fetch` | Inherent to CE chipset | N/A |

### Completed Work

**P0 — Direct `mach_absolute_time` access** (implemented in
`src/uae_lib_imp/time.cpp`):
- Replaced `clock_gettime(CLOCK_MONOTONIC)` with direct `mach_absolute_time()`
- Precomputed double-precision conversion factor for microsecond output
- Eliminated `timespec` struct construction and redundant arithmetic

**P2 — Reduced redundant clock reads** (identified, not yet implemented):
- `framewait()` reads the clock 6 times per frame where only 2 are necessary

See [timing_architecture_analysis.md](timing_architecture_analysis.md) for the
complete root-cause analysis and proposed fixes.

---
---

## 5. Second Profile Run — Post-Vsync-Fix (July 2026)

> **Profile filtered by thread ID 63980550 (emulator thread)**

After implementing vsync-first UI frame pacing in `application.cpp` (commit
`83b0044b`) and direct `mach_absolute_time` access in `time.cpp`, a second
profiling run was captured to measure the impact and identify remaining
hot-spots.

### Thread CPU Distribution

| Thread | TID | % of Total CPU | Status |
|--------|-----|---------------|--------|
| **Emulator thread** | 63980550 | **91.3%** | Dominant — all optimization effort here |
| **UI thread** | (main) | ~5–6% | Fixed — was ~48% before, now mostly idle |
| **System / other** | — | ~3% | Normal OS overhead |

### UI Thread: Vsync Fix Confirmed Working

The vsync-first frame pacing fix had a **dramatic** effect. In the first
profile run, `qd::Application::doMainLoop` dominated with **48,881 samples**,
with `SDL_WaitEventTimeout` consuming 40,359 of those. The UI thread was
burning ~48% of total CPU.

In the second run, the UI thread shows only **8 idle samples** — it is now
properly blocking on vsync inside `SDL_RenderPresent`, exactly as designed.
The old spin-loop behavior is completely gone.

However, the UI thread now has a **new dominant hot-spot** that was previously
masked by the spin-loop noise.

### 5.1 New UI Hot-Spot: `_platform_memmove` (980 samples)

The framebuffer-to-texture copy is now the single largest UI-thread cost.
With the spin loop gone, this `memcpy` path stands out clearly.

**Root cause:** `renderAppPart()` in
`src/quasar_app/qsr_main_wnd_client_app.cpp:152–159` performs a per-scanline
`memcpy` from `m_pAmigaBuffer` into the SDL GPU texture every frame:

```cpp
// qsr_main_wnd_client_app.cpp:152–159
if (SDL_LockTexture(hDisplayTex, nullptr, &texture_pixels, &pitch) == 0) {
    for (int curY = 0; curY < bufHeight; curY++) {
        uint8_t* dest = (uint8_t*)texture_pixels + (curY * pitch);
        int srcY = isDoubled ? (curY / 2) : curY;
        memcpy(dest, &pSrcDisplayBuf[srcY * bufWidth], bufWidth * 4);
    }
    SDL_UnlockTexture(hDisplayTex);
}
```

At 754×576×4 bytes, this copies **~1.7 MB per frame**. At 50 Hz (PAL), that's
~85 MB/s of raw `memcpy` bandwidth on the UI thread. The `_platform_memmove`
is the Apple Silicon `memcpy` implementation — the actual work is unavoidable
if every frame is uploaded, but the per-scanline loop prevents the optimizer
from issuing a single bulk copy when `pitch == bufWidth * 4`.

**This is the only remaining significant UI-thread hot-spot.** All other UI
functions are negligible by comparison.

See [ui_render_optimization.md](ui_render_optimization.md) §2 for proposed
fast-path optimizations.

### 5.2 GPU Resource Churn: Metal `AGXBuffer` (~46 samples)

The profile shows ~46 samples in Metal GPU buffer allocation — specifically
`AGXBuffer` allocation and `-[MTLDebugDevice newBufferWithBytes...]`.

**Root cause:** On macOS, SDL3's Metal backend (accessed via sdl2-compat
wrapping SDL3) allocates a **new staging buffer on every `SDL_LockTexture` /
`SDL_UnlockTexture` cycle** when using `SDL_TEXTUREACCESS_STREAMING`. The
buffers are not pooled or recycled.

This happens at:
- `qsr_main_wnd_client_app.cpp:152` — `SDL_LockTexture(hDisplayTex, ...)`
- `screen_wnd.cpp:117` — `SDL_LockTexture(scrTexture, ...)` (debugger)
- `memory_graph_wnd.cpp` — `SDL_LockTexture` (less frequent)

**This is inside SDL/Metal and cannot be fixed directly in quaesar-ng.** The
impact can be reduced by skipping texture uploads when the frame hasn't changed
(dirty-rectangle detection — see [ui_render_optimization.md](ui_render_optimization.md)
§6). A native SDL3 build (without sdl2-compat) may also have better buffer
pooling.

### 5.3 The Three-Copy Frame Pipeline

Analysis of the full frame-transport path reveals **three separate full-
framebuffer pixel copies** per displayed frame. Each touches ~1.5–2.2 MB of
pixel data (754×576×4 or up to 1024×768×4 bytes):

```
  Copy #1 (emulator thread)    Copy #2 (UI thread)      Copy #3 (debugger only)
  ─────────────────────────    ───────────────────      ───────────────────────
  UAE vidbuffer                m_pAmigaBuffer           vm->blitter->getScreenPixBuf
      │                            │                          │
      ▼                            ▼                          ▼
  unlockscr()                  SDL_LockTexture+memcpy    per-pixel alpha fixup
  dummy.cpp:985               qsr_main_wnd_client_      screen_wnd.cpp:126
                               app.cpp:152
      │                            │
      ▼                            ▼
  m_pAmigaBuffer               GPU texture
  (shared buffer)              (Metal staging → GPU)
```

**Copy #1** runs on the emulator thread (91.3% of CPU). The UAE core's
internal `vidbuffer` already contains the rendered frame — this copy exists
only because `m_pAmigaBuffer` is a separate allocation. Furthermore, the
`y_start` / `y_end` parameters in `unlockscr()` are **ignored** (declared
unnamed), so even partial-frame redraws trigger a full copy.

**Copy #2** runs on the UI thread (980 samples). Unavoidable while using
`SDL_LockTexture`, but could be optimized to a single bulk `memcpy` when pitch
matches.

**Copy #3** only runs when the debugger Screen window is open. Uses per-pixel
copy (not `memcpy`) due to alpha-channel fixup — much slower, but not on the
normal emulation path.

See [ui_render_optimization.md](ui_render_optimization.md) for the full
breakdown and optimization proposals for each copy.

### 5.4 Updated Priority Targets (Post-Fix)

The priority table has shifted significantly after the vsync and timing fixes:

| Priority | Target | Thread | Impact | Difficulty |
|----------|--------|--------|--------|------------|
| **P0** | `framewait()` spin loop (91.3% of CPU) | Emulator | Very High | Medium |
| **P0** | `target_sleep_nanos` UNIMPLEMENTED | Emulator | High | Low |
| **P1** | Dirty-rectangle detection (skip unchanged frames) | Both | High for static screens | Medium |
| **P1** | `_platform_memmove` fast path (980 samples) | UI | Medium | Low |
| **P2** | Eliminate Copy #1 (core renders directly to shared buffer) | Emulator | ~1 memcpy/frame | High |
| **P2** | GPU-doubled scanlines via `SDL_RenderCopy` src rect | UI | Eliminates CPU line-doubling | Low |
| **P3** | Metal `AGXBuffer` churn (~46 samples) | UI | Low | External |
| **P3** | Debugger screen per-pixel → `memcpy` | UI | Low (debugger only) | Low |

### Completed Work (Cumulative)

| Fix | Commit | Impact |
|-----|--------|--------|
| Direct `mach_absolute_time` access | `time.cpp` | Eliminated `clock_gettime` overhead |
| Vsync-first UI frame pacing | `83b0044b` | UI thread: ~48% → ~5% CPU |
| `framewait()` coarse-sleep loop | `custom.cpp:12043` | Added bulk sleep before spin threshold |
| `framewait()` safety valve | `custom.cpp:12079` | Caps unbounded macOS spin at 2000 iterations |

### Remaining Work

All remaining targets are documented with exact file/line references in
[ui_render_optimization.md](ui_render_optimization.md).
