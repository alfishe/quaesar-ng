# Emulation Core Switching — Design & Implementation

## Context (current state)

- A plugin-style factory registry already exists: each backend registers an `IAppPartServerProviderFactory` (`UaeServerProviderFactory` → id `"uae"`, `VAmigaServerProviderFactory` → id `"vamiga"`). `VmPlayersSelector::activateVmPlayerByIdStr()` builds the right `BaseVmServerAppPart`, which spawns its server thread and wires SDL framebuffer + debugger.
- Startup selection already works: `CfgQsrMain::vmPlayerId` (default `"uae"`) drives `QsrMainClientWndApp::init()`. The Emulator menu has a `// todo VM providers list` placeholder.
- The operation-routing lambda is UAE-specific but safely guarded: `dynamic_cast<UaeServerThread*>` returns null for vAmiga → falls through to the queued path. No backend bug today.

## Decisions (confirmed)

- **Cold boot** on switch (no RAM/CPU state transfer).
- **Process restart** as the switch mechanism (cleanest for UAE globals). Trade-off: brief window teardown/reappear; debugger layout survives via `default_layout.ini`.

## Why process restart

UAE uses process-global C singletons (`assert(!g_pSingleton)` in `UaeServerThread`, plus `currprefs`/`regs`/`debugger_active`). In-process teardown→rebuild of UAE is risky. Re-launching the process gives a fresh address space; the existing startup path already does all the SDL/debugger/core wiring for whichever `vmPlayerId` is set — so "instantiate new core, bind to SDL + debugger, start it" is achieved by the new process for free.

---

## Phase 1 — Design document (primary deliverable)

Write `doc/emulation_core_switch_design.md` in the existing incremental, section-by-section doc style (Mermaid where useful, relative paths), covering:

1. **Goal & scope**: startup + runtime core selection; cold-boot recycle via process restart.
2. **Current selection architecture**: factory registry, `activateVmPlayerByIdStr`, wiring chain (selector → `BaseVmServerAppPart` → server thread → `IVmClientPlayer` → SDL framebuffer; `onFrameUpdate` debugger bridge swap). Cite [vm_player_selector.cpp](src/quasar_app/vm_player_selector.cpp), [qsr_main_wnd_client_app.cpp](src/quasar_app/qsr_main_wnd_client_app.cpp), [qsr_application.cpp](src/quasar_app/qsr_application.cpp).
3. **The UAE global-singleton constraint** and why it forces serialize/recycle rather than concurrent hot-swap; why process restart was chosen over in-process rebuild.
4. **Switch flow** (sequence diagram): menu item → `SwitchEmuCore{coreId}` → set relaunch target + `requestAppToQuit()` → clean teardown in `destroy()` → `qsr_main` re-execs with `--core <id>`.
5. **Startup selection** hardening: `--core` CLI + `vmPlayerId` config precedence.
6. **Re-exec contract**: full original argv preserved; `--core` appended/overridden; backend-agnostic (works for any registered factory id).
7. **Forward-looking note**: future in-process vAmiga↔vAmiga hot-swap would need a teardown/rebuild lifecycle on `BaseVmServerAppPart`; out of scope here.

## Phase 2 — Startup selection hardening

- `src/quasar_app/qsr_main.cpp`: add CLI option `--core <id>` (CLI11) bound to `g_cfg_vm_wnd.vmPlayerId`. Default stays `"uae"`. If the id has no registered factory, log + fall back to `"uae"`.
- Validation helper: `VmPlayersSelector::hasFactory(id)` (thin wrapper over `findFactoryByIdStr`).

## Phase 3 — Factory enumeration API

- `src/quasar_app/vm_player_selector.h`: expose factory list for UI. Add `AppPartServerFactoryListMgr::getFactories()` (currently `m_appPartServerFactoryList` is private) returning `{id, guiName}` pairs. Add `VmPlayersSelector::getAvailableProviders()`.

## Phase 4 — Switch operation + UI

- `src/quasar_app/qsr_operations.h`: new op `struct SwitchEmuCore : amD::operation::OperationArgs { std::string coreId; ... }`.
- `src/quasar_app/ui/uae_wnd_desktop.cpp`: replace the `// todo VM providers list` placeholder in the Emulator menu with a submenu iterating `getAvailableProviders()`, radio-checking the active id, each item firing `doOperation_<SwitchEmuCore>({coreId})`.
- Handle `SwitchEmuCore` in `QsrMainClientWndApp::applyOperationMsgProcImp`: ignore if `coreId == current`; else set `QuaesarApplication::m_relaunchCoreId` and call `requestAppToQuit()`.

## Phase 5 — Restart / re-exec mechanism

- `src/quasar_app/qsr_application.h`: add `qtd::string m_relaunchCoreId;` + getter; cleared unless a switch is pending.
- `src/quasar_app/qsr_main.cpp`: after `doMainLoop()` + `g_pApp->destroy()` + `NFD_Quit()` + `SDL_Quit()`, if `m_relaunchCoreId` is set, re-exec:
  - POSIX (`#ifndef _WIN32`): build argv = original argv + `{"--core", coreId}`, then `execv`.
  - Windows: `CreateProcessW`/`_wexecv` then `exit(0)`.
  - Store a copy of original `argc/argv` at startup (SDL may reallocate) for reuse.
- Teardown is already clean: `Application::destroy()` → `UaeServerAppPart::destroyImp()` (or vAmiga) joins the emulator thread; `QsrMainClientWndApp::destroyImp()` tears down SDL window/texture/renderer. No new teardown code needed.

## Notes / non-goals

- No state transfer across switch (cold boot).
- No in-process concurrent cores (impossible for UAE; unnecessary given restart choice).
- The UAE-specific `execConsoleCmd` routing stays in the forwarding lambda (still guarded by `dynamic_cast`); not refactored since it is not broken and restart resets the process anyway. Document this as deliberate.
- Debugger window visibility resets to ini defaults on restart (acceptable; layout persists via ini).

## Verification

- Launch with `--core uae` (default) and `--core vamiga`; confirm each boots and binds debugger (registers + screen update).
- From the Emulator menu, switch UAE↔vAmiga; confirm process re-execs, the other core boots, SDL window + debugger rebind, and the active item is radio-checked correctly.
- Unknown core id falls back to UAE with a log line.