#pragma once
#include <imgui/imgui.h>
#include "qd/qui/shortcutHnd.h"
#include "qd/qui/uiOperation.h"
#include "qd/typeSystem/typeRegistry.h"
#include "qd/qui/shortcutMgr.h"


namespace qIm {

struct MenuItemOperation {
    qd::UiOperation* m_pOperation = nullptr;

public:
    MenuItemOperation(const char* pOperationClass, bool bDoOperation = true)
    {
        ImGuiStorage* storage = ImGui::GetStateStorage();

        void** pOpStorage = storage->GetVoidPtrRef(ImGui::GetID(pOperationClass));
        if (!*pOpStorage)
        {
            const qd::TypeInfo* pOperationType = qd::TypeRegistry::get()->findTypeByName(pOperationClass);
            if (!pOperationType)
            {
                assert2(0, "Operation class:'%s' type not declared", pOperationClass);
                return;
            }

            qd::UiOperationMgr* pOpMgr = qd::UiOperationMgr::get();
            assert(pOpMgr);
            if (pOpMgr)
            {
                qd::UiOperation* pOperation = pOpMgr->findOperationByType(*pOperationType);
                assert(pOperation);
                *pOpStorage = pOperation;
            }
        }

        qd::UiOperation* pOperation = reinterpret_cast<qd::UiOperation*>(*pOpStorage);
        if (!pOperation)
            return;

        qd::ShortcutHnd* pShortcuts = pOperation->getShortcuts();
        qd::string shortcutName;
        if (pShortcuts && pShortcuts->getNumShortcuts() > 0)
        {
            const qd::Shortcut* pSh = pShortcuts->getShortcut(0);
            shortcutName = pSh->toString();
        }

        bool bChecked = false;
        bool bEnabled = pOperation->isActive();
        if (ImGui::MenuItem(pOperation->m_name.c_str(), shortcutName.c_str(), &bChecked, bEnabled))
        {
            m_pOperation = pOperation;
            if (bDoOperation)
                m_pOperation->doOperation();
        }
    }

    operator bool () const { return m_pOperation; }

    ~MenuItemOperation() {}

}; // struct MenuItemOperation
//////////////////////////////////////////////////////////////////////////


inline void menuItemOperation(const char* pOperationClass, bool bDoOperation = true)
{
    qIm::MenuItemOperation menu(pOperationClass, bDoOperation);
}

}; // namespace qIm
