#include "dbgOperation.h"
#include <amDebugger/debuggerOps.h>
#include <amDebugger/ui/debuggerDesktop.h>
#include <amDebugger/ui/uiDefs.h>
#include <qd/qui/shortcutMgr.h>


namespace amD {
namespace operation {


void Operation::addShortcut(shortcut::EId sid)
{
    UiOperation::addShortcut(sid);
}


// Debugger* Operation::getDbg() const
// {
//     return m_pDbgGui->getDbg();
// }


void Operation::onOperationCreate(qd::UiOperationCreator* cp)
{
    TSuper::onOperationCreate(cp);

    auto ca = static_cast<amD::operation::AmDebuggerOperationCreator*>(cp);
//     m_pDbgGui = ca->gui;
//     m_pDbg = ca->dbg;//     assert(m_pDbgGui);    m_name = "NO NAME";
}


// void Operation::doOperationBase(qd::OperationEnvironment* env)
// {
//     qd::operation::args::DoOperation ms;
//     applyOperationMsgProc(env, &ms);
// }



//////////////////////////////////////////////////////////////////////////


}; // namespace operation
}; // namespace amD
