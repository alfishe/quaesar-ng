# 5 — UAE Backend

← [Operation Dispatch](04-operation-dispatch.md) · [Index](index.md) · → [vAmiga Backend](06-backend-vamiga.md)

The UAE backend is the **default and primary** emulator core. It is the mature
WinUAE engine, re-hosted on POSIX platforms via the FS-UAE adaptation layer.
This document maps its three sub-layers and the all-important **global-state
caveat**.

## The three sub-layers

```mermaid
graph TB
    subgraph QSR["Application glue — src/quasar_app/uae_imp/"]
        PART["UaeServerAppPart<br/>(ApplicationPart)"]
        TH["UaeServerThread<br/>(IVmClientPlayer, on emu thread)"]
        VM["UaeVmImp : IVm::VM<br/>(concrete snapshot impl)"]
    end

    subgraph PORT["POSIX porting layer — src/uae_lib_imp/"]
        CUSTOM["gfx · input · keyboard · file_system · hardfile_host · gui · thread · time · sound · adf"]
        SYSCONFIG["sysconfig.h · target.h · machdep/ · sounddep/ · threaddep/"]
    end

    subgraph CORE["Emulator core — libs/uae_lib/"]
        CPU["newcpu.cpp · cpuemu_*.cpp<br/>(68000–68060 interpreters + JIT)"]
        CUSTOMC["custom.cpp<br/>(Agnus/Denise/Paula, vsync loop)"]
        DEBUG["debug.cpp<br/>(activate_debugger, console REPL)"]
        CFG["cfgfile.cpp<br/>(the -s config parser)"]
        FS["filesys · hardfile · disk<br/>(storage)"]
    end

    PART --> TH
    TH --> VM
    VM --> CORE
    CORE --> PORT
    TH --> PORT

    style QSR fill:#1a2a3a,stroke:#48a,color:#fff
    style PORT fill:#2a2a1a,stroke:#aa4,color:#fff
    style CORE fill:#3a1a1a,stroke:#a55,color:#fff
```

| Layer | Location | Role |
|-------|----------|------|
| **Application glue** | `src/quasar_app/uae_imp/` | Spins up the UAE thread, owns the `IVm::VM` impl, bridges SDL events & operations. |
| **Porting layer** | `src/uae_lib_imp/` | The platform functions UAE *calls into*. UAE is written to call `custom.cpp`, `sound.cpp`, etc.; Quaesar provides these here. `UAE_CUSTOM_IMPL_DIR` points CMake here. |
| **Core** | `libs/uae_lib/` | The actual emulation: CPU interpreters, custom chips, debugger, config parser, filesystems. |

## The `UaeServerThread` — the thread boundary

`UaeServerThread` implements `qsr::IVmClientPlayer` and **lives on the UAE
emulation thread** (not the UI thread). It is the surface the UI window talks to.

```mermaid
graph LR
    subgraph UI["Main / UI thread"]
        WND["QsrMainClientWndApp"]
    end
    subgraph ET["UAE thread"]
        TH["UaeServerThread"]
        GLOBALS["uae_lib global state<br/>(regs, spcflags, debugger_active, ...)"]
    end

    WND -->|"getVm()"| TH
    WND -->|"lockDisplayTexBuf()"| TH
    WND -->|"pushSdlEvent()"| TH
    WND -->|"pushOperationMsg()"| TH
    TH -.->|"reads/writes"| GLOBALS

    TH -->|"m_pClientOpsStack (deque+mutex)"| OPS["queued operations"]
    TH -->|"m_sdlEventsQueue (deque+mutex)"| EVT["SDL events"]
    TH -->|"m_UaeScrTextureMutex + m_pAmigaBuffer"| SCR["screen pixels"]
    TH -->|"SDL_atomic_t m_scrFrameNo"| FR["frame counter"]

    style TH fill:#3a1a1a,stroke:#a55,color:#fff
    style GLOBALS fill:#5a2323,stroke:#a33,color:#fff
```

Cross-thread state is deliberately narrow and explicitly synchronized:

| Member | Type | Purpose |
|--------|------|---------|
| `m_eventMutex` + `m_sdlEventsQueue` | deque + mutex | host→emu input events |
| `m_pClientOpsStack` + (deque) | deque + mutex | queued `BaseOpArgs` ops |
| `m_UaeScrTextureMutex` + `m_pAmigaBuffer` | mutex + pixel buf | emu→host screen frame |
| `m_scrFrameNo` | `SDL_atomic_t` | cheap "new frame?" check |
| `m_pConsoleQueue` | custom queue | console-debugger command/response |
| `m_pauseEvent` | `ThreadEvent` | (legacy pause mechanism — see below) |

