#include "dbgOperation.h"
#include <amDebugger/msg_list.h>
#include <amDebugger/ui/dbgGuiDesktop.h>
#include <amDebugger/ui_defs.h>
#include <qd/qui/shortcutMgr.h>


namespace amD {
namespace operation {


Debugger* Operation::getDbg() const
{
    return m_pDbgGui->getDbg();
}


qd::EFlow Operation::applyOperationMsgProc(qd::operation::msg::Base* p_msg)
{
    return TSuper::_applyMsgProcDefImp(p_msg);
}


void Operation::onOperationCreate(qd::UiOperationCreator* cp)
{
    TSuper::onOperationCreate(cp);

    auto ca = static_cast<amD::operation::AmDebuggerOperationCreator*>(cp);
    m_pDbgGui = ca->gui;
    m_pDbg = ca->dbg;
    assert(m_pDbgGui);
    m_name = "NO NAME";
}


void Operation::doOperationBase()
{
    qd::operation::msg::DoOperation ms;
    applyOperationMsgProc(&ms);
}



//////////////////////////////////////////////////////////////////////////


}; // namespace operation
}; // namespace amD
