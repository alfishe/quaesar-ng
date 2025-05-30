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
    switch (p_msg->id)
    {
    case msg::OnDrawMainMenuItem::CID:
    {
        auto p = p_msg->cast_<msg::OnDrawMainMenuItem>();
        onDrawMainMenuItem(p->drawElement);
    }
    break;
    }
    return TSuper::_applyMsgProcDefImp(p_msg);
}


void Action::onNodeCreated(NodeCreator* cp)
{
    gui = cp->parent->findParentNode_<GuiManager>();
    assert(gui);
    supportMtd.none();
    mName = "NO NAME";
}

void Action::onDrawMainMenuItem(UiDrawEvent::Type event, void* /*= nullptr*/)
{
    Action* action = this;
    auto pShortcuts = action->getComp_<comp::ShortcutComp>();
    eastl::string shortcutName;
    if (pShortcuts && pShortcuts->getNumShortcuts() > 0)
    {
        const Shortcut* pSh = pShortcuts->getShortcut(0);
        shortcutName = pSh->toString();
    }

    bool bSelected = false;
    if (action->supportMtd[UiDrawEvent::MenuItemStateChecked])
    {
        action::msg::MenuItemStateGet menuState;
        menuState.menuType = event;
        action->applyActionMsgProc(&menuState);
        bSelected = menuState.checked > 0 ? menuState.checked != 0 : false;
    }

    bool enabled = true;
    if (ImGui::MenuItem(mName.c_str(), shortcutName.c_str(), &bSelected, enabled))
    {
        action::msg::DoAction msg;
        action->applyActionMsgProc(&msg);
    }
}

void Action::doActionBase()
{
    action::msg::DoAction ms;
    applyActionMsgProc(&ms);
}


//////////////////////////////////////////////////////////////////////////


}; // namespace action
}; // namespace qd
