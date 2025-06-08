#include "action_mgr.h"
#include <amDebugger/msg_list.h>
#include <amDebugger/ui/gui_manager.h>
#include <amDebugger/ui_defs.h>
#include <qdIce/qdUi/actionComps.h>
#include <qdIce/qdUI/shortcutMgr.h>


namespace qd {
namespace action {


Debugger* Action::getDbg() const
{
    return gui->getDbg();
}


qd::EFlow Action::applyActionMsgProc(action::msg::Base* p_msg)
{
    return TSuper::_applyMsgProcDefImp(p_msg);
}


void Action::onNodeCreated(NodeCreator* cp)
{
    TSuper::onNodeCreated(cp);
    gui = cp->parent->findParentNode_<GuiManager>();
    assert(gui);
    m_name = "NO NAME";
}


void Action::doActionBase()
{
    action::msg::DoAction ms;
    applyActionMsgProc(&ms);
}



//////////////////////////////////////////////////////////////////////////


}; // namespace action
}; // namespace qd
