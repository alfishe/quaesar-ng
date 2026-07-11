# UI Render & Frame-Transport Optimization Targets

> **Date:** July 2026  
> **Platform:** macOS 15.7.7 (Apple Silicon / ARM64)  
> **Build:** RelWithDebInfo (`-O2 -g -DNDEBUG -fno-omit-frame-pointer`)  
> **Related:** [profiling_hotspots.md](profiling_hotspots.md) · [timing_architecture_analysis.md](timing_architecture_analysis.md)

---

## Executive Summary

Post-vsync-fix profiling shows the **emulator thread (TID 63980550)** consuming
**91.3% of total CPU**. The remaining ~8.7% is split between the UI thread and
system overhead.

The frame pipeline has **three full-framebuffer pixel copies** per displayed
frame, plus an idle spin loop in `framewait()` that was previously masked by the
UI thread's own spin loop. With the UI loop fixed, these are now the dominant
cost centers.

```
Emulator Core                ServerThread (emu thread)          UI Thread
─────────────                ──────────────────────             ─────────
render frame                                                   renderAppPart()
  │                                                              │
  ├─① unlockscr()          ② lockDisplayTexBuf()                ③ SDL_LockTexture()
  │  memcpy row-by-row  ←──┤  hands off m_pAmigaBuffer      ───→  memcpy row-by-row
  │  into m_pAmigaBuffer   │                                      into GPU texture
  │                        │                                  SDL_RenderPresent()
  └─ framewait() spin      unlockDisplayTexBuf()
```

Three separate `memcpy` passes over the full framebuffer, each touching
~1.5–2.2 MB of pixel data (754×576×4 or up to 1024×768×4 bytes).

---

## 1. Pixel Copy #1 — UAE Core → `m_pAmigaBuffer`

**File:** `src/uae_lib_imp/dummy.cpp:985–1004`  
**Function:** `unlockscr(struct vidbuffer*, int, int)`

```cpp
void unlockscr(struct vidbuffer* vb_in, int, int) {
    struct vidbuffer* vb = avidinfo->gfxvidinfo.outbuffer;
    const int imgSizeX = vb->outwidth;
    const int imgSizeY = vb->outheight;
    uint8_t* sptr = vb->bufmem;
    if (uint32_t* pDestTxBuf = qsr_lockUaeScreenTexBuf(imgSizeX, imgSizeY)) {
        for (int y = 0; y < imgSizeY; y++) {
            uint8_t* dest = (uint8_t*)&pDestTxBuf[y * imgSizeX];
            memcpy(dest, sptr, imgSizeX * 4);   // ← per-scanline copy
            sptr += vb->rowbytes;
        }
        qsr_unlockUaeScreenTexBuf();
    }
}
```

**Problem:** Full-frame `memcpy` every frame, even if only a few scanlines
changed. The UAE core's internal `vidbuffer` already has the rendered frame —
this copy exists only because `m_pAmigaBuffer` is a separate allocation owned by
`UaeServerThread`.

**Call chain:**  
`vsync_handle_redraw` → `unlockscr(vb, y_start, y_end)` — note that `y_start`
and `y_end` parameters are **ignored** (declared as unnamed), so even partial
redraw triggers a full copy.

**Lock counterpart:**  
`UaeServerThread::_lockUaeScreenTexBuf()` at
`src/quasar_app/uae_imp/uae_server_thread.cpp:213–224` — acquires
`m_UaeScrTextureMutex` and may `delete[]`/re-`new` the buffer if dimensions
changed.

**Optimization ideas:**
- **A.** Pass `y_start`/`y_end` through and copy only the dirty scanline range.
- **B.** Eliminate this copy entirely: have the UAE core render directly into
  `m_pAmigaBuffer` by pointing `vidbuffer.bufmem` at it.
- **C.** If `vb->rowbytes == imgSizeX * 4` (contiguous), replace the per-scanline
  loop with a single `memcpy` of the full buffer.

**Estimated impact:** Medium — runs on the emulator thread (91.3% of CPU), but
is a single `memcpy` pass (~0.5–1 ms on Apple Silicon).

