#include "shortcutMgr.h"
#include "qd/base/base.h"
#include "qd/qui/comps/uiOperationMgrComp.h"
#include "qd/qui/shortcutHnd.h"
#include "qd/qui/uiOperation.h"
#include "qd/qui/operationsRegistry.h"
#include "qd/thread/thread.h"
#include <imgui/imgui_internal.h>


namespace qd {


qd::Shortcut& Shortcut::addKey(ImGuiKey key)
{
    m_keys.insert(key);
    return *this;
}


const char* Shortcut::toString() const
{
    if (!m_keyChord)
        return "";
    return ImGui::GetKeyChordName(m_keyChord);
//     if (m_keys.empty())
//         return {};
//     qd::InlineString result;
//     for (auto it = m_keys.begin(); it != m_keys.end(); ++it)
//     {
//         const qd::Key& curKey = *it;
//         if (it != m_keys.begin())
//             result += '+';
//         result.append(curKey.toString());
//     }
//     return result;
}


Shortcut::~Shortcut() {}


ImGuiKeyChord shortcut_to_keychord(const Shortcut& sh)
{
    ImGuiKeyChord nChord = 0;
    const Shortcut::Keys& keysList = sh.getKeys();
    for (const Key& curKey : keysList)
    {
        if ((curKey == ImGuiMod_Shift || curKey == ImGuiKey_LeftShift || curKey == ImGuiKey_RightShift))
            nChord |= ImGuiMod_Shift;
        else if ((curKey == ImGuiMod_Ctrl || curKey == ImGuiKey_LeftCtrl || curKey == ImGuiKey_RightCtrl))
            nChord |= ImGuiMod_Ctrl;
        else
            nChord |= curKey.getKeyCode();
    }
    return nChord;
}


bool ShortcutsMgr::isShortcutTriggered(const qd::Shortcut* p_shortcut) const
{
    if (!p_shortcut || !p_shortcut->m_keyChord)
        return false;

    if (ImGui::IsKeyChordPressed(p_shortcut->m_keyChord, (ImGuiInputFlags)p_shortcut->m_bRepeat, 0))
        return true;
    return false;

#if 0
    const ImGuiIO& io = ImGui::GetIO();
    size_t nKeysMatch = 0;
    for (const Key& curKey : keysList)
    {
        if ((curKey == ImGuiMod_Shift || curKey == ImGuiKey_LeftShift || curKey == ImGuiKey_RightShift) && io.KeyShift)
            nKeysMatch++;
        else if ((curKey == ImGuiMod_Ctrl || curKey == ImGuiKey_LeftCtrl || curKey == ImGuiKey_RightCtrl) && io.KeyCtrl)
            nKeysMatch++;
        else if (ImGui::IsKeyPressed(curKey.getKeyCode(), p_shortcut->m_bRepeat))
            nKeysMatch++;
    }
    if (nKeysMatch >= keysList.size())
        return true;
    return false;
#endif //
}


bool ShortcutsMgr::triggerShortcut(qd::IOperationEnvironment* /*env*/, uint32_t /*id*/)
{
    //     const Shortcut* pShortcut = getShortcut(id);
    //     if (UiOperation* pOperation = findOperationByShortcut(pShortcut)) {
    //         operation::DoOperation t;
    //         pOperation->applyOperationMsgProcImp(env, &t);
    //         return true;
    //     }
    return false;
}


ShortcutsMgr::~ShortcutsMgr()
{
    done();
}


qd::ShortcutsMgr* ShortcutsMgr::get()
{
    static ref_ptr<ShortcutsMgr> pInst(new ShortcutsMgr());
    return pInst.get();
}


void ShortcutsMgr::createPredefinedShortcuts(eastl::span<qd::ShortcutInitItem> shortcuts_list)
{
    for (int i = 0; i < (int)shortcuts_list.size(); ++i)
    {
        const qd::ShortcutInitItem &curItem = shortcuts_list[i];
        ShortcutSetupFunc setupFunc = curItem.m_setupFunc;
        if (setupFunc)
        {
            qd::Shortcut& curShortcut = getShortcut(curItem.m_shortcutId);
            setupFunc(curShortcut); // init callback
            curShortcut.m_keyChord = shortcut_to_keychord(curShortcut);
        }
    }
}


void ShortcutsMgr::done()
{
    m_shortcuts.clear(); // SAFE DELETE
}



qd::Shortcut& ShortcutsMgr::getShortcut(qd::ShortcutId shortcut_id)
{
    auto it = m_shortcuts.find((int)shortcut_id);
    if (it == m_shortcuts.end())
    {
        // return reference for later binding
        qd::Shortcut* pShortcut = new qd::Shortcut(shortcut_id);
        m_shortcuts[shortcut_id] = qd::unique_ptr<qd::Shortcut>(pShortcut);
        return *pShortcut;
    }
    return *it->second;
}


const qd::operation::OpDesc* ShortcutsMgr::findOperationByShortcut(const qd::Shortcut* pShortcut) const
{
    // IUiOperationsProvider* pOperationMgr = findParentCompI_<IUiOperationsProvider>();
    OperationsRegistry* pOperationMgr = &OperationsRegistry::get();
    assert(pOperationMgr);
    for (const qd::operation::OpDesc& pCurOperation : pOperationMgr->getOperationsList())
    {
        ShortcutsHnd* pShortcuts = pCurOperation.m_pShortcuts;
        if (!pShortcuts)
            continue;
        for (const Shortcut* curShortcut : pShortcuts->getShortcuts())
        {
            if (curShortcut == pShortcut)
                return &pCurOperation;
        }
    }
    return nullptr;
}


qd::InlineString Key::toString() const
{
    return ImGui::GetKeyName(m_keyId);
}


}; // namespace qd
