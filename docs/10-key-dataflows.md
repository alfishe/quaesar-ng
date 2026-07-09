# 10 — Key Dataflows

← [Build System](09-build-system.md) · [Index](index.md) · → [Threading Model](11-threading-model.md)

This document collects the **sequence diagrams** for the flows that matter most.
Component-level dataflow is covered in [doc 04](04-operation-dispatch.md); here
we trace concrete scenarios end to end.

---

## 1. Process startup & wiring

```mermaid
sequenceDiagram
    autonumber
    participant OS as OS / shell
    participant M as SDL_main
    participant CH as CrashHandler
    participant CLI as CLI11
    participant Q as QuaesarApplication
    participant MM as appPartsMgr
    participant Player as QsrMainClientWndApp
    participant Dbg as DebuggerApp

    OS->>M: quaesar demo.adf -k kick.rom -s ...
    M->>CH: create() + install()   (first thing)
    M->>CLI: parse argc/argv
    CLI-->>M: g_cfg_startup {input, kickRomPath, serialPort, uaeExtArgs[]}
    M->>M: SDL_Init(VIDEO|AUDIO)
    M->>Q: new QuaesarApplication()
    M->>Q: onConstruct(params)
    Q->>MM: createPart_<QsrMainClientWndApp>("VM client app")
    MM->>Player: instantiate (reflection)
    Q->>MM: createPart_<DebuggerApp>("Quaesar Debugger")
    MM->>Dbg: instantiate + init()
    Q->>Dbg: setForwardOpToEmulatorCb(λ)   (the debugger↔emu bridge)
    M->>M: NFD_Init()
    M->>Q: initialize() → doMainLoop()
    Note over Player: On first frame the chosen backend<br/>(default "uae") spins up its thread.
```

---

## 2. Backend thread boot (UAE)

```mermaid
sequenceDiagram
    autonumber
    participant Main as UI thread
    participant Part as UaeServerAppPart
    participant TH as UaeServerThread
    participant Proxy as qsr_imp_proxy
    participant Core as uae_lib (emu thread)

    Main->>Part: onPartCreate (vmPlayerId="uae")
    Part->>TH: new UaeServerThread(this)
    Part->>TH: initialize() → SDL_CreateThread
    TH->>Core: emu main (main.cpp)
    Core->>Core: cfgfile.cpp parses g_cfg_startup.uaeExtArgs<br/>(each "-s key=value")
    Core->>Core: build machine from quickstart / manual keys
    Core->>Core: mount DF0: = g_cfg_startup.input
    Core->>Proxy: qsr_setUaeInitialized(true)
    Proxy->>TH: UaeServerThread::get()->setUaeInitialized(true)
    TH-->>Main: m_onUaeInitialized signaled (UI can render)
    loop vsync (on emu thread)
        Core->>Proxy: qsr_onUaeHandleEvents()
        Proxy->>TH: onUaeHandleEvents()  → drain SDL events + op queue
        Core->>Proxy: qsr_lockUaeScreenTexBuf(w,h) / _unlock
        Note over TH: m_scrFrameNo++ (atomic)
    end
```

---

## 3. Render a debugger frame

The debugger window runs on the UI thread and throttles to ~15 FPS
(`DebuggerApp::kMinFrameIntervalMs = 66`).

```mermaid
sequenceDiagram
    autonumber
    participant Loop as qd::Application main loop (UI thread)
    participant Dbg as DebuggerApp
    participant Desktop as DebuggerDesktop
    participant Win as inspector windows
    participant VM as IVm::VM (snapshot)

    Loop->>Dbg: updateAppPart(dt, time)
    Dbg->>Dbg: throttle check (≥66ms since last)
    Loop->>Dbg: renderAppPart()
    Dbg->>VM: fetchStateFromEmu()   (snapshot CPU/mem/copper/custom)
    Dbg->>Desktop: render dockspace + toolbar
    Desktop->>Win: each registered window renders
    Win->>VM: cpu->getPC(), mem->getU16(addr), custom->getRegVal(...)
    Note over Win: pure read from the snapshot;<br/>no emulator mutation here
```

---

## 4. Pause / Resume (the canonical flow)

```mermaid
sequenceDiagram
    autonumber
    participant U as User
    participant UI as DebuggerDesktop (UI thread)
    participant Cb as forwardOpToEmulatorCb
    participant Player as IVmClientPlayer
    participant Emu as emulator thread
    participant VM as IVm::VM::applyOperationMsgProcImp
    participant Core as emulator core

    U->>UI: click Pause (Ctrl+F8)
    UI->>Cb: doOperation_<PauseEmulation>()
    Cb->>Cb: setDebugMode(Break)  [UI mirror only, no engine call]
    Cb->>Player: pushOperationMsg(clone)
    Note over Player,Emu: op waits in mutex-protected deque
    Emu->>Player: onUaeHandleEvents() / onVAmHandleEvents()
    Player->>VM: applyOperationMsgProcImp(PauseEmulation)
    alt UAE
        VM->>Core: activate_debugger() → SPCFLAG_BRK
        Core->>Core: interpreter loop sees BRK, enters debug_1() (blocks)
    else vAmiga
        VM->>Core: vAmiga->pause()  (ExecState ← paused)
    end
    Note over U: screen frozen; debugger shows current state
    U->>UI: click Continue
    UI->>Cb: DebugTraceContinue
    Cb->>Cb: setDebugMode(Live)  [mirror]
    Cb->>Emu: UAE: execConsoleCmd("g"); vAmiga: pushOperationMsg
    Note over Core: emulation resumes
```

