#pragma once
#include <amDebugger/dbgOperation.h>
#include <amDebugger/vm/memory.h>
#include <amDebugger/vm/vm.h>
#include <qdIce/qdBase/types.h>


namespace qd {
namespace operation {
namespace msg {


struct MenuItemStateGet : Base {
    TS_REFLECT_CLASS(MenuItemStateGet, operation::msg::Base);
    UiDrawEvent::Type menuType = UiDrawEvent::Undef;
    int checked = -1;
};



struct DoDebugTraceContinue : Base {
    TS_REFLECT_CLASS(DoDebugTraceContinue, operation::msg::Base);
};


struct DisasmToggleBreakpoint : Base {
    TS_REFLECT_CLASS(DisasmToggleBreakpoint, operation::msg::Base);
    AddrRef address = {};
    EReg reg = EReg::PC;
    int setBreakpoint = -1;
    int nBreakpoint = -1;
};


struct CopperToggleBreakpoint : Base {
    TS_REFLECT_CLASS(CopperToggleBreakpoint, operation::msg::Base);
    AddrRef address = {};
};

struct CopperTraceStep : Base {
    TS_REFLECT_CLASS(CopperTraceStep, operation::msg::Base);
};


};  // namespace msg
};  // namespace operation
};  // namespace qd
