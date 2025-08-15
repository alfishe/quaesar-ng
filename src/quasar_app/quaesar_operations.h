#pragma once
#include "amDebugger/debuggerOps.h"
#include "quaesar_app.h"

namespace qsr::operation::args {


struct ShowDebuggerWnd : public amD::operation::OperationArgs {
    DECLARE_OPERATION_1(qsr::operation::args::ShowDebuggerWnd);
    EQuaServerId dbgSource = EQuaServerId::S_UAE;

    static void setup(qd::operation::args::OpDesc& d) {
        d.m_label = "Show debugger window";
        d.addShortcut(amD::shortcut::EId::ShowDebuggerWnd);
    }
};

};  //namespace qsr::operation::args