See [Threading Model](11-threading-model.md) for the full concurrency picture.

## The global-state caveat (read this twice)

**UAE has no concept of "instances".** There is one interpreter, one set of
globals. Any `UaeVmImp` object — whether the "real" one attached to the player,
or a "dummy" mirror the debugger creates — commands the *same* process-global
engine.

```mermaid
graph TB
    REAL["real UaeVmImp<br/>(player backend)"]
    DUMMY["dummy UaeVmImp<br/>(debugger mirror)"]

    REAL --> GLOBALS["process-global UAE state<br/>regs.spcflags · debugger_active ·<br/>debugging · trace_mode · inside_debugger ..."]
    DUMMY --> GLOBALS

    NOTE["same C globals,<br/>two C++ wrappers,<br/>no isolation"]

    DUMMY -.-> NOTE
    NOTE -.-> GLOBALS

    style GLOBALS fill:#5a2323,stroke:#a33,color:#fff
    style NOTE fill:#5a2323,stroke:#a33,color:#fff
```

### Why this caused the pause bug

When `PauseEmulation` was delivered **twice** — once inline on the UI thread via
the dummy mirror, once queued on the emu thread — both calls raced on the
non-atomic `regs.spcflags` and on `activate_debugger()`'s reentrancy guard
(`if (debugger_active) return;`). `SPCFLAG_BRK` could be silently lost.

→ Full root-cause: [`doc/pause_bug_analysis.md`](../doc/pause_bug_analysis.md).

### How it's handled now

The `QuaesarApplication` forwarding callback (see [doc 04](04-operation-dispatch.md))
keeps the UI-thread mirror call **side-effect-free** — it only updates the
`Debugger`'s local UI state, never the engine. The actual pause/step/continue is
applied **exactly once**, on the emu thread, via either:

- the queued operations path (`pushOperationMsg`), or
- the **console-command escape hatch** (`execConsoleCmd("t"/"z"/"ot"/"g")`),
  needed because UAE's legacy console debugger (`debug_1()` in `debug.cpp`)
  blocks the operations queue while paused.

## The legacy console debugger

UAE ships its own text debugger (`debug.cpp`). When paused, `debug_1()` blocks
on a console-command queue. Quaesar reuses this for step/trace/continue by
injecting single-letter commands:

| Quaesar operation | Console command | Effect |
|-------------------|-----------------|--------|
| `DisasmTraceStepInto` | `t` | trace one instruction |
| `DisasmTraceStepOut` | `z` | step out |
| `CopperTraceStep` | `ot` | one copper step |
| `DebugTraceContinue` | `g` | go (resume) |

`UaeServerThread::execConsoleCmd()` and `uaeWaitConsoleCmdImpl()` are the two
ends of this queue (the latter is what UAE's `debug_1()` calls to read input).

## Boot sequence (UAE)

```mermaid
sequenceDiagram
    participant Main as UI/main thread
    participant Part as UaeServerAppPart
    participant TH as UaeServerThread
    participant Core as uae_lib

    Main->>Part: onPartCreate (VM player chosen = "uae")
    Part->>TH: new UaeServerThread(this)
    Part->>TH: initialize()  → spawns SDL_Thread
    TH->>Core: UAE entry (main.cpp) → cfgfile parse (-s args)
    Core->>Core: build machine (quickstart / manual config)
    Core->>TH: calls qsr_setUaeInitialized(true) via qsr_imp_proxy
    TH-->>Main: m_onUaeInitialized event signaled
    loop vsync
        Core->>TH: handle_events() → onUaeHandleEvents()
        TH->>TH: drain SDL events + op queue
        Core->>TH: render frame → _lockUaeScreenTexBuf/_unlock
    end
```

The proxy functions in `src/quasar_app/uae_imp/qsr_imp_proxy.cpp`
(`qsr_lockUaeScreenTexBuf`, `qsr_onUaeHandleEvents`, `qsr_waitConsoleCmd`, ...)
are the C-callable entry points UAE invokes back into Quaesar's `UaeServerThread`
singleton.

## When to touch what (UAE)

| Task | Edit |
|------|------|
| New `-s` config key | nothing — `cfgfile.cpp` already parses free-form keys |
| Platform port fix | `src/uae_lib_imp/` (sound/thread/gfx) |
| New debug command behavior | `debug.cpp` + map an op in the forwarding callback |
| Core emulation bug | `libs/uae_lib/` (but prefer the vAmiga core as a reference) |

---

← [Operation Dispatch](04-operation-dispatch.md) · [Index](index.md) · → [vAmiga Backend](06-backend-vamiga.md)
