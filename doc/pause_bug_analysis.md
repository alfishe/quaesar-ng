# Debugger Pause Bug: Root Cause Analysis & Fix Proposal

**Status:** Analysis complete, fix not yet implemented
**Branch:** `dbg-render-fix`
**Symptom:** Clicking Pause (or `Ctrl+F8`, or the Pause menu item) in the ImGui debugger window does not reliably stop UAE instance execution. The emulation keeps running (or the effect is inconsistent/timing-dependent) even though the UI reflects a "paused" state.

---

## 1. Summary

The Pause operation is delivered to the emulator **twice**, through two different code paths, from two different threads:

1. An **immediate, unsynchronized, UI-thread call** into UAE's core (intended only to update a menu-mirroring "dummy" VM).
2. A **correctly queued call**, drained later on the UAE emulation thread.

Because UAE's core (`libs/uae_lib`) is legacy C code with **process-global singleton state** (`regs.spcflags`, `debugger_active`, `debugging`, `trace_mode`, etc. — not per-instance), path (1) is not actually isolated to a "dummy" object as intended. It reaches into and mutates the *same* global engine state that the real UAE thread is concurrently reading and writing. This is an unsynchronized cross-thread race on non-atomic global flags, and it can silently prevent `SPCFLAG_BRK` from ever being observed by the CPU interpreter loop — making Pause appear to do nothing.

A second, unrelated defect was also found: a fully-implemented, purpose-built pause mechanism (`UaeServerThread::pauseEmulation()` / `m_isPaused` / `m_pauseEvent`) exists but is **never called from anywhere** — it is dead code that the `PauseEmulation` operation was apparently intended to use, but never got wired to.

---

## 2. How Pause is *supposed* to work today

```mermaid
sequenceDiagram
    participant UI as ImGui UI Thread<br/>(DebuggerDesktop)
    participant App as QuaesarApplication<br/>(forwardOpToEmulatorCb)
    participant Dbg as amD::Debugger<br/>(dummy mirror VM)
    participant Queue as UaeServerThread<br/>op queue (mutex)
    participant UaeThread as UAE Emulation Thread<br/>(custom.cpp vsync loop)
    participant Core as UAE Core Globals<br/>(regs.spcflags, debugger_active, ...)

    UI->>App: doOperation_<PauseEmulation>()
    App->>Dbg: pDbg->setDebugMode(Break)  [qsr_application.cpp:57-59]
    Note over App,Dbg: Runs INLINE on the UI thread.<br/>Intended only as a UI/menu-state mirror.
    Dbg->>Core: activate_debugger_new_pc(0, 0xFFFFFFFF)  [debug.cpp:145]
    Note over Dbg,Core: BUT the "dummy" VM wraps the SAME<br/>process-global UAE singleton state.<br/>This is an unsynchronized cross-thread write.

    par Concurrently
        UaeThread->>Core: newcpu.cpp interpreter loop<br/>reads/clears regs.spcflags every instruction
    and
        App->>Queue: pVmPlayer->pushOperationMsg(clone(op))  [qsr_application.cpp:79-81]
        Queue-->>UaeThread: drained inside onUaeHandleEvents()<br/>(called from custom.cpp:12222 handle_events())
        UaeThread->>Core: setVmDebugMode(Break) -> activate_debugger_new_pc()<br/>[uae_vm_imp.cpp:210-220]
    end

    Note over Core: Race: whichever call observes<br/>debugger_active/spcflags last "wins".<br/>SPCFLAG_BRK can be silently lost.
```

---

## 3. Why the race breaks Pause

`activate_debugger()` (`libs/uae_lib/debug.cpp:114-137`) contains an early-return guard:

```c
void activate_debugger (void)
{
    disasm_init();
    if (isfullscreen() > 0)
        return;

    debugger_load_libraries();

    debugger_used = 1;
    inside_debugger = 1;
    debug_pc = 0xffffffff;
    trace_mode = 0;
    if (debugger_active) {
        // already in debugger but some break point triggered
        // during disassembly etc..
        return;                      // <-- can return BEFORE set_special(SPCFLAG_BRK)
    }
    debug_cycles(1);
    debugger_active = 1;
    set_special (SPCFLAG_BRK);       // <-- the actual "stop the CPU" signal
    debugging = 1;
    mmu_triggered = 0;
    debugmem_disable();
}
```