---

## 2. Pixel Copy #2 — `m_pAmigaBuffer` → SDL GPU Texture

**File:** `src/quasar_app/qsr_main_wnd_client_app.cpp:148–164`  
**Function:** `QsrMainClientWndApp::renderAppPart()`

```cpp
SDL_Texture* hDisplayTex = tryRecreateEmuScreenTexture(bufWidth, bufHeight);
void* texture_pixels = nullptr;
int pitch = 0;
if (SDL_LockTexture(hDisplayTex, nullptr, (void**)&texture_pixels, &pitch) == 0) {
    for (int curY = 0; curY < bufHeight; curY++) {
        uint8_t* dest = (uint8_t*)texture_pixels + (curY * pitch);
        int srcY = isDoubled ? (curY / 2) : curY;
        memcpy(dest, &pSrcDisplayBuf[srcY * bufWidth], bufWidth * 4);
    }
    SDL_UnlockTexture(hDisplayTex);
}
SDL_RenderCopy(m_hWndRenderer, hDisplayTex, nullptr, &rect);
```

**Problem:** This is the **`_platform_memmove` (980 samples)** hot spot from the
profile. Per-scanline `memcpy` from the shared `m_pAmigaBuffer` into the GPU
texture's locked region.

The `isDoubled` path (when `bufHeight < 350`, i.e. PAL 256-line modes)
duplicates each source scanline, making it impossible to use a single bulk
`memcpy`. But for non-doubled (laced/high-res) modes, the per-scanline loop is
pure overhead — `pitch` may differ from `bufWidth * 4`, but when they match a
single `memcpy` would suffice.

**Texture access mode:**  
`SDL_TEXTUREACCESS_STREAMING` — confirmed correct for frequent updates  
(`qsr_main_wnd_client_app.cpp:75`).

**Frame-skip guard already present:**  
`qsr_main_wnd_client_app.cpp:107–109` — skips render when
`curFrame == m_renderedFrameNo`. This is working correctly.

**Optimization ideas:**
- **A.** Fast path for non-doubled mode: single `memcpy(texture_pixels, src, bufHeight * pitch)` when `pitch == bufWidth * 4`.
- **B.** Use `SDL_UpdateTexture()` instead of `SDL_LockTexture`/`SDL_UnlockTexture` — on some backends this avoids a full GPU staging buffer lock.
- **C.** For the doubled case, use `SDL_RenderCopy` with `src` rect at half height and let the GPU scale — eliminates the CPU-side line doubling entirely.

**Estimated impact:** High — this is the single largest UI-thread hot spot
(980 samples). Even though the UI thread is now ~8% of total, removing this copy
frees the UI thread to never block on texture uploads.

---

## 3. Pixel Copy #3 — Debugger Screen Window (Per-Pixel)

**File:** `libs/amDebugger/src/amDebugger/window/screen_wnd.cpp:112–141`  
**Function:** `ScreenWnd::grabScreenToTexture()`

```cpp
void* scrBuf = vm->blitter->getScreenPixBuf(0, &vbSizeX, &vbSizeY, &vbPitch);
for (int y = 0; y < scrSizeY; y++) {
    uint8_t* sptr = (uint8_t*)scrBuf + (y * vbPitch);
    uint32_t* dest = ((uint32_t*)pixels) + (y * scrSizeX);
    for (int x = 0; x < scrSizeX; ++x) {
        qd::Color c = *(uint32_t*)(sptr);
        c.a = 255;          // ← alpha fixup forces per-pixel
        *dest = c;
        ++dest;
        sptr += 4;
    }
}
```

**Problem:** Per-pixel copy (not per-scanline `memcpy`) because it forces
`alpha = 255` on every pixel. This is orders of magnitude slower than a
`memcpy`.

**Only active when debugger Screen window is open** — not on the hot path
during normal emulation. But if the user leaves the debugger open while
emulation runs, this re-executes every frame.

**Optimization ideas:**
- **A.** If the source buffer already has `alpha = 255` (or if the blend mode
  is `SDL_BLENDMODE_NONE`), replace with a single `memcpy`.
