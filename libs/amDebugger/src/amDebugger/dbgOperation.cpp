#include "dbgOperation.h"
#include <amDebugger/msg_list.h>
#include <amDebugger/ui/gui_manager.h>
#include <amDebugger/ui_defs.h>
#include <qd/ui/shortcutMgr.h>


namespace qd {
namespace operation {


Debugger* Operation::getDbg() const
{
    return gui->getDbg();
}


qd::EFlow Operation::applyOperationMsgProc(operation::msg::Base* p_msg)
{
    return TSuper::_applyMsgProcDefImp(p_msg);
}


void Operation::onNodeCreated(NodeCreator* cp)
{
    TSuper::onNodeCreated(cp);
    gui = cp->parent->findParentNode_<GuiManager>();
    assert(gui);
    m_name = "NO NAME";
}


void Operation::doOperationBase()
{
    operation::msg::DoOperation ms;
    applyOperationMsgProc(&ms);
}



//////////////////////////////////////////////////////////////////////////


}; // namespace operation
}; // namespace qd
