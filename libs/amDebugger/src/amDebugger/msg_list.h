#pragma once
#include <amDebugger/dbgOperation.h>
#include <amDebugger/vm/memory.h>
#include <amDebugger/vm/absEmu.h>
#include "qd/base/types.h"
#include "qd/qui/uiOperationArgs.h"


namespace amD {
namespace operation {
namespace args {


struct MenuItemStateGet : qd::operation::args::Base {
    //TS_REFLECT_CLASS(MenuItemStateGet, qd::operation::args::Base);
    DECLARE_OPERATION(amD::operation::args::MenuItemStateGet, amD::operation::UaeWndAlwaysOnTop);
    int checked = -1;
};



struct DoDebugTraceContinue : qd::operation::args::Base {
    //TS_REFLECT_CLASS(DoDebugTraceContinue, qd::operation::args::Base);
    DECLARE_OPERATION(amD::operation::args::DoDebugTraceContinue, amD::operation::DebugTraceContinue);
};


struct DisasmToggleBreakpoint : qd::operation::args::Base {
    //TS_REFLECT_CLASS(amD::operation::args::DisasmToggleBreakpoint, qd::operation::args::Base);
    DECLARE_OPERATION(amD::operation::args::DisasmToggleBreakpoint, amD::operation::DisasmToggleBreakpoint);
    AddrRef address = {};
    EReg reg = EReg::PC;
    int setBreakpoint = -1;
    int nBreakpoint = -1;
};


struct CopperToggleBreakpoint : qd::operation::args::Base {
    //TS_REFLECT_CLASS(CopperToggleBreakpoint, qd::operation::args::Base);
    DECLARE_OPERATION(amD::operation::args::CopperToggleBreakpoint, amD::operation::CopperToggleBreakpoint);
    AddrRef address = {};
};


struct CopperTraceStep : qd::operation::args::Base {
    //TS_REFLECT_CLASS(CopperTraceStep, qd::operation::args::Base);
    DECLARE_OPERATION(amD::operation::args::CopperTraceStep, amD::operation::CopperTraceStep);
};


};  // namespace args
};  // namespace operation
};  // namespace amD
