#pragma once
#include <amDebugger/action_mgr.h>
#include <amDebugger/vm/memory.h>
#include <amDebugger/vm/vm.h>
#include <qdIce/qdBase/types.h>


namespace qd {
namespace action {
namespace msg {


struct MenuItemStateGet : Base {
    TS_REFLECT_CLASS(MenuItemStateGet, action::msg::Base);
    UiDrawEvent::Type menuType = UiDrawEvent::Undef;
    int checked = -1;
};



struct DoDebugTraceContinue : Base {
    TS_REFLECT_CLASS(DoDebugTraceContinue, action::msg::Base);
};


struct DisasmToggleBreakpoint : Base {
    TS_REFLECT_CLASS(DisasmToggleBreakpoint, action::msg::Base);
    AddrRef address = {};
    EReg reg = EReg::PC;
    int setBreakpoint = -1;
    int nBreakpoint = -1;
};


struct CopperToggleBreakpoint : Base {
    TS_REFLECT_CLASS(CopperToggleBreakpoint, action::msg::Base);
    AddrRef address = {};
};

struct CopperTraceStep : Base {
    TS_REFLECT_CLASS(CopperTraceStep, action::msg::Base);
};


};  // namespace msg
};  // namespace action
};  // namespace qd
