#include "mainMenu.h"
#include "qdIce/qdTypeSystem/typeRegistry.h"
#include "qdIce/qdUI/uiOperationManager.h"
#include "qdIce/qdUI/shortcutComp.h"
#include "qdIce/qdUI/shortcutMgr.h"


namespace qd
{

	void qd::UiMenuOperation::setup(const char* operation_class_name)
	{
        m_pOperationType = qd::TypeRegistry::get()->findTypeByName(operation_class_name);
        if (!m_pOperationType)
        {
            assert2(0, "Operation class:'%s' type not declared", operation_class_name);
            return;
        }

        IUiOperationsProvider* pMgr = findParentCompI_<qd::IUiOperationsProvider>();
        assert(pMgr);
        if (pMgr)
        {
            m_pOperation = pMgr->findOperationByType(*m_pOperationType);
            assert(m_pOperation);
        }
	}


	void qd::UiMenuOperation::draw()
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
        //action::msg::MenuItemStateGet menuState;
        //menuState.menuType = event;
        //m_pOperation->applyActionMsgProc(&menuState);

        bool bEnabled = m_pOperation->isActive();
        if (ImGui::MenuItem(m_pOperation->m_name.c_str(), shortcutName.c_str(), &bChecked, bEnabled))
        {
            m_pOperation->doOperation();
        }

	}


}; // namespace qd