```mermaid
stateDiagram-v2
    [*] --> Running
    Running --> RaceWindow: PauseEmulation op fires
    state RaceWindow {
        [*] --> UIThreadCall
        [*] --> QueuedCall
        UIThreadCall --> CheckFlag1: sets debugger_active=1 first
        QueuedCall --> CheckFlag2: sees debugger_active already 1
        CheckFlag2 --> EarlyReturn: returns WITHOUT calling set_special(BRK) again
        CheckFlag1 --> MaybeSetsBRK: sets SPCFLAG_BRK (racy, non-atomic)
    }
    RaceWindow --> BRKLost: concurrent UAE-thread<br/>read-modify-write on regs.spcflags<br/>clobbers the UI-thread's write
    RaceWindow --> BRKHonored: no interleaving this time -<br/>pause "happens to work"
    BRKLost --> Running: CPU interpreter never sees BRK -<br/>Pause silently no-ops
    BRKHonored --> Paused
    Paused --> [*]
```

Key points:

- `regs.spcflags` is a **plain, non-atomic `int`**, read and read-modify-written by the UAE interpreter loop (`libs/uae_lib/newcpu.cpp`) on essentially every instruction.
- The UI-thread call to `activate_debugger_new_pc()` is **not synchronized** with any of that — no mutex, no memory barrier, no queue.
- `activate_debugger()`'s own reentrancy guard (`if (debugger_active) return;`) means whichever of the two calls (UI-thread or UAE-thread-queued) runs second is short-circuited and never reaches `set_special(SPCFLAG_BRK)` at all — so only one of the two calls ever has a chance to actually raise the flag, and *that* one is racing non-atomically against the interpreter's own flag manipulation.
- Additionally, `activate_debugger()` calls `disasm_init()` and `debugger_load_libraries()` — file I/O and buffer setup — from whichever thread invokes it, which is itself unsafe to run concurrently with the UAE thread's own use of that state.

---

## 4. Why the "dummy" VM isn't actually a dummy

```mermaid
flowchart TD
    A["DebuggerApp::init()<br/>debuggerWndApp.cpp:64"] --> B["setDbgServiceBridge(create_dummy_connection())"]
    B --> C["DummyVmDbgServiceBridge<br/>dbgConnection.cpp:9-23"]
    C --> D["m_vm = IVm::createByFactory_&lt;IVm::VM&gt;()"]
    D --> E["uae_vm_imp.cpp:411<br/>-> new UaeVmImp()"]
    E --> F["UaeVmImp wraps libs/uae_lib<br/>PROCESS-GLOBAL singleton state<br/>(regs, debugger_active, debugging, trace_mode, ...)"]

    G["QsrMainClientWndApp::getVmProvider()<br/>the REAL running emulator instance"] --> H["Also a UaeVmImp<br/>wrapping the SAME globals"]

    F -. "same global C state,\ntwo different C++ wrapper objects,\nno actual isolation" .-> H

    style F fill:#5a2323,stroke:#a33,color:#fff
    style H fill:#5a2323,stroke:#a33,color:#fff
```

The comment at `qsr_application.cpp:54` (*"Mirror debug mode to the Debugger's dummy VM for menu enable/disable state"*) reveals the original intent: this call was meant to update UI-only state so the Pause/Continue menu items enable/disable correctly, with zero effect on the actual emulator. That assumption is false for the UAE backend, because UAE has no notion of "instances" — there is one interpreter, one set of globals, and *any* `UaeVmImp` object (dummy or real) commands the same engine.

