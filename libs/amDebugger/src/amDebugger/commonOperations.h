#pragma once
#include "amDebugger/dbgOperation.h"
#include "amDebugger/shortcutsList.h"
#include "amDebugger/debuggerOps.h"


struct SDL_Window;


namespace amD::operation {


struct DebugDmaOption : public amD::operation::Operation {
    QDB_REG_OPERATION(amD::operation::DebugDmaOption);

    void setup() { m_name = "Debug DMA"; }
    inline static const char* dma_options = "off\0"
                                            "mode 2\0"
                                            "mode 3\0"
                                            "mode 4\0"
                                            "\0";

    qd::EFlow applyOperationMsgProc(qd::IOperationEnvironment* env, amD::operation::OperationArgs* p_msg) override
    {
        if (auto p = p_msg->cast_<qd::operation::args::DoOperation>())
        {
            changeDebugDmaMode(env, p->arg0.getInt());
            return qd::EFlow::SUCCESS;
        }
        else
            return Operation::applyOperationMsgProc(env, p_msg);
    }

    int getCurDebugDmaMode(qd::IOperationEnvironment* env);
    void changeDebugDmaMode(qd::IOperationEnvironment* env, int nMode);
};
//////////////////////////////////////////////////////////////////////////



struct UaeWndAlwaysOnTop : public amD::operation::Operation {
    QDB_REG_OPERATION(amD::operation::UaeWndAlwaysOnTop);
    void setup()
    {
        m_name = "Always on Top";
        addShortcut(amD::shortcut::EId::AlwaysOnTopEmu);
    }

    virtual qd::EFlow applyOperationMsgProc(qd::IOperationEnvironment* env, amD::operation::OperationArgs* p_msg) override;

    SDL_Window* getEmulatorMainWindow(qd::IOperationEnvironment* env);
};
//////////////////////////////////////////////////////////////////////////



struct DisasmTraceStep : public amD::operation::Operation {
    QDB_REG_OPERATION(amD::operation::DisasmTraceStep);
    void setup()
    {
        m_name = "Step Into";
        addShortcut(amD::shortcut::EId::DebugTraceStepInto);
    }
    virtual void doOperation(qd::IOperationEnvironment* env) override;
};
//////////////////////////////////////////////////////////////////////////


struct DisasmTraceStepOut : public amD::operation::Operation {
    QDB_REG_OPERATION(amD::operation::DisasmTraceStepOut)
    void setup()
    {
        m_name = "Step Out";
        addShortcut(amD::shortcut::EId::DebugTraceStepOut);
    }
    virtual void doOperation(qd::IOperationEnvironment* env) override;
};
//////////////////////////////////////////////////////////////////////////


struct DebugTraceStart : public amD::operation::Operation {
    QDB_REG_OPERATION(amD::operation::DebugTraceStart)
    void setup()
    {
        m_name = "Debug Trace Mode";
        addShortcut(amD::shortcut::EId::DebugTraceStart);
    }
    virtual void doOperation(qd::IOperationEnvironment* env) override;
};
//////////////////////////////////////////////////////////////////////////


extern void uae_op_debug_trace_continue(qd::IOperationEnvironment* env, amD::operation::args::DoDebugTraceContinue* p);

struct DebugTraceContinue : public amD::operation::Operation {
    QDB_REG_OPERATION(amD::operation::DebugTraceContinue)
    void setup()
    {
        m_name = "Continue";
        addShortcut(amD::shortcut::EId::DebugTraceContinue);
    }
    virtual qd::EFlow applyOperationMsgProc(qd::IOperationEnvironment* env, amD::operation::OperationArgs* p_msg) override
    {
        if (auto args = p_msg->cast_<amD::operation::args::DoDebugTraceContinue>())
            uae_op_debug_trace_continue(env, args);
        return qd::EFlow::DONE;
    }
};
//////////////////////////////////////////////////////////////////////////


struct DebugWaitScanLines : public amD::operation::Operation {
    QDB_REG_OPERATION(amD::operation::DebugWaitScanLines)
    void setup()
    {
        m_name = "Wait N scanlines";
        addShortcut(amD::shortcut::EId::DebugWaitScanLines);
    }
    virtual qd::EFlow applyOperationMsgProc(qd::IOperationEnvironment* env, amD::operation::OperationArgs* p_msg) override;
};
//////////////////////////////////////////////////////////////////////////


struct DisasmToggleBreakpoint : public amD::operation::Operation {
    QDB_REG_OPERATION(amD::operation::DisasmToggleBreakpoint)
    void setup()
    {
        m_name = "Disasm breakpoint";
        addShortcut(amD::shortcut::EId::DisasmToggleBreakpoint);
    }

    virtual qd::EFlow applyOperationMsgProc(qd::IOperationEnvironment* env, amD::operation::OperationArgs* p_msg) override;
};
//////////////////////////////////////////////////////////////////////////


struct CopperTraceStep : public Operation {
    QDB_REG_OPERATION(amD::operation::CopperTraceStep)
    void setup()
    {
        m_name = "Copper Trace Step";
        addShortcut(amD::shortcut::EId::CopperTraceStep);
    }
    virtual qd::EFlow applyOperationMsgProc(qd::IOperationEnvironment* env, amD::operation::OperationArgs* p_msg) override;
};
//////////////////////////////////////////////////////////////////////////


struct CopperToggleBreakpoint : public Operation {
    QDB_REG_OPERATION(amD::operation::CopperToggleBreakpoint)
    void setup()
    {
        m_name = "Copper breakpoint";
        addShortcut(amD::shortcut::EId::CopperToggleBreakpoint);
    }
    virtual qd::EFlow applyOperationMsgProc(qd::IOperationEnvironment* env, amD::operation::OperationArgs* p_msg) override;
};
//////////////////////////////////////////////////////////////////////////


struct ToggleTurboEmulation : public Operation {
    QDB_REG_OPERATION(amD::operation::ToggleTurboEmulation);
    void setup()
    {
        m_name = "Turbo Emulation";
        addShortcut(amD::shortcut::EId::ToggleTurboEmulation);
    }
    virtual void doOperation(qd::IOperationEnvironment* env) override;
};
//////////////////////////////////////////////////////////////////////////


struct UaeResetAmiga : public Operation {
    QDB_REG_OPERATION(amD::operation::UaeResetAmiga);
    void setup()
    {
        m_name = "Reset Amiga";
        addShortcut(amD::shortcut::EId::ResetAmigaEmu);
    }

    virtual void doOperation(qd::IOperationEnvironment* env) override;
};
//////////////////////////////////////////////////////////////////////////


}; // namespace amD::operation
