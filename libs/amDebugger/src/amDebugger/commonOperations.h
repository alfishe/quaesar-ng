#pragma once
#include "amDebugger/dbgOperation.h"
#include "amDebugger/shortcut_list.h"


struct SDL_Window;


namespace amD::operation {


struct DebugDmaOption : public amD::operation::Operation {
    QDB_REG_OPERATION(amD::operation::DebugDmaOption);

    void setup() { m_name = "Debug DMA"; }

    qd::EFlow applyOperationMsgProc(qd::operation::args::Base* p_msg) override
    {
        if (auto p = p_msg->cast_<qd::operation::args::DoOperation>())
        {
            changeDebugDmaMode(p->arg0.getInt());
            return qd::EFlow::SUCCESS;
        }
        else
            return Operation::applyOperationMsgProc(p_msg);

    }

    int getCurDebugDmaMode();
    void changeDebugDmaMode(int nMode);
};
//////////////////////////////////////////////////////////////////////////



struct UaeWndAlwaysOnTop : public amD::operation::Operation {
    QDB_REG_OPERATION(amD::operation::UaeWndAlwaysOnTop);
    void setup()
    {
        m_name = "Always on Top";
        // supportMtd.set(UiDrawEvent::MainMenu_Emul).set(UiDrawEvent::MenuItemStateChecked);
        addShortcut(amD::shortcut::EId::AlwaysOnTopEmu);
    }

    virtual qd::EFlow applyOperationMsgProc(qd::operation::args::Base* p_msg) override;

    SDL_Window* getEmulatorMainWindow();
};
//////////////////////////////////////////////////////////////////////////



struct DisasmTraceStep : public amD::operation::Operation {
    QDB_REG_OPERATION(amD::operation::DisasmTraceStep);
    void setup()
    {
        m_name = "Step Into";
        addShortcut(amD::shortcut::EId::DebugTraceStepInto);
    }
    virtual void doOperation(qd::OperationEnvironment* history = nullptr) override;
};
//////////////////////////////////////////////////////////////////////////


struct DisasmTraceStepOut : public amD::operation::Operation {
    QDB_REG_OPERATION(amD::operation::DisasmTraceStepOut)
    void setup()
    {
        m_name = "Step Out";
        addShortcut(amD::shortcut::EId::DebugTraceStepOut);
    }
    virtual void doOperation(qd::OperationEnvironment* history = nullptr) override;
};
//////////////////////////////////////////////////////////////////////////


struct DebugTraceStart : public amD::operation::Operation {
    QDB_REG_OPERATION(amD::operation::DebugTraceStart)
    void setup()
    {
        m_name = "Debug Trace Mode";
        addShortcut(amD::shortcut::EId::DebugTraceStart);
    }
    virtual void doOperation(qd::OperationEnvironment* history = nullptr) override;
};
//////////////////////////////////////////////////////////////////////////


struct DebugTraceContinue : public amD::operation::Operation {
    QDB_REG_OPERATION(amD::operation::DebugTraceContinue)
    void setup()
    {
        m_name = "Continue";
        addShortcut(amD::shortcut::EId::DebugTraceContinue);
    }
    virtual qd::EFlow applyOperationMsgProc(qd::operation::args::Base* msg) override;
};
//////////////////////////////////////////////////////////////////////////


struct DebugWaitScanLines : public amD::operation::Operation {
    QDB_REG_OPERATION(amD::operation::DebugWaitScanLines)
    void setup()
    {
        m_name = "Wait N scanlines";
        addShortcut(amD::shortcut::EId::DebugWaitScanLines);
    }
    virtual qd::EFlow applyOperationMsgProc(qd::operation::args::Base* msg) override;
};
//////////////////////////////////////////////////////////////////////////


struct DisasmToggleBreakpoint : public amD::operation::Operation {
    QDB_REG_OPERATION(amD::operation::DisasmToggleBreakpoint)
    void setup()
    {
        m_name = "Disasm breakpoint";
        addShortcut(amD::shortcut::EId::DisasmToggleBreakpoint);
    }

    virtual qd::EFlow applyOperationMsgProc(qd::operation::args::Base* msg) override;
};
//////////////////////////////////////////////////////////////////////////


struct CopperTraceStep : public Operation {
    QDB_REG_OPERATION(amD::operation::CopperTraceStep)
    void setup()
    {
        m_name = "Copper Trace Step";
        addShortcut(amD::shortcut::EId::CopperTraceStep);
    }
    virtual qd::EFlow applyOperationMsgProc(qd::operation::args::Base* msg) override;
};
//////////////////////////////////////////////////////////////////////////


struct CopperToggleBreakpoint : public Operation {
    QDB_REG_OPERATION(amD::operation::CopperToggleBreakpoint)
    void setup()
    {
        m_name = "Copper breakpoint";
        addShortcut(amD::shortcut::EId::CopperToggleBreakpoint);
    }
    virtual qd::EFlow applyOperationMsgProc(qd::operation::args::Base* msg) override;
};
//////////////////////////////////////////////////////////////////////////


struct ToggleTurboEmulation : public Operation {
    QDB_REG_OPERATION(amD::operation::ToggleTurboEmulation);
    void setup()
    {
        m_name = "Turbo Emulation";
        addShortcut(amD::shortcut::EId::ToggleTurboEmulation);
    }
    virtual void doOperation(qd::OperationEnvironment* history = nullptr) override;
};
//////////////////////////////////////////////////////////////////////////


struct UaeResetAmiga : public Operation {
    QDB_REG_OPERATION(amD::operation::UaeResetAmiga);
    void setup()
    {
        m_name = "Reset Amiga";
        addShortcut(amD::shortcut::EId::ResetAmigaEmu);
    }

    virtual void doOperation(qd::OperationEnvironment* history = nullptr) override;
};
//////////////////////////////////////////////////////////////////////////


}; // namespace amD::operation
