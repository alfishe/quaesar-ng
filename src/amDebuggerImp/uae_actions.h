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


struct DebugDmaOption : public Action {
    QDB_REG_ACTION(qd::action::DebugDmaOption);
    void setup() {
        m_name = "Debug DMA";
    }

    void onDrawMainMenuItem(UiDrawEvent::Type event);

    virtual EFlow applyActionMsgProc(action::msg::Base* p_msg) override;
};
//////////////////////////////////////////////////////////////////////////


struct DisasmTraceStep : public Action {
    QDB_REG_ACTION(qd::action::DisasmTraceStep);
    void setup() {
        m_name = "Step Into";
        addShortcut(qd::shortcut::EId::DebugTraceStepInto);
    }
    virtual EFlow applyActionMsgProc(action::msg::Base* msg) override {
        if (auto p = msg->cast_<action::msg::DoAction>()) {
            getDbg()->setDebugMode(DebuggerMode_Break);
            getDbg()->execConsoleCmd("t");
            return EFlow::SUCCESS;
        } else
            return Action::applyActionMsgProc(msg);
    }
};
//////////////////////////////////////////////////////////////////////////


struct DisasmTraceStepOut : public Action {
    QDB_REG_ACTION(qd::action::DisasmTraceStepOut)
    void setup() {
        m_name = "Step Out";
        addShortcut(qd::shortcut::EId::DebugTraceStepOut);
    }
    virtual EFlow applyActionMsgProc(action::msg::Base* msg) override {
        if (auto p = msg->cast_<action::msg::DoAction>()) {
            getDbg()->execConsoleCmd("z");
            return EFlow::SUCCESS;
        } else
            return Action::applyActionMsgProc(msg);
    }
};
//////////////////////////////////////////////////////////////////////////


struct DebugTraceStart : public Action {
    QDB_REG_ACTION(qd::action::DebugTraceStart)
    void setup() {
        m_name = "Debug Trace Mode";
        addShortcut(qd::shortcut::EId::DebugTraceStart);
    }
    virtual EFlow applyActionMsgProc(action::msg::Base* msg) override {
        if (auto p = msg->cast_<action::msg::DoAction>()) {
            getDbg()->setDebugMode(DebuggerMode_Break);
            return EFlow::SUCCESS;
        } else
            return Action::applyActionMsgProc(msg);
    }
};
//////////////////////////////////////////////////////////////////////////


struct DebugTraceContinue : public Action {
    QDB_REG_ACTION(qd::action::DebugTraceContinue)
    void setup() {
        m_name = "Continue";
        addShortcut(qd::shortcut::EId::DebugTraceContinue);
    }
    virtual EFlow applyActionMsgProc(action::msg::Base* msg) override {
        if (msg->cast_<action::msg::DoAction>() || msg->cast_<action::msg::DoDebugTraceContinue>()) {
            getDbg()->execConsoleCmd("g");
            return EFlow::SUCCESS;
        } else
            return Action::applyActionMsgProc(msg);
    }
};
//////////////////////////////////////////////////////////////////////////


struct DebugWaitScanLines : public Action {
    QDB_REG_ACTION(qd::action::DebugWaitScanLines)
    void setup() {
        m_name = "Wait N scanlines";
        addShortcut(qd::shortcut::EId::DebugWaitScanLines);
    }
    virtual EFlow applyActionMsgProc(action::msg::Base* msg) override {
        if (msg->cast_<action::msg::DoAction>() || msg->cast_<action::msg::DoDebugTraceContinue>()) {
            eastl::string cmd;
            cmd.append_sprintf("fs %i", getDbg()->getWaitScanLines());
            getDbg()->execConsoleCmd(eastl::move(cmd));
            return EFlow::SUCCESS;
        } else
            return Action::applyActionMsgProc(msg);
    }
};
//////////////////////////////////////////////////////////////////////////


struct DisasmToggleBreakpoint : public Action {
    QDB_REG_ACTION(qd::action::DisasmToggleBreakpoint)
    void setup() {
        m_name = "Disasm breakpoint";
        addShortcut(qd::shortcut::EId::DisasmToggleBreakpoint);
    }

