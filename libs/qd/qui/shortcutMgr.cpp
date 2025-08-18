#include "shortcutMgr.h"
#include "qd/base/base.h"
#include "qd/qui/comps/uiOperationMgrComp.h"
#include "qd/qui/shortcutHnd.h"
#include "qd/qui/uiOperation.h"
#include "qd/qui/uiOperationMgr.h"
#include "qd/thread/thread.h"


namespace qd {


qd::Shortcut& Shortcut::addKey(ImGuiKey key)
{
    m_keys.insert(key);
    return *this;
}


qd::InlineString Shortcut::toString() const
{
    if (m_keys.empty())
        return {};
    qd::InlineString result;
    for (auto it = m_keys.begin(); it != m_keys.end(); ++it)
    {
        const qd::Key& curKey = *it;
        if (it != m_keys.begin())
            result += '+';
        result.append(curKey.toString());
    }
    return result;
}


Shortcut::~Shortcut() {}


ImGuiKeyChord Shortcut::getChord() const
{
    ImGuiKeyChord nChord = 0;
    const Shortcut::Keys& keysList = getKeys();
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
    if (!p_shortcut)
        return false;
    const Shortcut::Keys& keysList = p_shortcut->getKeys();
    if (keysList.empty())
        return false;
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
}


bool ShortcutsMgr::triggerShortcut(qd::IOperationEnvironment* env, uint32_t id)
{
    //     const Shortcut* pShortcut = getShortcut(id);
    //     if (UiOperation* pOperation = findOperationByShortcut(pShortcut)) {
    //         operation::args::DoOperation t;
    //         pOperation->applyOperationMsgProc(env, &t);
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


void ShortcutsMgr::createPredefinedShortcuts(eastl::span<ShortcutSetupFunc> shortcuts_list)
{
    for (int id = 0; id < (int)shortcuts_list.size(); ++id)
    {
        ShortcutSetupFunc setupFunc = shortcuts_list[id];
        if (setupFunc)
        {
            qd::Shortcut& curShortcut = getShortcut(id);
            setupFunc(curShortcut);
        }
    }
}


void ShortcutsMgr::done()
{
    while (!m_shortcuts.empty())
    {
        auto& p = m_shortcuts.back();
        delete p.second;
        m_shortcuts.pop_back();
    }
}


void ShortcutsMgr::update(qd::IOperationEnvironment* env, qd::UiOperationMgr* pOperationMgr)
{
    // IUiOperationsProvider* pOperationMgr = pOpMgr;
    assert(pOperationMgr);
    for (const qd::operation::args::OpDesc &pCurOperation : pOperationMgr->getOperationsList())
    {
        qd::ShortcutsHnd* pShortcuts = pCurOperation.m_pShortcuts;
        if (!pShortcuts)
            continue;
        for (const qd::Shortcut* curShortcut : pShortcuts->getShortcuts())
        {
            if (!isShortcutTriggered(curShortcut))
                continue;
            env->applyOperationMsgProc(pCurOperation.m_pOpTemplate);
        }
    }
}


qd::Shortcut& ShortcutsMgr::getShortcut(qd::ShortcutId shortcut_id)
{
    auto it = m_shortcuts.find((int)shortcut_id);
    if (it == m_shortcuts.end())
    {
        Shortcut* pShortcut = new Shortcut(shortcut_id);
        m_shortcuts[shortcut_id] = pShortcut;
        return *pShortcut;
    }
    return *it->second;
}


const qd::operation::args::OpDesc* ShortcutsMgr::findOperationByShortcut(const qd::Shortcut* pShortcut) const
{
    // IUiOperationsProvider* pOperationMgr = findParentCompI_<IUiOperationsProvider>();
    UiOperationMgr* pOperationMgr = &UiOperationMgr::get();
    assert(pOperationMgr);
    for (const qd::operation::args::OpDesc& pCurOperation : pOperationMgr->getOperationsList())
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
    return ImGui::GetKeyName(mKey);
}


}; // namespace qd
