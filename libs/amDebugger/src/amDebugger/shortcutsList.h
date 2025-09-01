#pragma once
#include "qd/base/base.h"
#include "qd/enum/enumBase.h"
#include "qd/qui/shortcut.h"
#include <imgui/imgui.h>


namespace amD {
namespace shortcut {

#define SHORTCUT(name, setup_func)

//
// Shortcuts list + Enum class shortcut::EId declaration
// 'qd::shortcut::EId::DebugTraceStart'
//
#define SHORTCUT_LIST(SHORTCUT)                                                                                   \
    SHORTCUT(DisasmTraceStepInto, [](qd::Shortcut& s) { s.addKey(ImGuiKey_F11).setRepeat(); })                    \
    SHORTCUT(DisasmTraceStepOut, [](qd::Shortcut& s) { s.addKey(ImGuiKey_F10).setRepeat(); })                     \
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
    SHORTCUT(ShowDebuggerWnd, [](qd::Shortcut& s) { s.addKey(ImGuiKey_F12).addKey(ImGuiMod_Shift); })             \
    SHORTCUT(ShowUaeOptionsWnd, [](qd::Shortcut& s) { s.addKey(ImGuiKey_P).addKey(ImGuiMod_Ctrl); })              \
    /* END OF SHORTCUTS LIST */
//////////////////////////////////////////////////////////////////////////


struct EId {
    enum Type : uint32_t {
        UNDEF = 0,
#undef SHORTCUT
#define SHORTCUT(name, setup_func) name,
        SHORTCUT_LIST(SHORTCUT)
#undef SHORTCUT
        MAX_COUNT
    };
    ENUM_DECLARE_BASE(amD::shortcut::, EId, Type, UNDEF);
}; // struct


// simple array with index
inline static qd::ShortcutInitItem g_shortcuts_list[] = {
#define SHORTCUT(name, setup_func) {EId::##name, setup_func},
    SHORTCUT_LIST(SHORTCUT)
#undef SHORTCUT
}; // ShortcutList


// extern qd::Shortcut *makeInstance(qd::shortcut::EId id);


}; // namespace shortcut
}; // namespace amD
