# Decomposition Analysis of `drawing.cpp`

## Goal Description
The `drawing.cpp` file in UAE is a classic monolithic C++ file (5535 lines, ~168 KB). It handles a massive array of graphics emulation tasks including Playfields (OCS/ECS/AGA), Sprites, HAM, Genlock, palette management, window coordinate tracking, and high-frequency line drawing logic.

Decomposing this file into functionally grouped modules is **highly feasible** and will significantly improve maintainability, compilation times, and readability.

## Structural Analysis
From the file scan, we can identify several distinct conceptual domains currently mixed into the same namespace:

1. **State & Display Geometry (Lines 1-1400+)**
   - Window coordinate mapping, native/Amiga conversions (`res_shift`, `lores_set`).
   - Frame and VBLANK/HBLANK limits (`set_vblanking_limits`, `set_hblanking_limits`).
   - Buffer memory allocations (`xlinebuffer`, `refresh_indicator_buffer`).

2. **Palette & Color Operations**
   - OCS/ECS lookup tables (`xcolors`), AGA 24-bit mappings (`xredcolors`).
   - Color state caching (`colors_for_drawing`).

3. **Sprite Rendering (Lines 1700-1950+)**
   - High-fidelity sprite rendering (`render_sprites`, `sh_render_sprites`, `shsprite`).
   - Sprite buffering (`spritepixels_buffer`).

4. **Line Drawing & Pixel Crunching (Lines 1950-3500+)**
   - The heavily macro/inlined logic for pushing bits to the screen buffer.
   - Routines like `linetoscr_32_sh_spr`, `linetoscr_16_shrink`, `pfield_do_linetoscr`, `fill_line2`.

5. **HAM (Hold-And-Modify) Decoding**
   - Special decoding logic (`decode_ham`, etc.) which has very distinct behavior.

6. **Genlock / Screen-blanking logic**
   - `get_genlock_very_rare_and_complex_case`, `row_map_genlock_buffer`.

## Proposed Architecture (Target Files)

To decompose this file, I propose breaking it down into a central context/header and modular `.cpp` implementations.

### 1. `drawing_internal.h` [NEW]
**Purpose:** Since many static arrays (e.g., `dblpf_ms1`, `spritepixels_buffer`) are currently shared implicitly within the monolith, they will need to be exposed to the modularized `.cpp` files without leaking into the public `drawing.h` API.

### 2. `drawing_geometry.cpp` [NEW]
**Purpose:** Handles display resolution, limit calculations, and blanking areas.
- Functions: `lores_set`, `set_vblanking_limits`, `set_hblanking_limits`, `check_custom_limits`.

### 3. `drawing_palette.cpp` [NEW]
**Purpose:** Handles Amiga-to-Host color space conversion and AGA extensions.
- Functions: Initializing `xcolors`, `xredcolors`, `colors_for_drawing`.

### 4. `drawing_sprites.cpp` [NEW]
**Purpose:** Hardware sprite emulation, multiplexing, and drawing to the line buffer.
- Functions: `render_sprites`, `sh_render_sprites`, `shsprite`.

### 5. `drawing_render.cpp` (or `drawing_pfield.cpp`) [NEW]
**Purpose:** Playfield drawing routines and bitplane crunching. This will house the core `linetoscr_*` and `pfield_do_linetoscr` logic.
- Due to heavy use of `inline` or macros for 16/32 bit color depths, this might stay somewhat large, but isolating it removes the clutter of geometry/sprites.

### 6. `drawing_ham.cpp` [NEW]
**Purpose:** Hold-and-Modify specific routines.

## Refactoring Strategy

Since you requested *only an analysis and proposal*, no changes have been applied. If we were to execute this:
1. Extract types and shared static globals into `drawing_internal.h`.
2. Move functions into their respective `.cpp` files in chunks, verifying the build at each step.
3. Update `CMakeLists.txt` in `libs/uae_lib/` to compile the new files.

> [!WARNING]
> **Performance Hazard**
> Many functions in `drawing.cpp` (like `shsprite` or `linetoscr_*`) heavily rely on compiler inlining and static variables for the massive performance requirements of cycle-accurate emulation. Moving them across compilation units (without LTO) could cause slight performance regressions unless they are explicitly placed in headers (`drawing_render_inline.h`) or we rely strictly on modern link-time optimization (LTO).

## Open Questions

1. Do you want to proceed with this physical file separation?
2. Are you comfortable with exposing some previously `static` variables into an internal header (`drawing_internal.h`), or would you prefer encapsulating them into a `DrawingState` class?
3. Should we keep the rendering functions in `.cpp` files, or use a template/header-only approach for the high-performance pixel loops to guarantee inlining?