(Note: for the vAmiga backend this is not a problem, since vAmiga's `VAmiga::pause()`/`run()` go through a proper per-instance command queue rather than global C state — see `libs/vAmiga_imp_lib/src/qvAmigaImp/va_vm_imp.cpp:238-244`. This bug is UAE-backend-specific.)

---

## 5. The dead, purpose-built pause mechanism

A cleaner pause mechanism already exists in `UaeServerThread` but is never invoked:

```mermaid
flowchart LR
    subgraph Built["Built, but never wired"]
        P1["UaeServerThread::pauseEmulation()<br/>uae_server_thread.cpp:267-274"] --> P2["m_isPaused = true<br/>m_pauseEvent"]
        P2 --> P3["onUaeHandleEvents() blocks on m_pauseEvent<br/>uae_server_thread.cpp:253-254"]
        P3 --> P4["custom.cpp:12222<br/>while (handle_events()) { ...paused redraw loop... }"]
    end

    subgraph Actual["What PauseEmulation actually triggers"]
        A1["setVmDebugMode(Break)"] --> A2["activate_debugger_new_pc()"]
        A2 --> A3["SPCFLAG_BRK + legacy console debugger REPL<br/>debug_1() blocks on console command queue"]
    end

    X["PauseEmulation op"] -.->|"never connects here"| P1
    X -->|"actually goes here"| A1

    style P1 fill:#234,stroke:#48a,color:#fff
    style P2 fill:#234,stroke:#48a,color:#fff
    style P3 fill:#234,stroke:#48a,color:#fff
    style P4 fill:#234,stroke:#48a,color:#fff
```

`onUaeHandleEvents()` always returns `false` (`uae_server_thread.cpp:264`), so the `while (handle_events())` paused-redraw loop in `custom.cpp:12222-12234` never executes its body — confirming `pauseEmulation()` has no live callers today.

---

## 6. Full current call graph (as-is)

```mermaid
flowchart TD
    U["User clicks Pause<br/>debuggerDesktop.cpp:296-297"] --> Op["doOperation_&lt;PauseEmulation&gt;()"]
    Op --> DD["DebuggerDesktop::applyOperationMsgProcImp<br/>debuggerDesktop.cpp:29-43"]
    DD --> FWD["m_pDbgApp->forwardOpToEmulator(args)<br/>debuggerWndApp.h:92-94"]
    FWD --> CB["forwardOpToEmulatorCb lambda<br/>qsr_application.cpp:53-83"]

    CB --> M1["1) pDbg->setDebugMode(Break)<br/>qsr_application.cpp:57-59<br/>(UI thread, unsynchronized)"]
    M1 --> DbgSet["Debugger::setDebugMode<br/>debugger.cpp:85-88"]
    DbgSet --> DummyVM["dummy UaeVmImp::setVmDebugMode<br/>uae_vm_imp.cpp:210-220"]
    DummyVM --> ADNP1["activate_debugger_new_pc()<br/>debug.cpp:145-151"]
    ADNP1 --> Globals["UAE global state<br/>regs.spcflags, debugger_active, debugging"]

    CB --> M2["2) pVmPlayer->pushOperationMsg(clone)<br/>qsr_application.cpp:79-81<br/>(mutex-queued)"]
    M2 --> Drain["UaeServerThread::onUaeHandleEvents()<br/>uae_server_thread.cpp:241-244<br/>(runs ON the UAE thread, via<br/>custom.cpp:12222 handle_events())"]
    Drain --> RealApply["UaeVmImp::applyOperationMsgProcImp<br/>uae_vm_imp.cpp:125-127"]
    RealApply --> RealSet["setVmDebugMode(Break)"]
    RealSet --> ADNP2["activate_debugger_new_pc()"]
    ADNP2 --> Globals

    Globals --> CPU["newcpu.cpp interpreter loop<br/>check_debugger() every instruction<br/>reacts to SPCFLAG_BRK"]
    CPU -.->|"race: flag may be lost<br/>before this check runs"| Globals

    style M1 fill:#5a2323,stroke:#a33,color:#fff
    style DummyVM fill:#5a2323,stroke:#a33,color:#fff
    style ADNP1 fill:#5a2323,stroke:#a33,color:#fff
    style Globals fill:#5a2323,stroke:#a33,color:#fff
```

---

## 7. Distinct pause-related state (reference)

| State | Set at | Read at |
|---|---|---|
| `regs.spcflags & SPCFLAG_BRK` | `set_special()` calls in `debug.cpp` (multiple sites) | `newcpu.cpp` interpreter loop, every instruction |
| `debugger_active` (global int) | `activate_debugger()` / `deactivate_debugger()` (`debug.cpp`) | `qsr_application.cpp:67` routing decision; `UaeVmImp::Emu::isDebugActivated()` |
| `debugging` (global int) | `activate_debugger()` / `deactivate_debugger()` | `newcpu.cpp` BRK handlers |
| `IVm::EVmDebugMode` (Live/Break) | `setVmDebugMode()` in both `UaeVmImp` and `VaVmImp`, and the dummy mirror VM in `debugger.cpp` | UI enable/disable state (`debuggerDesktop.cpp:74,287-288`) |
| `UaeServerThread::m_isPaused` / `m_pauseEvent` | `pauseEmulation()` (never called) | `onUaeHandleEvents()` — dead path |
| vAmiga `Thread::state` (`ExecState`) | `switchState()` (`Thread.cpp`) | `isRunning()`/`isPaused()` — correct, per-instance, unaffected by this bug |

---

## 8. Proposed fix

Two independent problems, two independent fixes. Both are recommended.

### 8.1 Stop the UI thread from touching live UAE engine state

`qsr_application.cpp:53-62` should not call `setVmDebugMode()` on the dummy/mirror VM for backends whose engine is a process-global singleton. Options, in order of preference:

1. **Remove the dummy-VM mutation entirely.** Drive the Pause/Continue menu enable state from the *real* running VM's `EVmDebugMode` (already queryable via `pVmPlayer`/`getVmProvider()`), instead of maintaining a separate shadow VM. This removes the second, racy call path outright.
2. If the dummy-VM indirection must stay (e.g. for cases where no real VM is attached yet), make `Debugger::setDebugMode()` a **pure UI-state setter** (store an enum locally) rather than forwarding into `IVm::VM::setVmDebugMode()`, since for UAE that call is not actually side-effect-free.

### 8.2 Wire Pause to the existing (currently dead) safe mechanism

Reuse `UaeServerThread::pauseEmulation()` / `m_isPaused` / `m_pauseEvent` instead of `activate_debugger_new_pc()` for the plain "Pause" operation:

- It already integrates correctly with the `custom.cpp:12222` `while (handle_events())` paused-redraw loop.
- It avoids the legacy `debug_1()` console-REPL entirely for a simple pause (that path should remain reserved for the actual step/trace/console-debugging operations, which do need it).
- It requires only completing the wiring: route `PauseEmulation`/its resume counterpart through `pauseEmulation()`/`resumeEmulation()` on the UAE thread (already queue-drained on-thread, so no new synchronization is needed) instead of `setVmDebugMode(Break)`.

### 8.3 Suggested corrected flow

```mermaid
sequenceDiagram
    participant UI as ImGui UI Thread
    participant App as QuaesarApplication
    participant Queue as UaeServerThread op queue
    participant UaeThread as UAE Emulation Thread
    participant Pause as UaeServerThread::pauseEmulation()

    UI->>App: doOperation_<PauseEmulation>()
    App->>App: update local UI enable/disable state only<br/>(no call into any IVm::VM)
    App->>Queue: pushOperationMsg(PauseEmulation)  [single path]
    Queue-->>UaeThread: drained in onUaeHandleEvents()
    UaeThread->>Pause: pauseEmulation()
    Pause->>Pause: m_isPaused = true
    Note over UaeThread: handle_events() loop now blocks<br/>on m_pauseEvent until Continue is queued
    UaeThread->>UI: Paused (single, deterministic path, no race)
```

---

## 9. Files referenced

- `src/quasar_app/qsr_application.cpp` (op forwarding, the racy inline call)
- `libs/amDebugger/src/amDebugger/debugger.cpp` (`setDebugMode`, `getVm`)
- `libs/amDebugger/src/amDebugger/dbgConnection.cpp` / `.h` (dummy VM construction)
- `libs/amDebugger/src/amDebugger/debuggerWndApp.cpp` (`DebuggerApp::init`)
- `libs/amDebugger/src/amDebugger/ui/debuggerDesktop.cpp` (Pause button/menu/shortcut)
- `src/quasar_app/uae_imp/uae_vm_imp.cpp` (`setVmDebugMode`, `applyOperationMsgProcImp`)
- `src/quasar_app/uae_imp/uae_server_thread.cpp` (op queue, `onUaeHandleEvents`, dead `pauseEmulation`/`m_isPaused`)
- `libs/uae_lib/debug.cpp` (`activate_debugger`, `activate_debugger_new_pc`, `debug_1`)
- `libs/uae_lib/newcpu.cpp` (`check_debugger`, SPCFLAG_BRK checks)
- `libs/uae_lib/custom.cpp:12222` (`handle_events()` vsync loop, paused-redraw loop)
- `libs/vAmiga_imp_lib/src/qvAmigaImp/va_vm_imp.cpp` (unaffected vAmiga backend, for comparison)