- **B.** Use SSE/NEON to set alpha in bulk: mask with `0x00FFFFFF` then OR with
  `0xFF000000` — 4 pixels at a time on ARM.
- **C.** Set `SDL_BLENDMODE_NONE` on the texture and skip alpha fixup entirely.

**Estimated impact:** Low (only when debugger Screen window is open).

---

## 4. `target_sleep_nanos` — UNIMPLEMENTED

**File:** `src/uae_lib_imp/dummy.cpp:1847–1850`

```cpp
int target_sleep_nanos(int) {
    UNIMPLEMENTED();
    return 0;
}
```

**Called from:** `libs/uae_lib/custom.cpp:13222, 13234` — inside
`scanlinesleep()`, which is part of the per-scanline timing loop.

Currently this function aborts on debug builds and silently returns 0 on
release. The `UNIMPLEMENTED()` macro logs a warning and may trigger a debugger
break. On release builds, returning 0 means "don't sleep" — contributing to the
emulator thread's high CPU usage, since `scanlinesleep()` never actually yields.

**Fix:** Implement with `nanosleep` or `clock_nanosleep`:

```cpp
int target_sleep_nanos(int nanos) {
    if (nanos <= 0) return 0;
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = nanos;
    nanosleep(&ts, nullptr);
    return 0;
}
```

**Guard with `#ifndef _WIN32`** — Windows has its own implementation in
`libs/uae_lib/od-win32/win32.cpp:292`.

**Estimated impact:** Medium — reduces emulator thread CPU by allowing
scanline-level sleep instead of spin.

---

## 5. `framewait()` Spin Loop (Emulator Thread — 91.3% of CPU)

**File:** `libs/uae_lib/custom.cpp:11893–12106`  
**Function:** `framewait()`

This is the frame synchronization barrier. The relevant code path (non-VSYNC,
non-throttle) is the `else` branch at line 12033:

```cpp
// custom.cpp:12043–12083
int spin_count = 0;
while (!currprefs.turbo_emulation) {
    float v = rpt_vsync(clockadjust) / (syncbase / 1000.0f);
    if (v >= -FRAMEWAIT_MIN_MS)   // FRAMEWAIT_MIN_MS = 4
        break;
    int sleep_time = (int)(-v) - FRAMEWAIT_MIN_MS;
    if (sleep_time > 1) {
        if (cpu_sleep_millis(sleep_time) < 0)
            break;
    } else {
        if (cpu_sleep_millis(1) < 0)
            break;
    }
}
while (rpt_vsync(clockadjust) < 0) {    // ← high-precision spin
    rtg_vsynccheck();
    if (++spin_count > 2000) {           // safety valve
        sleep_millis(0);
        spin_count = 0;
    }
}
```

**Constants:** `custom.cpp:55–56`
```cpp
#define FRAMEWAIT_MIN_MS 4    // switch from sleep to spin when within 4ms
#define FRAMEWAIT_SPLIT 4     // vsynctimeperline divisor
```

**Problem:** The first loop sleeps in 1ms increments until within 4ms of the
frame deadline. The second loop busy-spins for the final 4ms. Each iteration
calls `rpt_vsync()` → `read_processor_time()` → `uae_time()` → `mach_absolute_time()`.

The safety valve (spin_count > 2000) caps unbounded spinning on macOS where
`show_screen()` is a no-op stub.

**Already analyzed in depth:**  
[timing_architecture_analysis.md](timing_architecture_analysis.md) — see
sections on `show_screen()` no-op and proposed `display_vblank` shim.

**Optimization ideas:**
- **A.** Implement `show_screen()` to block on a real vsync signal (CVDisplayLink on macOS).
- **B.** Increase `FRAMEWAIT_MIN_MS` from 4 to 6–8 to reduce spin time at the cost of frame timing jitter.
- **C.** Replace the spin loop with `clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, ...)` for high-precision sleep without spinning.

**Estimated impact:** Very high — this is the emulator thread's primary
non-emulation CPU consumer (23,923 total samples in prior profile).

