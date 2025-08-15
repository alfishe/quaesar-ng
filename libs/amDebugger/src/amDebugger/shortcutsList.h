#pragma once
#include "qd/base/base.h"
#include "qd/qui/shortcutMgr.h"


namespace amD {
namespace shortcut {

#define SHORTCUT(name, setup_func)

//
// Shortcuts list + Enum class shortcut::EId declaration
// 'qd::shortcut::EId::DebugTraceStart'
//
#define SHORTCUT_LIST(SHORTCUT)                                                                                   \
    SHORTCUT(DebugTraceStepInto, [](qd::Shortcut& s) { s.addKey(ImGuiKey_F11).setRepeat(); })                     \
    SHORTCUT(DebugTraceStepOut, [](qd::Shortcut& s) { s.addKey(ImGuiKey_F10).setRepeat(); })                      \
    SHORTCUT(DebugTraceStart, [](qd::Shortcut& s) { s.addKey(ImGuiKey_F12); })                                    \
    SHORTCUT(DebugTraceContinue, [](qd::Shortcut& s) { s.addKey(ImGuiKey_F5); })                                  \
    SHORTCUT(DebugWaitScanLines, [](qd::Shortcut&) {})                                                            \
    SHORTCUT(DisasmToggleBreakpoint, [](qd::Shortcut& s) { s.addKey(ImGuiKey_F9); })                              \
    SHORTCUT(CopperToggleBreakpoint,                                                                              \
        [](qd::Shortcut& s) { s.addKey(ImGuiKey_F9).addKey(ImGuiMod_Shift).setRepeat(); })                        \
    SHORTCUT(CopperTraceStep, [](qd::Shortcut& s) { s.addKey(ImGuiKey_F11).addKey(ImGuiMod_Shift).setRepeat(); }) \
    SHORTCUT(ToggleTurboEmulation, [](qd::Shortcut& s) { s.addKey(ImGuiKey_NumLock); })                           \
    SHORTCUT(ResetAmigaEmu, [](qd::Shortcut&) {})                                                                 \
    SHORTCUT(AlwaysOnTopEmu, [](qd::Shortcut& s) { s.addKey(ImGuiKey_T).addKey(ImGuiMod_Ctrl); })                 \
    SHORTCUT(ShowDebuggerWnd, [](qd::Shortcut&) {})                                                               \
    /* END OF SHORTCUTS LIST */
//////////////////////////////////////////////////////////////////////////


enum class EId {
    UNDEF = -1,
#undef SHORTCUT
#define SHORTCUT(name, setup_func) name,
    SHORTCUT_LIST(SHORTCUT)
#undef SHORTCUT
    MAX_COUNT
}; // enum


inline static qd::ShortcutSetupFunc g_shortcuts_list[] = {
#define SHORTCUT(name, setup_func) setup_func,
    SHORTCUT_LIST(SHORTCUT)
#undef SHORTCUT
}; // ShortcutList


// extern qd::Shortcut *makeInstance(qd::shortcut::EId id);


}; // namespace shortcut
}; // namespace amD