> Historical bug context: the original implementation delivered Pause **twice**
> (UI-thread inline + queued), racing on UAE's non-atomic globals. The current
> single-path design fixes that — see
> [`doc/pause_bug_analysis.md`](../doc/pause_bug_analysis.md).

---

## 5. Step Into while paused (UAE console escape hatch)

When UAE's `debug_1()` is blocked waiting for console input, the operations
queue is stuck. Step/continue therefore bypass the queue and inject console
commands.

```mermaid
sequenceDiagram
    autonumber
    participant UI as DebuggerDesktop (UI thread)
    participant Cb as forwardOpToEmulatorCb
    participant Uae as UaeServerThread
    participant Q as console cmd queue
    participant Core as UAE core (blocked in debug_1)

    UI->>Cb: DisasmTraceStepInto
    Cb->>Cb: dynamic_cast<UaeServerThread*> ok; debugger_active==true
    Note over Cb: queue path won't work (debug_1 is blocking)
    Cb->>Uae: execConsoleCmd("t")
    Uae->>Q: push "t"
    Core->>Q: readLine() returns "t"   (unblocks)
    Core->>Core: trace one instruction → re-enter debug_1()
    Note over UI: next render shows advanced PC
```

vAmiga has no blocking console REPL, so stepping always uses the normal queued
operation path.

---

## 6. Disk / ADF load at startup

```mermaid
sequenceDiagram
    autonumber
    participant CLI as CLI11
    participant Main as qsr_main
    participant Core as emulator core
    participant ADF as ADFlib / vAmiga Media

    CLI->>CLI: parse positional "input" → g_cfg_startup.input
    Main->>Core: startup, config carries the input path
    Core->>Core: cfgfile mounts DF0: = input path
    alt .adf
        Core->>ADF: open + parse OFS/FFS structures
    else .ipf / .dms
        Core->>Core: flux/raw decode (xdms / IPF support)
    end
    Core->>Core: Kickstart boots, reads boot blocks via Paula disk DMA
    Note over Core: trackmo / demo loads off DF0:
```

Disk swap is done by reconfiguring `floppy1`/`floppy2`/... on the command line
(multi-drive) — there is no runtime disk-swap GUI.

---

## 7. Screen frame transport (emu → UI texture)

```mermaid
sequenceDiagram
    autonumber
    participant Core as emulator core (emu thread)
    participant TH as *ServerThread (emu thread)
    participant Tex as SDL_Texture (UI thread)
    participant Wnd as QsrMainClientWndApp (UI thread)

    Core->>TH: render frame → _lockUaeScreenTexBuf(w,h) / fetchScreenBufferToTexture
    TH->>TH: copy pixels into m_pAmigaBuffer under m_UaeScrTextureMutex
    TH->>TH: SDL_atomic_t m_scrFrameNo++
    Wnd->>TH: getScrFrameNo() changed?
    Wnd->>TH: lockDisplayTexBuf(&w,&h,&pixels)
    TH-->>Wnd: shared pixel buffer pointer
    Wnd->>Tex: SDL_UpdateTexture(...)
    Wnd->>Tex: SDL_RenderCopy → present
    Wnd->>TH: unlockDisplayTexBuf()
```

The atomic frame counter is the cheap "is there a new frame?" signal; the mutex
guards only the actual pixel copy.

---

## 8. Input event transport (host → emu)

```mermaid
sequenceDiagram
    autonumber
    participant SDL as SDL event pump (UI thread)
    participant Wnd as QsrMainClientWndApp
    participant TH as *ServerThread
    participant Core as emulator core (emu thread)

    SDL->>Wnd: SDL_Event (keyboard/mouse/joystick)
    Wnd->>TH: pushSdlEvent(event)   (into m_sdlEventsQueue + mutex)
    Core->>TH: on*HandleEvents()
    TH->>TH: drain m_sdlEventsQueue under m_eventMutex
    TH->>Core: replay events into inputdevice / vAmiga input API
```

---

## 9. Known sharp edge: disassembly window stack-buffer aliasing

A historical SIGBUS on ARM64 came from an `EASTL fixed_string` (inline stack
buffer) placed adjacent to a VM pointer; a write overran into the pointer.

```mermaid
graph LR
    A["fixed_string buffer<br/>(on stack)"] -->|"overrun by N bytes"| B["adjacent IVm::VM*<br/>corrupted"]
    B -->|"dereferenced"| CRASH["SIGBUS on ARM64"]

    style A fill:#5a2323,stroke:#a33,color:#fff
    style CRASH fill:#5a2323,stroke:#a33,color:#fff
```

Lesson: when laying out locals in debugger windows, keep `fixed_string` /
`fixed_vector` buffers away from raw pointers, or prefer heap-backed
`qtd::string`/`qtd::vector` for anything that grows.

---

← [Build System](09-build-system.md) · [Index](index.md) · → [Threading Model](11-threading-model.md)
