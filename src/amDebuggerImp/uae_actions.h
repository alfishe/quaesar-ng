#pragma once
// clang-format off
#include <src/sysconfig.h>
#include "sysdeps.h"
#include "options.h"
#include "keyboard.h"
#include "inputdevice.h"
#include "inputrecord.h"
#include "keybuf.h"
#include "custom.h"
#include "xwin.h"
#include "drawing.h"
#include <uae_src/include/savestate.h>
#include <uae_src/include/uae.h>
// clang-format on
#include <SDL.h>
#include <amDebugger/action_mgr.h>
#include <amDebugger/debugger.h>
#include <amDebugger/msg_list.h>
#include <amDebugger/shortcut/shortcut_list.h>
#include <qdIce/qdImGui/imgui_eastl.h>
#include <qdIce/qdTypeSystem/attributesCommon.h>
#include <quaesar.h>
#include <uae_src/include/debug.h>


//////////////////////////////////////////////////////////////////////////
// These are private UAE actions and in a good case they should not be called directly by name,
// instead they are should triggered via messages (dbg->applyActionMsg(m)) or shortcuts.
//

namespace qd {
namespace action {
namespace uae {


struct DebugDmaOption : public Action {
    QDB_REG_ACTION(DebugDmaOption);
    void setup() {
        supportMtd.set(UiDrawEvent::MainMenu_Debug);
    }

    void onDrawMainMenuItem(UiDrawEvent::Type event);

    virtual EFlow applyActionMsgProc(action::msg::Base* p_msg) override;
};
//////////////////////////////////////////////////////////////////////////


struct DisasmTraceStep : public Action {
    QDB_REG_ACTION(DisasmTraceStep);
    void setup() {
        mName = "Step Into";
        supportMtd.set(UiDrawEvent::MainMenu_Debug);
        addShortcut(qd::shortcut::EId::DebugTraceStepInto);
    }
    virtual EFlow applyActionMsgProc(action::msg::Base* msg) override {
        switch (msg->id) {
            case action::msg::DoAction::CID: {
                getDbg()->setDebugMode(DebuggerMode_Break);
                getDbg()->execConsoleCmd("t");
                return EFlow::SUCCESS;
            } break;
            default:
                return Action::applyActionMsgProc(msg);
        }
    }
};
//////////////////////////////////////////////////////////////////////////


struct DisasmTraceStepOut : public Action {
    QDB_REG_ACTION(DisasmTraceStepOut)
    void setup() {
        mName = "Step Out";
        supportMtd.set(UiDrawEvent::MainMenu_Debug);
        addShortcut(qd::shortcut::EId::DebugTraceStepOut);
    }
    virtual EFlow applyActionMsgProc(action::msg::Base* msg) override {
        switch (msg->id) {
            case action::msg::DoAction::CID: {
                getDbg()->execConsoleCmd("z");
                return EFlow::SUCCESS;
            } break;
            default:
                return Action::applyActionMsgProc(msg);
        }
    }
};
//////////////////////////////////////////////////////////////////////////


struct DebugTraceStart : public Action {
    QDB_REG_ACTION(DebugTraceStart)
    void setup() {
        mName = "Debug Trace Mode";
        supportMtd.set(UiDrawEvent::MainMenu_Debug);
        addShortcut(qd::shortcut::EId::DebugTraceStart);
    }
    virtual EFlow applyActionMsgProc(action::msg::Base* msg) override {
        switch (msg->id) {
            case action::msg::DoAction::CID: {
                getDbg()->setDebugMode(DebuggerMode_Break);
                return EFlow::SUCCESS;
            } break;
            default:
                return Action::applyActionMsgProc(msg);
        }
    }
};
//////////////////////////////////////////////////////////////////////////


struct DebugTraceContinue : public Action {
    QDB_REG_ACTION(DebugTraceContinue)
    void setup() {
        mName = "Continue";
        supportMtd.set(UiDrawEvent::MainMenu_Debug);
        addShortcut(qd::shortcut::EId::DebugTraceContinue);
    }
    virtual EFlow applyActionMsgProc(action::msg::Base* msg) override {
        switch (msg->id) {
            case action::msg::DoAction::CID:
            case action::msg::DoDebugTraceContinue::CID: {
                getDbg()->execConsoleCmd("g");
                return EFlow::SUCCESS;
            } break;
            default:
                return Action::applyActionMsgProc(msg);
        }
    }
};
//////////////////////////////////////////////////////////////////////////


struct DebugWaitScanLines : public Action {
    QDB_REG_ACTION(DebugWaitScanLines)
    void setup() {
        mName = "Wait N scanlines";
        supportMtd.set(UiDrawEvent::MainMenu_Debug);
        addShortcut(qd::shortcut::EId::DebugWaitScanLines);
    }
    virtual EFlow applyActionMsgProc(action::msg::Base* msg) override {
        switch (msg->id) {
            case action::msg::DoAction::CID:
            case action::msg::DoDebugTraceContinue::CID: {
                eastl::string cmd;
                cmd.append_sprintf("fs %i", getDbg()->getWaitScanLines());
                getDbg()->execConsoleCmd(eastl::move(cmd));
                return EFlow::SUCCESS;
            } break;
            default:
                return Action::applyActionMsgProc(msg);
        }
    }
};
//////////////////////////////////////////////////////////////////////////


struct DisasmToggleBreakpoint : public Action {
    QDB_REG_ACTION(DisasmToggleBreakpoint)
    void setup() {
        mName = "Disasm breakpoint";
        supportMtd.set(UiDrawEvent::MainMenu_Debug);
        addShortcut(qd::shortcut::EId::DisasmToggleBreakpoint);
    }

