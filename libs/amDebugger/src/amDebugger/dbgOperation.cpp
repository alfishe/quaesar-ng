#include "dbgOperation.h"
#include <amDebugger/msg_list.h>
#include <amDebugger/ui/dbgGuiDesktop.h>
#include <amDebugger/ui_defs.h>
#include <qd/ui/shortcutMgr.h>


namespace amD {
namespace operation {


Debugger* Operation::getDbg() const
{
    return gui->getDbg();
}


qd::EFlow Operation::applyOperationMsgProc(qd::operation::msg::Base* p_msg)
{
    return TSuper::_applyMsgProcDefImp(p_msg);
}


void Operation::onNodeCreated(qd::NodeCreator* cp)
{
    TSuper::onNodeCreated(cp);
    gui = cp->parent->findParentNode_<DbgGuiDesktop>();
    assert(gui);
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
