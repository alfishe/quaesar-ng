#include "qimMenu.h"
#include "qd/typeSystem/typeRegistry.h"
#include "qd/qui/shortcutHnd.h"
#include "qd/qui/shortcutMgr.h"
#include "qd/qui/operationsRegistry.h"
#include "imgui/imgui_internal.h"
#include "SDL_log.h"
#include "qd/qimGui/qimMessages.h"
#include "qd/qimGui/qimContext.h"



namespace qim {

#if 0

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


void UiMenuOperation::onDrawEndImp(qim::Context* ctx)
{
    auto pMenu = ctx->getStackTreeTopElem()->cast_<qim::UiMenu>();
    assert(pMenu);
    if (pMenu && pMenu->isOpen())
    {
        if (!m_pOperation)
            return;

        qd::ShortcutHnd* pShortcuts = m_pOperation->getShortcuts();
        qd::string shortcutName;
        if (pShortcuts && pShortcuts->getNumShortcuts() > 0)
        {
            const qd::Shortcut* pSh = pShortcuts->getShortcut(0);
            shortcutName = pSh->toString();
        }

        bool bChecked = false;
        bool bEnabled = m_pOperation->isActive();
        if (ImGui::MenuItem(m_pOperation->m_name.c_str(), shortcutName.c_str(), &bChecked, bEnabled))
        {
            m_pOperation->doOperation();
        }
    }
}


qim::Behavior* UiMenuBeh::createElementData(const qd::TypeInfo& type)
{
    if (type == qd::typeof_<qim::UiMenuItem>())
        return new UiMenuItem();
    return new UiMenu();
}


void UiMenu::onDrawBeginImp(qim::Context* ctx)
{
    ctx->pushStackElement(this);
    //
    m_isOpen = ImGui::BeginMenu(m_text.c_str());
}


void UiMenu::onDrawEndImp(qim::Context* ctx)
{
    ctx->popStackElement(this);

    if (m_isOpen)
        ImGui::EndMenu();
}


void UiMenuItem::onBeforeEndImp(qim::Context* ctx)
{
#if 0
    auto pMenu = ctx->getStackTreeTopElem(-1)->cast_<qim::UiMenu>();
    assert(pMenu);
    if (!pMenu || !pMenu->isOpen())
        return;
    assert(!m_text.empty());
    bool bClick = ImGui::MenuItem(m_text.c_str());

    if (bClick)
    {
        qim::msg::OnElemClicked t;
        t.m_pElem = this;
        notifyCompsOrParents(nullptr, t);
    }
#endif // 0
}

#endif // 

}; // namespace qim