---

## 6. No Dirty-Rectangle / Partial-Frame Detection

**Finding:** There is **no dirty-rectangle or frame-changed detection** in the
pixel transport pipeline. The UAE core has internal dirty tracking for CPU
cache lines (`c->dirty[j][k]` in `newcpu.cpp:1634`) and filesystem nodes
(`aino->dirty` in `filesys.cpp`), but **nothing for the display buffer**.

The frame counter (`m_scrFrameNo`, atomic) only signals "a new frame was
produced" — it says nothing about which regions changed.

Every frame, regardless of whether the Amiga screen actually changed (e.g.
static Workbench), goes through the full three-copy pipeline.

**Where dirty tracking could be added:**
- The UAE core's drawing code (`libs/uae_lib/drawing.cpp`) processes scanlines
  via `vsync_handle_redraw` (`drawing.cpp:5122`). The `lof_changed` and
  `drawlines` parameters indicate structural changes, but not pixel-level
  changes.
- A bitfield of "dirty scanline ranges" could be maintained in `vidbuffer` and
  passed through `unlockscr(vb, y_start, y_end)` — these parameters already
  exist but are currently ignored.

**Estimated impact:** Very high for static screens (Workbench idle, file
dialogs). Zero impact for animation-heavy scenes. Most usage involves long
periods of static screen content.

---

## 7. GPU Resource Churn — Metal Backend

**Symptom:** Profile shows ~46 samples in `AGXBuffer` allocation /
`-[MTLDebugDevice newBufferWithBytes...]` — Metal GPU buffer allocation
happening per-frame.

**Root cause:** `SDL_LockTexture` on the SDL3 Metal backend (via sdl2-compat
wrapping SDL3 on macOS) allocates a new staging buffer for each lock/unlock
cycle when using `SDL_TEXTUREACCESS_STREAMING`. The buffer is not pooled.

**Where it happens:**  
`qsr_main_wnd_client_app.cpp:152` — `SDL_LockTexture(hDisplayTex, ...)`  
`screen_wnd.cpp:117` — `SDL_LockTexture(scrTexture, ...)`  
`memory_graph_wnd.cpp` — `SDL_LockTexture` (less frequent)

**This is inside SDL/Metal and cannot be fixed in quaesar-ng directly.**
However, the impact can be reduced by:
- Skipping texture upload entirely when the frame hasn't changed (see §6).
- Upgrading to a native SDL3 build (not sdl2-compat) which may have better
  buffer pooling.

---

## 8. VAmiga Backend — Zero-Copy Path (For Comparison)

**File:** `libs/vAmiga_imp_lib/src/qvAmigaImp/va_server_thread.cpp:304–317`

The VAmiga backend does **not** copy pixels at all — it directly assigns the
vAmiga texture buffer pointer:

```cpp
const uint32_t* pCurDisplayTexBuf = vVideoPort.getTexture(&nr, &lof, &prevlof);
m_pAmigaBuffer = const_cast<uint32_t*>(pCurDisplayTexBuf);
```

This eliminates copy #1 entirely. The UI thread still performs copy #2
(`m_pAmigaBuffer` → SDL texture) via the same `lockDisplayTexBuf()` interface.

---

## Priority Summary

| Priority | Target | Location | Impact | Difficulty |
|----------|--------|----------|--------|------------|
| **P0** | `framewait()` spin loop | `custom.cpp:12043–12083` | ~40% emu CPU | Medium |
| **P0** | `target_sleep_nanos` UNIMPLEMENTED | `dummy.cpp:1847` | Emu CPU + crashes | Low |
| **P1** | Dirty-rectangle detection (skip unchanged frames) | `dummy.cpp:985` + `qsr_main_wnd_client_app.cpp:101` | Eliminates copies #1–3 for static screens | Medium |
| **P1** | Texture upload fast path (bulk `memcpy`) | `qsr_main_wnd_client_app.cpp:152` | 980 UI samples | Low |
| **P2** | Eliminate copy #1 (core renders directly to `m_pAmigaBuffer`) | `dummy.cpp:985` | ~1 emu-thread `memcpy`/frame | High |
| **P2** | GPU-doubled scanlines via `SDL_RenderCopy` src rect | `qsr_main_wnd_client_app.cpp:153` | Eliminates CPU line-doubling | Low |
| **P3** | Debugger screen per-pixel → `memcpy` | `screen_wnd.cpp:130` | Only when debugger open | Low |
| — | Metal AGXBuffer churn | SDL3 backend (external) | ~46 samples | External |

