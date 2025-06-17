#include "qimMenu.h"
#include "qd/typeSystem/typeRegistry.h"
#include "qd/ui/shortcutComp.h"
#include "qd/ui/shortcutMgr.h"
#include "qd/ui/uiOperationManager.h"
#include "imgui/imgui_internal.h"
#include "SDL_log.h"



namespace qim {


void UiMenuOperation::setup(const char* operation_class_name)
{
    m_pOperationType = qd::TypeRegistry::get()->findTypeByName(operation_class_name);
    if (!m_pOperationType)
    {
        assert2(0, "Operation class:'%s' type not declared", operation_class_name);
        return;
    }

    qd::UiOperationMgr* pOpMgr = qd::UiOperationMgr::get();
    assert(pOpMgr);
    if (pOpMgr)
    {
        m_pOperation = pOpMgr->findOperationByType(*m_pOperationType);
        assert(m_pOperation);
    }
}


void UiMenuOperation::onEnd(qim::Context* ctx)
{
    auto pMenu = ctx->getStackTreeTop()->cast_<qim::UiMenu>();
    assert(pMenu);
    if (pMenu && pMenu->isOpen())
    {
        if (!m_pOperation)
            return;

        auto pShortcuts = m_pOperation->getComp_<qd::ShortcutComp>();
        qd::string shortcutName;
        if (pShortcuts && pShortcuts->getNumShortcuts() > 0)
        {
            const qd::Shortcut* pSh = pShortcuts->getShortcut(0);
            shortcutName = pSh->toString();
        }

        bool bChecked = false;
        // action::msg::MenuItemStateGet menuState;
        // menuState.menuType = event;
        // m_pOperation->applyActionMsgProc(&menuState);

        bool bEnabled = m_pOperation->isActive();
        if (ImGui::MenuItem(m_pOperation->m_name.c_str(), shortcutName.c_str(), &bChecked, bEnabled))
        {
            m_pOperation->doOperation();
        }
    }
}


qim::ElementData* UiMenuBeh::createElementData(const qd::TypeInfo& type)
{
    if (type == qd::typeof_<qim::UiMenuItem>())
        return new UiMenuItem();
    return new UiMenu();
}


void UiMenu::onBegin(qim::Context* ctx)
{
    //
    m_isOpen = ImGui::BeginMenu(m_text.c_str());
}


void UiMenu::onEnd(qim::Context* ctx)
{
    if (m_isOpen)
        ImGui::EndMenu();
}


}; // namespace qim
