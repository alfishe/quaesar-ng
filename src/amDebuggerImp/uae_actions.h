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
#include <amDebugger/dbgOperation.h>
#include <amDebugger/debugger.h>
#include <amDebugger/msg_list.h>
#include <amDebugger/shortcut/shortcut_list.h>
#include <quaesar.h>
#include <uae_src/include/debug.h>
#include "qd/qdImGui/imgui_eastl.h"
#include "qd/qdTypeSystem/attributesCommon.h"


//////////////////////////////////////////////////////////////////////////
// These are private UAE operations and in a good case they should not be called directly by name,
// instead they are should triggered via messages (dbg->applyOperationMsg(m)) or shortcuts.
//

namespace qd {
namespace operation {


struct DebugDmaOption : public Operation {
    QDB_REG_OPERATION(qd::operation::DebugDmaOption);
    void setup() {
        m_name = "Debug DMA";
    }

    void onDrawMainMenuItem(UiDrawEvent::Type event);

    virtual EFlow applyOperationMsgProc(operation::msg::Base* p_msg) override;
};
//////////////////////////////////////////////////////////////////////////


struct DisasmTraceStep : public Operation {
    QDB_REG_OPERATION(qd::operation::DisasmTraceStep);
    void setup() {
        m_name = "Step Into";
        addShortcut(qd::shortcut::EId::DebugTraceStepInto);
    }
    virtual EFlow applyOperationMsgProc(operation::msg::Base* msg) override {
        if (auto p = msg->cast_<operation::msg::DoOperation>()) {
            getDbg()->setDebugMode(DebuggerMode_Break);
            getDbg()->execConsoleCmd("t");
            return EFlow::SUCCESS;
        } else
            return Operation::applyOperationMsgProc(msg);
    }
};
//////////////////////////////////////////////////////////////////////////


struct DisasmTraceStepOut : public Operation {
    QDB_REG_OPERATION(qd::operation::DisasmTraceStepOut)
    void setup() {
        m_name = "Step Out";
        addShortcut(qd::shortcut::EId::DebugTraceStepOut);
    }
    virtual EFlow applyOperationMsgProc(operation::msg::Base* msg) override {
        if (auto p = msg->cast_<operation::msg::DoOperation>()) {
            getDbg()->execConsoleCmd("z");
            return EFlow::SUCCESS;
        } else
            return Operation::applyOperationMsgProc(msg);
    }
};
//////////////////////////////////////////////////////////////////////////


struct DebugTraceStart : public Operation {
    QDB_REG_OPERATION(qd::operation::DebugTraceStart)
    void setup() {
        m_name = "Debug Trace Mode";
        addShortcut(qd::shortcut::EId::DebugTraceStart);
    }
    virtual EFlow applyOperationMsgProc(operation::msg::Base* msg) override {
        if (auto p = msg->cast_<operation::msg::DoOperation>()) {
            getDbg()->setDebugMode(DebuggerMode_Break);
            return EFlow::SUCCESS;
        } else
            return Operation::applyOperationMsgProc(msg);
    }
};
//////////////////////////////////////////////////////////////////////////


struct DebugTraceContinue : public Operation {
    QDB_REG_OPERATION(qd::operation::DebugTraceContinue)
    void setup() {
        m_name = "Continue";
        addShortcut(qd::shortcut::EId::DebugTraceContinue);
    }
    virtual EFlow applyOperationMsgProc(operation::msg::Base* msg) override {
        if (msg->cast_<operation::msg::DoOperation>() || msg->cast_<operation::msg::DoDebugTraceContinue>()) {
            getDbg()->execConsoleCmd("g");
            return EFlow::SUCCESS;
        } else
            return Operation::applyOperationMsgProc(msg);
    }
};
//////////////////////////////////////////////////////////////////////////


struct DebugWaitScanLines : public Operation {
    QDB_REG_OPERATION(qd::operation::DebugWaitScanLines)
    void setup() {
        m_name = "Wait N scanlines";
        addShortcut(qd::shortcut::EId::DebugWaitScanLines);
    }
    virtual EFlow applyOperationMsgProc(operation::msg::Base* msg) override {
        if (msg->cast_<operation::msg::DoOperation>() || msg->cast_<operation::msg::DoDebugTraceContinue>()) {
            eastl::string cmd;
            cmd.append_sprintf("fs %i", getDbg()->getWaitScanLines());
            getDbg()->execConsoleCmd(eastl::move(cmd));
            return EFlow::SUCCESS;
        } else
            return Operation::applyOperationMsgProc(msg);
    }
};
//////////////////////////////////////////////////////////////////////////


struct DisasmToggleBreakpoint : public Operation {
    QDB_REG_OPERATION(qd::operation::DisasmToggleBreakpoint)
    void setup() {
        m_name = "Disasm breakpoint";
        addShortcut(qd::shortcut::EId::DisasmToggleBreakpoint);
    }