---

## Appendix A: Texture Inventory

| Texture | File | Pixel Format | Access Mode | Blend | Scale |
|---------|------|-------------|-------------|-------|-------|
| Main emu display | `qsr_main_wnd_client_app.cpp:75` | `ARGB8888` | `STREAMING` | default | default |
| Debugger screen | `screen_wnd.cpp:98` | `ARGB8888` | `STREAMING` | `BLEND` | `Linear` |
| Memory graph | `memory_graph_wnd.cpp:30` | `ARGB8888` | `STREAMING` | `BLEND` | `Linear` |
| ImGui font | `imgui_impl_sdlrenderer2.cpp:236` | `RGBA32` | `STATIC` | — | — |

All emu-related textures correctly use `STREAMING`. No changes needed.

## Appendix B: Frame-Transport Call Graph

```
Emulator Thread (TID 63980550 — 91.3% CPU):
  m68k_go()
    └─ vsync_handler_post()           custom.cpp:12220
         ├─ vsync_handle_redraw()     drawing.cpp:5122
         │    └─ crender_screen()
         ├─ framewait()               custom.cpp:11893  ← P0 spin loop
         │    ├─ rpt_vsync() → read_processor_time() → uae_time()
         │    │                        time.cpp:28
         │    └─ show_screen()        dummy.cpp (no-op on macOS)
         └─ unlockscr()               dummy.cpp:985     ← Copy #1
              └─ qsr_lockUaeScreenTexBuf()
                   └─ UaeServerThread::_lockUaeScreenTexBuf()
                        uae_server_thread.cpp:213

UI Thread (~8% CPU):
  doMainLoop()                        application.cpp:85
    └─ onFrameRender() → renderAppPart()
         qsr_main_wnd_client_app.cpp:101
         ├─ getScrFrameNo() check     line 107  (frame-skip guard)
         ├─ lockDisplayTexBuf()       line 121
         ├─ SDL_LockTexture + memcpy  line 152  ← Copy #2 (980 samples)
         └─ SDL_RenderPresent()       line 171
```

---
---

## 9. Screen Path Consolidation: Toward Zero-CPU-Copy

### 9.1 The Two Independent Screen Paths (Current State)

The project has **two completely independent screen capture paths** that do not
share any buffer, synchronization, or texture:

