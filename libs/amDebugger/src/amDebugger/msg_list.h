#pragma once
#include <amDebugger/dbgOperation.h>
#include <amDebugger/vm/memory.h>
#include <amDebugger/vm/vm.h>
#include <qd/base/types.h>
#include "qd/qui/uiOperationMessages.h"


namespace amD {
namespace operation {
namespace msg {


struct MenuItemStateGet : qd::operation::msg::Base {
    TS_REFLECT_CLASS(MenuItemStateGet, qd::operation::msg::Base);
    int checked = -1;
};



struct DoDebugTraceContinue : qd::operation::msg::Base {
    TS_REFLECT_CLASS(DoDebugTraceContinue, qd::operation::msg::Base);
};


struct DisasmToggleBreakpoint : qd::operation::msg::Base {
    TS_REFLECT_CLASS(DisasmToggleBreakpoint, qd::operation::msg::Base);
    AddrRef address = {};
    EReg reg = EReg::PC;
    int setBreakpoint = -1;
    int nBreakpoint = -1;
};


struct CopperToggleBreakpoint : qd::operation::msg::Base {
    TS_REFLECT_CLASS(CopperToggleBreakpoint, qd::operation::msg::Base);
    AddrRef address = {};
};


struct CopperTraceStep : qd::operation::msg::Base {
    TS_REFLECT_CLASS(CopperTraceStep, qd::operation::msg::Base);
};


};  // namespace msg
};  // namespace operation
};  // namespace amD