    virtual EFlow applyActionMsgProc(action::msg::Base* msg) override {
        switch (msg->id) {
            case action::msg::DisasmToggleBreakpoint::CID: {
                auto p = msg->cast_<action::msg::DisasmToggleBreakpoint>();
                eastl::string cmd;
                cmd.sprintf("f %08x", (uint32_t)p->address);
                if (p->nBreakpoint >= 0)
                    cmd.append_sprintf(" %i", p->nBreakpoint);
                getDbg()->execConsoleCmd(eastl::move(cmd));
                return EFlow::SUCCESS;
            } break;
            default:
                return Action::applyActionMsgProc(msg);
        }
    }
};
//////////////////////////////////////////////////////////////////////////


struct CopperTraceStep : public Action {
    QDB_REG_ACTION(CopperTraceStep)
    void setup() {
        mName = "Copper Trace Step";
        supportMtd.set(UiDrawEvent::MainMenu_Debug);
        addShortcut(qd::shortcut::EId::CopperTraceStep);
    }
    virtual EFlow applyActionMsgProc(action::msg::Base* msg) override {
        switch (msg->id) {
            case action::msg::CopperTraceStep::CID:
            case action::msg::DoAction::CID: {
                getDbg()->execConsoleCmd("ot");
                return EFlow::SUCCESS;
            } break;
            default:
                return Action::applyActionMsgProc(msg);
        }
    }
};
//////////////////////////////////////////////////////////////////////////


struct CopperToggleBreakpoint : public Action {
    QDB_REG_ACTION(CopperToggleBreakpoint)
    void setup() {
        mName = "Copper breakpoint";
        supportMtd.set(UiDrawEvent::MainMenu_Debug);
        addShortcut(qd::shortcut::EId::CopperToggleBreakpoint);
    }
    virtual EFlow applyActionMsgProc(action::msg::Base* msg) override {
        switch (msg->id) {
            case action::msg::CopperToggleBreakpoint::CID: {
                auto p = msg->cast_<action::msg::CopperToggleBreakpoint>();
                eastl::string cmd;
                cmd.sprintf("ob %08x", (uint32_t)p->address);
                getDbg()->execConsoleCmd(eastl::move(cmd));
                return EFlow::SUCCESS;
            } break;
            default:
                return Action::applyActionMsgProc(msg);
        }
    }
};
//////////////////////////////////////////////////////////////////////////


struct ToggleTurboEmulation : public Action {
    QDB_REG_ACTION(ToggleTurboEmulation);
    void setup() {
        mName = "Turbo Emulation";
        supportMtd.set(UiDrawEvent::MainMenu_Emul);
        addShortcut(qd::shortcut::EId::ToggleTurboEmulation);
    }
    virtual EFlow applyActionMsgProc(action::msg::Base* msg) override {
        switch (msg->id) {
            case action::msg::DoAction::CID: {
                if (currprefs.turbo_emulation != 0) {
                    // off
                    warpmode(0);
                } else {
                    // on
                    warpmode(2);
                }
                return EFlow::SUCCESS;
            } break;
            default:
                return Action::applyActionMsgProc(msg);
        }
    }
};
//////////////////////////////////////////////////////////////////////////


struct UaeResetAmiga : public Action {
    QDB_REG_ACTION(UaeResetAmiga);
    void setup() {
        mName = "Reset Amiga";
        supportMtd.set(UiDrawEvent::MainMenu_Emul);
        addShortcut(qd::shortcut::EId::ResetAmigaEmu);
    }

    virtual EFlow applyActionMsgProc(action::msg::Base* msg) override {
        switch (msg->id) {
            case msg::DoAction::CID: {
                ::uae_reset(1, 1);
                return EFlow::SUCCESS;
            } break;
            default:
                return Action::applyActionMsgProc(msg);
        }
    }
};
//////////////////////////////////////////////////////////////////////////


struct UaeWndAlwaysOnTop : public Action {
    QDB_REG_ACTION(UaeWndAlwaysOnTop);
    void setup() {
        mName = "Always on Top";
        supportMtd.set(UiDrawEvent::MainMenu_Emul).set(UiDrawEvent::MenuItemStateChecked);
        addShortcut(qd::shortcut::EId::AlwaysOnTopEmu);
    }

    virtual EFlow applyActionMsgProc(action::msg::Base* msg) override {
        switch (msg->id) {
            case msg::DoAction::CID: {
                Uint32 flags = SDL_GetWindowFlags(app->mUaeWindow);
                bool setOnTop = (flags & SDL_WINDOW_ALWAYS_ON_TOP) != 0;
                SDL_SetWindowAlwaysOnTop(app->mUaeWindow, (SDL_bool)(!setOnTop));
                return EFlow::SUCCESS;
            } break;
            case msg::MenuItemStateGet::CID: {
                auto p = static_cast<msg::MenuItemStateGet*>(msg);
                Uint32 flags = SDL_GetWindowFlags(app->mUaeWindow);
                p->checked = (flags & SDL_WINDOW_ALWAYS_ON_TOP) ? 1 : 0;
                return EFlow::SUCCESS;
            } break;
            default:
                return Action::applyActionMsgProc(msg);
        }
    }
};
//////////////////////////////////////////////////////////////////////////


//#undef QDB_REG_ACTION

};  // namespace uae
};  // namespace action
};  // namespace qd
