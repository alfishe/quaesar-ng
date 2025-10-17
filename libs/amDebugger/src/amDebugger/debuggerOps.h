#pragma once
#include "qd/base/baseTypes.h"
#include <amDebugger/shortcutsList.h>
#include <amDebugger/vm/vmInterface.h>
#include <amDebugger/vm/memory.h>


FORWARD_DECLARATION_2(amD, Debugger);
FORWARD_DECLARATION_2(amD, DebuggerDesktop);
FORWARD_DECLARATION_3(amD, operation, Operation);
FORWARD_DECLARATION_3S(amD, shortcut, EId);

using namespace IVm;

namespace amD {

namespace operation {
using qd::operation::OpDesc;
static constexpr size_t MAX_OP_SIZE = 192;



class OperationArgs : public qd::operation::BaseOpArgs
{
public:
    static void setup(amD::operation::OpDesc& /*d*/) {}
};



//////////////////////////////////////////////////////////////////////////
template<class TClass>
static amD::operation::Operation* createOperationCb_(const qd::TypeInfo& meta, qd::UiOperationCreator* cp)
{
    TClass* pNewInst = new TClass();
    pNewInst->onOperationCreate(cp);
    pNewInst->setup();
    return pNewInst;
}


#define QDB_REG_OPERATION(ClassName)                                         \
    TS_BEGIN_REFLECT_CLASS(ClassName, amD::operations::Operation);           \
    TS_ATTRIBUTE(qd::tsAttr::CreateClassCb(&createOperationCb_<TRefClass>)); \
    TS_END();

//////////////////////////////////////////////////////////////////////////


struct AmDebuggerOperationCreator : public qd::UiOperationCreator {
    amD::DebuggerDesktop* gui = nullptr;
    amD::Debugger* dbg = nullptr;
}; // struct AmDebuggerOperationCreator
//////////////////////////////////////////////////////////////////////////



//------------------------------------------------------------------------
// Operations arguments
//
struct VmEmuReset : public amD::operation::OperationArgs {
    DECLARE_OPERATION_1(amD::operation::VmEmuReset);
    static void setup(qd::operation::OpDesc& d)
    {
        d.m_id = "reset";
        d.m_name = "Reset Amiga";
        d.addShortcut(amD::shortcut::EId::ResetAmigaEmu);
    }
};

struct ExecConsoleCmd : public amD::operation::OperationArgs {
    DECLARE_OPERATION_1(amD::operation::ExecConsoleCmd);
    qd::string cmd;
    static void setup(qd::operation::OpDesc& d)
    {
        d.m_id = "console";
        d.m_name = "Console command";
    }
};

struct DebugDmaOption : public amD::operation::OperationArgs {
    DECLARE_OPERATION_1(amD::operation::DebugDmaOption);
    int dmaMode = 0;
    static void setup(qd::operation::OpDesc& d)
    { d.m_name = "Debug DMA"; }
    inline static const char* dma_options = "off\0"
                                            "mode 2\0"
                                            "mode 3\0"
                                            "mode 4\0"
                                            "\0";

    int getCurDebugDmaMode(qd::IOperationEnvironment* env);
    void changeDebugDmaMode(qd::IOperationEnvironment* env, int nMode);
};

struct DebugTraceStart : public amD::operation::OperationArgs {
    DECLARE_OPERATION_1(amD::operation::DebugTraceStart)
    static void setup(qd::operation::OpDesc& d)
    {
        d.m_name = "Debug Trace Mode";
        d.addShortcut(amD::shortcut::EId::DebugTraceStart);
    }
};


struct DebugTraceContinue : public amD::operation::OperationArgs {
    DECLARE_OPERATION_1(amD::operation::DebugTraceContinue);
    static void setup(qd::operation::OpDesc& d)
    {
        d.m_name = "Continue";
        d.addShortcut(amD::shortcut::EId::DebugTraceContinue);
    }
    IVm::EVmDebugMode debugMode = EVmDebugMode::Live;
};



struct DisasmTraceStepInto : public amD::operation::OperationArgs {
    DECLARE_OPERATION_1(amD::operation::DisasmTraceStepInto);
    static void setup(qd::operation::OpDesc& d)
    {
        d.m_name = "Step Into";
        d.addShortcut(amD::shortcut::EId::DisasmTraceStepInto);
    }
};



struct DisasmTraceStepOut : public amD::operation::OperationArgs {
    DECLARE_OPERATION_1(amD::operation::DisasmTraceStepOut)
    static void setup(qd::operation::OpDesc& d)
    {
        d.m_name = "Step Out";
        d.addShortcut(amD::shortcut::EId::DisasmTraceStepOut);
    }
};


struct CopperTraceStep : public amD::operation::OperationArgs {
    DECLARE_OPERATION_1(amD::operation::CopperTraceStep)
    static void setup(qd::operation::OpDesc& d)
    {
        d.m_name = "Copper Trace Step";
        d.addShortcut(amD::shortcut::EId::CopperTraceStep);
    }
};


struct DisasmToggleBreakpoint : public amD::operation::OperationArgs {
    DECLARE_OPERATION_1(amD::operation::DisasmToggleBreakpoint);
    static void setup(qd::operation::OpDesc& d)
    {
        d.m_name = "Disasm breakpoint";
        d.addShortcut(amD::shortcut::EId::DisasmToggleBreakpoint);
    }
    AddrRef address = {};
    EReg reg = EReg::PC;
    int setBreakpoint = -1;
    int nBreakpoint = -1;
};


struct CopperToggleBreakpoint : public amD::operation::OperationArgs {
    DECLARE_OPERATION_1(amD::operation::CopperToggleBreakpoint);
    AddrRef address = {};
    static void setup(qd::operation::OpDesc& d)
    {
        d.m_name = "Copper breakpoint";
        d.addShortcut(amD::shortcut::EId::CopperToggleBreakpoint);
    }
};


struct ToggleTurboEmulation : public amD::operation::OperationArgs {
    DECLARE_OPERATION_1(amD::operation::ToggleTurboEmulation);
    static void setup(qd::operation::OpDesc& d)
    {
        d.m_name = "Turbo Emulation";
        d.addShortcut(amD::shortcut::EId::ToggleTurboEmulation);
    }
};


struct DebugWaitScanLines : public amD::operation::OperationArgs {
    DECLARE_OPERATION_1(amD::operation::DebugWaitScanLines);
    int waitScanLines = 1;
    static void setup(qd::operation::OpDesc& d)
    {
        d.m_name = "Wait N scanlines";
        d.addShortcut(amD::shortcut::EId::DebugWaitScanLines);
    }
};



struct VmPlayerWndAlwaysOnTop : public amD::operation::OperationArgs {
    DECLARE_OPERATION_1(amD::operation::VmPlayerWndAlwaysOnTop);
    static void setup(qd::operation::OpDesc& d)
    {
        d.m_name = "Always on Top";
        d.addShortcut(amD::shortcut::EId::AlwaysOnTopEmu);
    }
};



}; // namespace operations
}; // namespace amD
