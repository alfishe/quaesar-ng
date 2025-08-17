#pragma once
#include "qd/qui/shortcutHnd.h"
#include "qd/qui/shortcutMgr.h"
#include "qd/qui/uiOperation.h"
#include "qd/typeSystem/typeRegistry.h"
#include <imgui/imgui.h>


namespace qIm {

// ImGui's Operation menu control by operation's ClassName
//
struct MenuItemOperation {
    qd::UiOperation* m_pOperation = nullptr;

public:
    MenuItemOperation(qd::IOperationEnvironment* env, const char* pOperationClass, bool bDoOperation = true)
    {
        ImGuiStorage* pImGuiStorage = ImGui::GetStateStorage();

        // find & store operation Type
        void** pCachedOpInstance = pImGuiStorage->GetVoidPtrRef(ImGui::GetID(pOperationClass));
        if (!*pCachedOpInstance)
        {
            // find operation TYPE via reflection by ClassName
            const qd::TypeInfo* pOperationType = qd::TypeRegistry::get()->findTypeByName(pOperationClass);
            if (!pOperationType)
            {
                assert2(0, "Operation class:'%s' type not declared", pOperationClass);
                return;
            }

            // find operation instance by operation Type
            qd::UiOperationMgr* pOpMgr = &qd::UiOperationMgr::get();
            {
                qd::UiOperation* pOperation = pOpMgr->findOperationByType(*pOperationType);
                assert(pOperation);
                *pCachedOpInstance = pOperation; // save instance in ImGui cache storage
            }
        }
        qd::UiOperation* pOperation = reinterpret_cast<qd::UiOperation*>(*pCachedOpInstance);
        if (!pOperation)
            return;

        // print shortcut abbr
        qd::ShortcutsHnd* pShortcuts = pOperation->getShortcuts();
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
                pOperation->doOperation(env);
        }
    }

    operator bool () const { return m_pOperation; }
    ~MenuItemOperation() = default;

}; // struct MenuItemOperation
//////////////////////////////////////////////////////////////////////////


inline void menuItemOperation(qd::IOperationEnvironment* env, const char* pOperationClass, bool bDoOperation = true)
{
    qIm::MenuItemOperation menu(env, pOperationClass, bDoOperation);
}


inline void menuItemOperationArgs(qd::IOperationEnvironment* pEnv, qd::operation::args::Base* p_arg,
    const char* gui_label)
{
    bool bChecked = false;
    bool bEnabled = true;

    qd::InlineString shortcutName;
    qd::UiOperationMgr* pOpMgr = &qd::UiOperationMgr::get();
    if (const qd::operation::args::OpDesc* pDesc = pOpMgr->findOpDesc(p_arg->getCid()))
    {
        pDesc->getShortcutGuiStr(shortcutName);
        if (!gui_label)
            gui_label = pDesc->m_name.c_str();
    }

    if (ImGui::MenuItem(gui_label, shortcutName.c_str(), &bChecked, bEnabled))
    {
        pEnv->applyOperationMsgProc(p_arg);
    }
}


template<class TOp>
inline void menuItemOperationArgs_(qd::IOperationEnvironment* pEnv, const char* gui_label = nullptr)
{
    TOp pArg;
    menuItemOperationArgs(pEnv, &pArg, gui_label);
}


}; // namespace qIm