| Consumer | Source buffer | Copy method | Thread sync | Frame guard |
|----------|-------------|-------------|-------------|-------------|
| **Main window** `renderAppPart()` | `m_pAmigaBuffer` (a *snapshot copy* of UAE's vidbuffer) | Per-scanline `memcpy` (`qsr_main_wnd_client_app.cpp:152`) | `m_UaeScrTextureMutex` | Yes (`m_renderedFrameNo` check at line 107) |
| **Debugger** `ScreenWnd::grabScreenToTexture()` | UAE's **raw internal** `vidbuffer.bufmem` | **Per-pixel** with alpha fixup (`screen_wnd.cpp:126`) | **NONE!** | **NONE!** |

The debugger bypasses `m_pAmigaBuffer` entirely. It calls
`vm->blitter->getScreenPixBuf()` which on the UAE backend returns a **direct
pointer into the emulator core's internal render buffer** — no mutex, no frame
boundary guarantee, no copy. This is a **race condition**: the emulator thread
can be mid-write into `vb->bufmem` when the debugger reads it.

This explains the debugger screen **lagging a few frames**: it captures whatever
is in the buffer at the moment ImGui draws, which may be a half-rendered frame.

### 9.2 Cross-Platform Backend Analysis

The path to consolidation differs significantly by platform because the UAE
core's `vidbuffer` allocation strategy changes:

**Windows (win32gfx.cpp:3919, 1463):**
- When using Direct3D, `vb->bufmem` is obtained via `D3D_locktexture()` — it
  points **directly into a locked GPU texture**.
- `unlockscr()` then unlocks the D3D texture. This is already near-zero-copy:
  the UAE core renders into a GPU-mapped texture directly.
- Consolidation here means: share the D3D texture handle between main window
  and debugger, no CPU copies at all.

**macOS (our build — drawing.cpp:5372):**
- `vb->bufmem = xcalloc(...)` — a regular heap allocation. No GPU mapping.
- The UAE core renders pixels into this heap buffer.
- Then `unlockscr()` (`dummy.cpp:985`) copies to `m_pAmigaBuffer`.
- Then `renderAppPart()` copies to SDL texture. Two CPU copies.
- SDL_Renderer backend is **Metal** (via sdl2-compat → SDL3).

**Linux (same code path as macOS):**
- Same `xcalloc` allocation in `drawing.cpp:5372`. Two CPU copies.
- SDL_Renderer backend is typically **OpenGL** or **Vulkan** depending on
  SDL build and desktop compositor.

| Platform | vidbuffer location | Backend | CPU copies | GPU texture sharing |
|----------|-------------------|---------|-----------|-------------------|
| **Windows (D3D)** | GPU texture (locked) | Direct3D 11 | **0** | Share D3D texture handle |
| **Windows (SDL)** | Heap allocation | D3D/OpenGL via SDL | 2 | Share via SDL_Texture |
| **macOS** | Heap allocation | Metal via SDL | 2 | Share via SDL_Texture |
| **Linux** | Heap allocation | OpenGL/Vulkan via SDL | 2 | Share via SDL_Texture |

### 9.3 Proposed Consolidation: Unified `lockDisplayTexBuf` Path

The key insight: **`lockDisplayTexBuf()` / `unlockDisplayTexBuf()` already
provides the correct synchronization contract** — mutex-protected, frame-
boundary snapshot of the latest complete frame in `m_pAmigaBuffer`.

Both consumers should use it. The debugger must stop accessing raw
`vb->bufmem` directly.

```
CURRENT (3 separate paths):

  Emulator Thread:
    UAE core ──render──▶ vb->bufmem (heap)
                          │
                    ① unlockscr() memcpy ──▶ m_pAmigaBuffer (under mutex)
                                                    ▲
  UI Thread:                                       │
    Main window ─── lockDisplayTexBuf() ──────────┘
                  ② memcpy ──▶ SDL_Texture ──▶ GPU

    Debugger ───── getScreenPixBuf() ──▶ vb->bufmem (RAW, UNSYNC!)
                  ③ per-pixel copy ──▶ SDL_Texture ──▶ GPU


PROPOSED (1 shared snapshot, GPU handles scaling):

  Emulator Thread:
    UAE core ──render──▶ vb->bufmem (heap)
                          │
                    ① unlockscr() memcpy ──▶ m_pAmigaBuffer (under mutex)
                                                    ▲           ▲
  UI Thread:                                       │           │
    Main window ─── lockDisplayTexBuf() ──────────┘           │
                  ② SDL_UpdateTexture ──▶ GPU Texture          │
                                                                │
    Debugger ───── lockDisplayTexBuf() ────────────────────────┘
                  ②' SDL_UpdateTexture ──▶ GPU Texture
                  (or: share main window's texture directly)
```

Copy #3 is **eliminated** — the debugger reads from the same mutex-protected
snapshot as the main window. The race condition is fixed.

### 9.4 Implementation Steps

#### Step 1: Debugger uses `lockDisplayTexBuf` instead of raw `getScreenPixBuf`

**File:** `libs/amDebugger/src/amDebugger/window/screen_wnd.cpp`

Replace `grabScreenToTexture()` to use the IVmClientPlayer mutex-protected path.
The debugger needs access to the `IVmClientPlayer` (or at minimum, a method on
`IVm::VM` that provides the same synchronized snapshot).

**Problem:** `ScreenWnd` currently accesses screen data via
`vm->blitter->getScreenPixBuf()` — this is on the `IVm::VM` interface and
returns unsynchronized raw memory. The `lockDisplayTexBuf()` method is on
`IVmClientPlayer` (`qsr_app_interfaces.h:18`), which the debugger does not
currently have access to.

**Options:**
- **A.** Add a `lockFrameBuffer()` / `unlockFrameBuffer()` method to `IVm::VM`
  that wraps the same mutex-protected `m_pAmigaBuffer`. Both main window and
debugger use this path. Clean interface boundary.
- **B.** Pass `IVmClientPlayer*` to the debugger. Breaks the debugger/backend
  separation.
- **C.** Have the debugger use `getScreenPixBuf()` but fix the UAE backend to
  return `m_pAmigaBuffer` (not raw `vb->bufmem`) and add mutex locking inside
  `getScreenPixBuf`.

**Recommended:** Option **A** — add synchronized framebuffer access to `IVm::VM`.

#### Step 2: Main window fast-path: bulk `memcpy` when contiguous

**File:** `src/quasar_app/qsr_main_wnd_client_app.cpp:152–161`

When `isDoubled == false` and `pitch == bufWidth * 4`, replace the per-scanline
loop with a single bulk `memcpy`:

```cpp
if (!isDoubled && pitch == bufWidth * 4) {
    // Fast path: single contiguous copy
    memcpy(texture_pixels, pSrcDisplayBuf, (size_t)bufHeight * pitch);
} else {
    // Slow path: per-scanline copy (line doubling or pitch mismatch)
    for (int curY = 0; curY < bufHeight; curY++) { ... }
}
```

This is a pure optimization — same behavior, fewer loop iterations, lets the
optimizer issue wide SIMD copies.

#### Step 3: GPU-side line doubling (eliminate CPU scanline duplication)

**File:** `src/quasar_app/qsr_main_wnd_client_app.cpp:127–131, 153–158`

Currently when `bufHeight < 350` (PAL 256-line modes), the code doubles the
height and manually duplicates each scanline in the CPU copy loop. Instead:

- Upload only the original `bufHeight` scanlines to the texture
- Use `SDL_RenderCopy` with a source rect of `{0, 0, bufWidth, origHeight}`
  and destination rect of `{0, 0, new_width, new_height}` — the GPU scales
  vertically for free.

This eliminates the `isDoubled` branch entirely and halves the texture upload
size for PAL modes.

#### Step 4: Consider `SDL_UpdateTexture` instead of `LockTexture`/`UnlockTexture`

`SDL_UpdateTexture` takes a source pointer and pitch, and handles the GPU upload
internally. On some backends (particularly OpenGL/Vulkan) this avoids allocating
a staging buffer per lock/unlock cycle.

```cpp
// Instead of:
SDL_LockTexture(hDisplayTex, nullptr, &texture_pixels, &pitch);
memcpy(...);
SDL_UnlockTexture(hDisplayTex);

// Use:
SDL_UpdateTexture(hDisplayTex, nullptr, pSrcDisplayBuf, bufWidth * 4);
```

**Caveat:** This only works when the source buffer is contiguous and the pitch
matches. For the doubled case or pitch mismatch, `LockTexture` is still needed.
Also, `SDL_UpdateTexture` may internally copy into a staging buffer anyway,
so the benefit is backend-dependent.

### 9.5 Why True Zero-CPU-Copy Is Impossible on macOS/Linux (SDL2)

On Windows with Direct3D, the UAE core's `vidbuffer.bufmem` can point directly
into a locked GPU texture (`D3D_locktexture` in `win32gfx.cpp:1463`). The UAE
core's pixel output goes straight to GPU memory. No CPU copy needed.

On macOS/Linux, SDL2's `SDL_LockTexture` / `SDL_UpdateTexture` always requires
a **CPU-accessible staging buffer** that is then uploaded to GPU memory via the
driver. The Metal backend (`AGXBuffer`) and OpenGL backend (`glTexSubImage2D`)
both internally perform a DMA transfer from the staging buffer to GPU texture
memory. The CPU must populate that staging buffer first.

Therefore, on macOS/Linux we will always need **at least one CPU `memcpy`** to
get pixels from `m_pAmigaBuffer` into the SDL texture staging region.

The consolidation goal is:
- **3 CPU copies → 1 CPU copy** (for both main window + debugger combined)
- **1 GPU blit/scale** (via `SDL_RenderCopy`, free)
- **0 race conditions** (all consumers use mutex-protected path)

### 9.6 Consolidated Outcome Per Platform

| Platform | Before | After | Notes |
|----------|--------|-------|-------|
| **Windows (D3D)** | 0 copies (already optimal) | 0 copies | Share D3D texture handle to debugger |
| **Windows (SDL)** | 3 CPU copies | 1 CPU copy | Same SDL path as macOS |
| **macOS** | 3 CPU copies | 1 CPU copy | Single `SDL_UpdateTexture`, GPU scaling |
| **Linux** | 3 CPU copies | 1 CPU copy | Single `SDL_UpdateTexture`, GPU scaling |

### 9.7 VAmiga Backend: Already Near-Optimal

The VAmiga backend (`va_server_thread.cpp:304–317`) already avoids copy #1 by
directly assigning the vAmiga texture buffer pointer:

```cpp
m_pAmigaBuffer = const_cast<uint32_t*>(pCurDisplayTexBuf);
```

No `memcpy` on the emulator thread. The UI thread still performs copy #2, but
that's the unavoidable SDL texture upload. If the debugger is also switched to
`lockDisplayTexBuf()`, the VAmiga backend achieves the 1-copy goal with no
further changes.

---
---

## 10. Multi-Window Vsync Pacing

### 10.1 Problem: Dual-Vsync Halves Frame Rate

The application has two SDL windows rendered in a single main-loop iteration:

1. **Main window** (`QsrMainClientWnd`) — displays the emulator screen via SDL texture
2. **Debugger window** (`DebuggerApp`) — ImGui debugger UI with screen preview

The main loop calls `updateAppPart` + `renderAppPart` for **both** windows
sequentially each iteration:

```
iteration:
  updateAppPart(main)      → renderAppPart(main)      → SDL_RenderPresent [vsync block]
  updateAppPart(debugger)   → renderAppPart(debugger)   → SDL_RenderPresent [vsync block]
```

**Bug:** Both SDL renderers were created with `SDL_RENDERER_PRESENTVSYNC`.
Each `SDL_RenderPresent` call blocks for a full vsync period (~16.6ms at 60Hz).
Two independent vsync waits per iteration = **~33ms total** = **~30fps effective
loop rate**.

The emulator produces frames at 50Hz (PAL) or 60Hz (NTSC). At a 30fps loop rate,
half the emulator frames are never displayed — the debugger screen preview
appeared to run at half the framerate of the main window.

### 10.2 Fix: Single-Vsync Authority

**Rule: Only one renderer in the application may request `SDL_RENDERER_PRESENTVSYNC`.**

The main window renderer keeps vsync (it paces the entire loop). The debugger
renderer uses `SDL_RENDERER_ACCELERATED` only — no vsync:

```
iteration:
  updateAppPart(main)      → renderAppPart(main)      → SDL_RenderPresent [vsync block ~16.6ms]
  updateAppPart(debugger)   → renderAppPart(debugger)   → SDL_RenderPresent [returns immediately]
```

**Result:** One vsync wait per iteration = ~16.6ms = **60fps loop rate**.
Both windows can capture all 50 emulator frames.

| File | Renderer flags | Role |
|------|----------------|------|
| `qsr_main_wnd_client_app.cpp:67` | `PRESENTVSYNC \| ACCELERATED` | **Sole vsync authority** — paces the loop |
| `debuggerWndApp.cpp:97` | `ACCELERATED` (no vsync) | Debugger — RenderPresent returns instantly |