    virtual EFlow applyActionMsgProc(action::msg::Base* msg) override {
        if (auto p = msg->cast_<action::msg::DisasmToggleBreakpoint>()) {
            eastl::string cmd;
            cmd.sprintf("f %08x", (uint32_t)p->address);
            if (p->nBreakpoint >= 0)
                cmd.append_sprintf(" %i", p->nBreakpoint);
            getDbg()->execConsoleCmd(eastl::move(cmd));
            return EFlow::SUCCESS;
        } else
            return Action::applyActionMsgProc(msg);
    }
};
//////////////////////////////////////////////////////////////////////////


struct CopperTraceStep : public Action {
    QDB_REG_ACTION(qd::action::CopperTraceStep)
    void setup() {
        m_name = "Copper Trace Step";
        addShortcut(qd::shortcut::EId::CopperTraceStep);
    }
    virtual EFlow applyActionMsgProc(action::msg::Base* msg) override {
        if (msg->cast_<action::msg::DoAction>() || msg->cast_<action::msg::CopperTraceStep>()) {
            getDbg()->execConsoleCmd("ot");
            return EFlow::SUCCESS;
        } else
            return Action::applyActionMsgProc(msg);
    }
};
//////////////////////////////////////////////////////////////////////////


struct CopperToggleBreakpoint : public Action {
    QDB_REG_ACTION(qd::action::CopperToggleBreakpoint)
    void setup() {
        m_name = "Copper breakpoint";
        addShortcut(qd::shortcut::EId::CopperToggleBreakpoint);
    }
    virtual EFlow applyActionMsgProc(action::msg::Base* msg) override {
        if (auto p = msg->cast_<action::msg::CopperToggleBreakpoint>()) {
            eastl::string cmd;
            cmd.sprintf("ob %08x", (uint32_t)p->address);
            getDbg()->execConsoleCmd(eastl::move(cmd));
            return EFlow::SUCCESS;
        } else
            return Action::applyActionMsgProc(msg);
    }
};
//////////////////////////////////////////////////////////////////////////


struct ToggleTurboEmulation : public Action {
    QDB_REG_ACTION(qd::action::ToggleTurboEmulation);
    void setup() {
        m_name = "Turbo Emulation";
        addShortcut(qd::shortcut::EId::ToggleTurboEmulation);
    }
    virtual EFlow applyActionMsgProc(action::msg::Base* msg) override {
        if (auto p = msg->cast_<action::msg::DoAction>()) {
            if (currprefs.turbo_emulation != 0) {
                // off
                warpmode(0);
            } else {
                // on
                warpmode(2);
            }
            return EFlow::SUCCESS;
        } else
            return Action::applyActionMsgProc(msg);
    }
};
//////////////////////////////////////////////////////////////////////////


struct UaeResetAmiga : public Action {
    QDB_REG_ACTION(qd::action::UaeResetAmiga);
    void setup() {
        m_name = "Reset Amiga";
        addShortcut(qd::shortcut::EId::ResetAmigaEmu);
    }

    virtual EFlow applyActionMsgProc(action::msg::Base* msg) override {
        if (auto p = msg->cast_<action::msg::DoAction>()) {
            ::uae_reset(1, 1);
            return EFlow::SUCCESS;
        } else
            return Action::applyActionMsgProc(msg);
    }
};
//////////////////////////////////////////////////////////////////////////


struct UaeWndAlwaysOnTop : public Action {
    QDB_REG_ACTION(qd::action::UaeWndAlwaysOnTop);
    void setup() {
        m_name = "Always on Top";
        //supportMtd.set(UiDrawEvent::MainMenu_Emul).set(UiDrawEvent::MenuItemStateChecked);
        addShortcut(qd::shortcut::EId::AlwaysOnTopEmu);
    }

    virtual EFlow applyActionMsgProc(action::msg::Base* msg) override;
};
//////////////////////////////////////////////////////////////////////////


//#undef QDB_REG_ACTION

};  // namespace action
};  // namespace qd