    virtual EFlow applyOperationMsgProc(operation::msg::Base* msg) override {
        if (auto p = msg->cast_<operation::msg::DisasmToggleBreakpoint>()) {
            eastl::string cmd;
            cmd.sprintf("f %08x", (uint32_t)p->address);
            if (p->nBreakpoint >= 0)
                cmd.append_sprintf(" %i", p->nBreakpoint);
            getDbg()->execConsoleCmd(eastl::move(cmd));
            return EFlow::SUCCESS;
        } else
            return Operation::applyOperationMsgProc(msg);
    }
};
//////////////////////////////////////////////////////////////////////////


struct CopperTraceStep : public Operation {
    QDB_REG_OPERATION(qd::operation::CopperTraceStep)
    void setup() {
        m_name = "Copper Trace Step";
        addShortcut(qd::shortcut::EId::CopperTraceStep);
    }
    virtual EFlow applyOperationMsgProc(operation::msg::Base* msg) override {
        if (msg->cast_<operation::msg::DoOperation>() || msg->cast_<operation::msg::CopperTraceStep>()) {
            getDbg()->execConsoleCmd("ot");
            return EFlow::SUCCESS;
        } else
            return Operation::applyOperationMsgProc(msg);
    }
};
//////////////////////////////////////////////////////////////////////////


struct CopperToggleBreakpoint : public Operation {
    QDB_REG_OPERATION(qd::operation::CopperToggleBreakpoint)
    void setup() {
        m_name = "Copper breakpoint";
        addShortcut(qd::shortcut::EId::CopperToggleBreakpoint);
    }
    virtual EFlow applyOperationMsgProc(operation::msg::Base* msg) override {
        if (auto p = msg->cast_<operation::msg::CopperToggleBreakpoint>()) {
            eastl::string cmd;
            cmd.sprintf("ob %08x", (uint32_t)p->address);
            getDbg()->execConsoleCmd(eastl::move(cmd));
            return EFlow::SUCCESS;
        } else
            return Operation::applyOperationMsgProc(msg);
    }
};
//////////////////////////////////////////////////////////////////////////


struct ToggleTurboEmulation : public Operation {
    QDB_REG_OPERATION(qd::operation::ToggleTurboEmulation);
    void setup() {
        m_name = "Turbo Emulation";
        addShortcut(qd::shortcut::EId::ToggleTurboEmulation);
    }
    virtual EFlow applyOperationMsgProc(operation::msg::Base* msg) override {
        if (auto p = msg->cast_<operation::msg::DoOperation>()) {
            if (currprefs.turbo_emulation != 0) {
                // off
                warpmode(0);
            } else {
                // on
                warpmode(2);
            }
            return EFlow::SUCCESS;
        } else
            return Operation::applyOperationMsgProc(msg);
    }
};
//////////////////////////////////////////////////////////////////////////


struct UaeResetAmiga : public Operation {
    QDB_REG_OPERATION(qd::operation::UaeResetAmiga);
    void setup() {
        m_name = "Reset Amiga";
        addShortcut(qd::shortcut::EId::ResetAmigaEmu);
    }

    virtual EFlow applyOperationMsgProc(operation::msg::Base* msg) override {
        if (auto p = msg->cast_<operation::msg::DoOperation>()) {
            ::uae_reset(1, 1);
            return EFlow::SUCCESS;
        } else
            return Operation::applyOperationMsgProc(msg);
    }
};
//////////////////////////////////////////////////////////////////////////


struct UaeWndAlwaysOnTop : public Operation {
    QDB_REG_OPERATION(qd::operation::UaeWndAlwaysOnTop);
    void setup() {
        m_name = "Always on Top";
        //supportMtd.set(UiDrawEvent::MainMenu_Emul).set(UiDrawEvent::MenuItemStateChecked);
        addShortcut(qd::shortcut::EId::AlwaysOnTopEmu);
    }

    virtual EFlow applyOperationMsgProc(operation::msg::Base* msg) override;
};
//////////////////////////////////////////////////////////////////////////


//#undef QDB_REG_OPERATION

};  // namespace operation
};  // namespace qd
