#pragma once
#include "qd/qui/shortcutHnd.h"
#include "qd/qui/shortcutMgr.h"
#include "qd/qui/uiOperation.h"
#include "qd/typeSystem/typeRegistry.h"
#include <imgui/imgui.h>


namespace qIm {


inline void menuItemFromOperationArgs(qd::IOperationEnvironment* pEnv, qd::operation::BaseOpArgs* pOpArgs,
    const char* gui_label = nullptr, bool p_checked = false, bool p_enabled = true)
{
    qd::OperationsRegistry* pOpMgr = &qd::OperationsRegistry::get();
    const char* shortcutsStr = nullptr;
    if (const qd::operation::OpDesc* pDesc = pOpMgr->findOpDesc(pOpArgs->getCid()))
    {
        shortcutsStr = pDesc->getShortcutGuiStr();
        if (!gui_label || !gui_label[0])
            gui_label = pDesc->m_name.c_str();
    }
    if (ImGui::MenuItem(gui_label, shortcutsStr, p_checked, p_enabled))
    {
        pEnv->applyOperationMsgProc(pOpArgs);
    }
}


template<class TOpArg>
inline void menuItemFromOperationArgs_(qd::IOperationEnvironment* pEnv, const char* gui_label = nullptr,
    bool p_checked = false, bool p_enabled = true)
{
    TOpArg pArg;
    pEnv->setupDefaultOperationArgs(&pArg);
    menuItemFromOperationArgs(pEnv, &pArg, gui_label, p_checked, p_enabled);
}


}; // namespace qIm
