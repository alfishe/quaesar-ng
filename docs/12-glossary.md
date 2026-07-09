# 12 — Glossary

← [Threading Model](11-threading-model.md) · [Index](index.md)

Quick reference for the abbreviations, namespaces, and Amiga-specific terms used
throughout this documentation and the codebase.

## Namespaces & prefixes

| Term | Meaning |
|------|---------|
| `qsr::` | **Q**uaesar application code (`src/quasar_app/`). E.g. `qsr::QuaesarApplication`, `qsr::IVmClientPlayer`. |
| `qd::` | The **qd** foundation library (`libs/qd/`). Application framework, containers, ImGui glue. |
| `qtd::` | EASTL-wrapper aliases defined by `qd/stl/` (`qtd::string`, `qtd::vector`, ...). Prefer these over raw EASTL/`std`. |
| `amD::` | **am****D**ebugger (`libs/amDebugger/`). The debugger UI + the `IVm` abstraction. |
| `IVm::` | The **virtual-machine abstraction** namespace. `IVm::VM` is the central snapshot interface. |
| `IVm::imp::` | Concrete backend impls of `IVm::VM` (`UaeVmImp`, `VAmVmImp`). |
| `vamiga::` | The vAmiga core namespace (`libs/vAmiga/Core/`). |

## Key types & interfaces

| Type | Where | What |
|------|-------|------|
| `IVm::VM` | `amDebugger/vm/vmInterface.h` | Abstract snapshot of the machine — **the seam**. See [doc 03](03-vm-abstraction-boundary.md). |
| `IVm::Memory/Cpu/CustomRegs/Copper/Blitter/Emu/Floppy` | same | Sub-modules of `IVm::VM`. |
| `qsr::IVmClientPlayer` | `qsr_app_interfaces.h` | UI↔emu thread surface (`getVm`, `pushOperationMsg`, `lockDisplayTexBuf`, ...). |
| `amD::IVmDbgServiceBridge` | `dbgConnection.h` | Hands the debugger a client/server `IVm::VM`. |
| `qd::Application` / `ApplicationPart` | `qd/app/` | Main loop owner / pluggable subsystem. |
| `qd::IOperationEnvironment` / `BaseOpArgs` | `qd/qui/` | The operations command bus. See [doc 04](04-operation-dispatch.md). |
| `qsr::BaseVmServerAppPart` | `qsr_application.h` | Base class for `UaeServerAppPart` / `VAmServerAppPart`. |
| `UaeServerThread` / `VAmServerThread` | `uae_imp/` / `vAmiga_imp_lib/` | The emulation-thread facade (impls of `IVmClientPlayer`). |

## Amiga / emulator terms

| Term | Meaning |
|------|---------|
| **UAE** | **U**nix **A**miga **E**mulator — the family WinUAE/FS-UAE/E-UAE belong to. Quaesar's primary core. |
| **WinUAE** | The Windows reference emulator; the upstream of `libs/uae_lib`. |
| **FS-UAE** | A POSIX port of WinUAE; the basis of Quaesar's POSIX adaptation (`-DFSUAE`). |
| **vAmiga** | A modern C++ rewrite of the Amiga (`libs/vAmiga/`, target `VACore`). Quaesar's second core. |
| **Moira** | The 68000 CPU core used inside vAmiga (`libs/vAmiga/Core/Components/CPU/Moira/`). |
| **Kickstart** | The Amiga ROM (`-k kick13.rom`). Required to boot. |
| **ADF** | Amiga Disk File — the standard floppy image format. |
| **OCS / ECS / AGA** | Original / Enhanced / Advanced Graphics Architecture — the three chipset generations. |
| **Agnus / Denise / Paula** | The three custom chips (DMA+video timing / video / audio+floppy). |
| **Copper** | The Amiga's coprocessor that drives the display list ("copper list"). |
| **Blitter** | The Amiga's block-transfer coprocessor. |
| **Chip / Fast / Slow (Bogo) RAM** | The three Amiga RAM regions (`chipmem`/`fastmem`/`bogomem`). |
| **Quickstart** | A WinUAE template (`-s quickstart=A500,0`) that preconfigures a whole machine. |
| **Trackmo** | A multi-part demo that loads itself off floppy with a custom loader. |
| **Warp mode** | Run the CPU "as fast as possible" (`cpu_speed=max`) for fast boot/compile. |

## UAE internals you'll see in the code

| Symbol | Meaning |
|--------|---------|
| `regs.spcflags` | Per-instruction special-flags bitfield. `SPCFLAG_BRK` = "stop the CPU". |
| `debugger_active` / `debugging` | Global ints gating the console debugger. Set by `activate_debugger()`. |
| `debug_1()` | The console-debugger REPL loop in `debug.cpp`; blocks while paused. |
| `activate_debugger()` / `activate_debugger_new_pc()` | Enter the debugger / enter with a PC filter. |
| `handle_events()` | The per-vsync event pump in `custom.cpp`; calls `onUaeHandleEvents()`. |
| `cfgfile.cpp` | The `-s key=value` configuration parser. |
| `newcpu.cpp` | The main 68000 interpreter loop. |
| `cpuemu_*.cpp` | Per-CPU-model instruction handlers (68000…68060) + JIT. |

## CMake / build terms

| Term | Meaning |
|------|---------|
| `QUAESAR_EXTRA_LIBS` | Accumulator for libs added from child CMake scopes (via `quaesar_add_libs`). |
| `UAE_CUSTOM_IMPL_DIR` | Points at `src/uae_lib_imp/` — the porting layer UAE compiles against. |
| `QD_USE` | Feature list for the `qd` library (`EASTL;SDL2;IMGUI`). |
| `VAMIGA` | Option gating the vAmiga core + impl lib. |
| `force_load / --whole-archive / WHOLEARCHIVE` | Force-include the entire `amDebugger` archive so static registrars survive. |
| `CONFIGURE_DEPENDS` | Makes `file(GLOB)` re-evaluate when sources change. |
| `FSUAE` / `UAE=1` | Preprocessor switches selecting POSIX (FS-UAE) code paths in `uae_lib`. |

## UI / font terms

| Term | Meaning |
|------|---------|
| **Font Awesome** | The icon font (`data/static/fa-solid-900.ttf`) merged into the ImGui atlas so debugger toolbar buttons are glyphs. |
| **Source Code Pro** | The monospace UI font family (`data/static/SourceCodePro-*.ttf`). |
| **ImGui dockspace** | The debugger's dockable window layout; `default_layout.ini` is the bundled starter layout. |

## Shortcuts reference

Shortcuts are enumerated in `amDebugger/shortcutsList.h` (`amD::shortcut::EId`).
Notable ones:

| Id | Default | Action |
|----|---------|--------|
| `ShowDebuggerWnd` | Shift+F12 | open/focus debugger |
| `PauseEmulation` | Ctrl+F8 | pause/resume |
| `DebugTraceStart` / `DebugTraceContinue` | — | enter / leave trace mode |
| `DisasmTraceStepInto` / `StepOut` | — | CPU step |
| `CopperTraceStep` | — | copper step |
| `ResetAmigaEmu` | — | warm reset |
| `ToggleTurboEmulation` | — | warp toggle |
| `AlwaysOnTopEmu` | — | player window always-on-top |

ESC always exits the whole app (`CfgQsrMain::quitByEsc`).

---

← [Threading Model](11-threading-model.md) · [Index](index.md)
