#pragma once
#include "amDebugger/debuggerOps.h"
#include "quaesar_app.h"

namespace qsr::operations {


struct ShowDebuggerWnd : public amD::operation::OperationArgs {
    DECLARE_OPERATION_1(qsr::operations::ShowDebuggerWnd);
    EQuaServerId dbgSource = EQuaServerId::UNDEF;

    static void setup(qd::operation::args::OpDesc& d) {
        d.m_name = "Activate debugger";
        d.addShortcut(amD::shortcut::EId::ShowDebuggerWnd);
    }
};

struct ShowUaeOptionsWnd : public amD::operation::OperationArgs {
    DECLARE_OPERATION_1(qsr::operations::ShowUaeOptionsWnd);

    static void setup(qd::operation::args::OpDesc& d) {
        d.m_name = "Options...";
        d.addShortcut(amD::shortcut::EId::ShowUaeOptionsWnd);
    }
};


};  //namespace qsr::operations
