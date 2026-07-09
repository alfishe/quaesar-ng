# Quaesar-NG Architecture Documentation

This is the **navigable map** of the Quaesar-NG codebase. It is organized around
**subsystem boundaries and dataflow**, not around individual files. Each document
is small, focused, and cross-linked so you can follow the control flow from the
UI all the way down to the emulated silicon.

> Scope note: this set lives in `docs/` and is separate from the older,
> topic-specific analyses in `doc/` (see [Related Documents](#related-documents)).

---

## Start here

| # | Document | What it answers |
|---|----------|-----------------|
| 1 | [Project Overview](01-overview.md) | What is Quaesar-NG? Design philosophy, tech stack, supported machines. |
| 2 | [System Architecture](02-system-architecture.md) | What are the layers, and where are the seams between them? |
| 3 | [The `IVm` Abstraction Boundary](03-vm-abstraction-boundary.md) | How does one debugger talk to two completely different emulator cores? |
| 4 | [Operation Dispatch](04-operation-dispatch.md) | How does a button click in the debugger reach the emulated CPU? |

## Backends (the two emulator cores)

| # | Document | What it answers |
|---|----------|-----------------|
| 5 | [UAE Backend](05-backend-uae.md) | The WinUAE/FS-UAE core, its global-state model, and its threading caveats. |
| 6 | [vAmiga Backend](06-backend-vamiga.md) | The modern C++ core and its per-instance model. |

## Inventory: what's in the tree

| # | Document | What it answers |
|---|----------|-----------------|
| 7 | [External Dependencies](07-external-dependencies.md) | What does every folder under `external/` do? |
| 8 | [Custom Libraries](08-libs-modules.md) | What do `qd`, `amDebugger`, `exprParser` provide? |
| 9 | [Build System](09-build-system.md) | How do the CMake targets depend on each other? |

## How it runs

| # | Document | What it answers |
|---|----------|-----------------|
| 10 | [Key Dataflows](10-key-dataflows.md) | Step-by-step sequence diagrams: boot, pause/resume, step, disk load. |
| 11 | [Threading Model](11-threading-model.md) | Which thread owns what, and what queues connect them? |
| 12 | [Glossary](12-glossary.md) | `IVm`, `amD`, `qd`, `qsr`, `SPCFLAG_BRK`, etc. |

## Contributor & user guides

| # | Document | What it answers |
|---|----------|-----------------|
| 13 | [CLI Reference](13-cli-reference.md) | Every command-line flag and the `-s` config keys. |
| 14 | [Getting Started](14-getting-started.md) | Prerequisites, building on each OS, IDE setup, first run. |
| 15 | [Debugger UI Guide](15-debugger-ui-guide.md) | How to add a debugger window (`QDB_WINDOW_REGISTER`, `AmDbgWindow` lifecycle). |
| 16 | [Memory & Caching](16-memory-and-caching.md) | Safe emulator-memory reads, the CDA disassembler, `QuadTreeAddrMap`, invalidation. |
| 17 | [Testing & CI](17-testing-and-ci.md) | What's tested, the CI matrix, how to verify locally before pushing. |
| 18 | [Feature Matrix](18-feature-matrix.md) | What's supported vs disabled vs removed in the WinUAE/FS-UAE core. |
| 19 | [Release & Packaging](19-release-packaging.md) | Current release state and what per-platform packaging entails. |

---

## The one diagram to remember

```mermaid
graph TB
    subgraph UI["Presentation — Dear ImGui windows"]
        DESKTOP["DebuggerDesktop<br/>+ windows"]
        PLAYER["VM Player Window<br/>(Amiga screen)"]
    end

    subgraph APP["Application orchestration — qd + qsr"]
        QAPP["QuaesarApplication"]
        OPREG["OperationsRegistry<br/>(command/shortcut dispatch)"]
    end

    subgraph DBG["Debugger library — amDebugger"]
        DBGAPP["DebuggerApp"]
        DBG["Debugger<br/>+ IVmDbgServiceBridge"]
    end

    subgraph VMABS["VM Abstraction — IVm::VM (the seam)"]
        IVM["IVm::VM<br/>Memory · Cpu · CustomRegs<br/>Copper · Blitter · Emu · Floppy"]
    end

    subgraph BACK["Backend impls (one chosen at runtime)"]
        UAE["UaeVmImp → uae_lib<br/>(global C state)"]
        VAM["VAmVmImp → VACore<br/>(per-instance C++)"]
    end

    subgraph EXT["External platform"]
        SDL["SDL2"]
        EASTL["EASTL"]
        IMGUI["Dear ImGui"]
        CAP["Capstone"]
    end

    DESKTOP --> DBGAPP
    DBGAPP --> DBG
    DBG --> IVM
    PLAYER --> QAPP
    QAPP --> OPREG
    OPREG --> DBGAPP
    IVM --> UAE
    IVM --> VAM
    UAE -.-> SDL
    VAM -.-> SDL
    DESKTOP -.-> IMGUI
    DESKTOP -.-> CAP
```

The single most important seam is **`IVm::VM`** ([doc 03](03-vm-abstraction-boundary.md)).
Everything above it is emulator-agnostic; everything below it is backend-specific.

---

## Suggested reading order

**New contributor?** 1 → 2 → **14** → 3 → 4, then 10.

**Just want to use it?** **13** (and the narrative [`doc/user_guide.md`](../doc/user_guide.md)).

**Debugging a pause/step bug?** 4 → 5 → 10 (and the historical
[`doc/pause_bug_analysis.md`](../doc/pause_bug_analysis.md)).

**Adding a debugger window?** **15** → 16 → 4.

**Adding a new dependency?** 7 → 9.

**Adding a new backend?** 3 → 6 → 11 (mirror what UAE/vAmiga do).

**Wondering if a feature exists?** **18**.

---

## Related documents

Pre-existing, topic-specific documents in the sibling `doc/` folder:

- [`doc/user_guide.md`](../doc/user_guide.md) — end-user CLI guide (narrative).
- [`doc/pause_bug_analysis.md`](../doc/pause_bug_analysis.md) — deep root-cause
  analysis of the UAE pause race; referenced from [Key Dataflows](10-key-dataflows.md).
- [`doc/bsdsocket_integration_plan.md`](../doc/bsdsocket_integration_plan.md) —
  plan for re-enabling `bsdsocket.library` networking.
