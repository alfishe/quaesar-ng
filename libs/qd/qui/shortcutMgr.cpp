#include "shortcutMgr.h"
#include "qd/qui/shortcutHnd.h"
#include "qd/qui/uiOperationMgr.h"
#include "qd/base/base.h"
#include "qd/thread/thread.h"
#include "qd/qui/uiOperationArgs.h"
#include "qd/qui/uiOperation.h"
#include "qd/qui/comps/uiOperationMgrComp.h"


namespace qd {


qd::Shortcut& Shortcut::addKey(ImGuiKey key) {
    m_keys.insert(key);
    return *this;
}


qd::string Shortcut::toString() const {
    qd::string result;
    for (auto it = m_keys.begin(); it != m_keys.end(); ++it) {
        const qd::Key& curKey = *it;
        if (it != m_keys.begin())
            result += '+';
        result += curKey.toString();
    }
    return result;
}


Shortcut::~Shortcut() {
}


ImGuiKeyChord Shortcut::getChord() const {
    ImGuiKeyChord nChord = 0;
    const Shortcut::Keys& keysList = getKeys();
    for (const Key& curKey : keysList) {
        if ((curKey == ImGuiMod_Shift || curKey == ImGuiKey_LeftShift || curKey == ImGuiKey_RightShift))
            nChord |= ImGuiMod_Shift;
        else if ((curKey == ImGuiMod_Ctrl || curKey == ImGuiKey_LeftCtrl || curKey == ImGuiKey_RightCtrl))
            nChord |= ImGuiMod_Ctrl;
        else
            nChord |= curKey.getKeyCode();
    }
    return nChord;
}


bool ShortcutsMgr::isShortcutTriggered(const qd::Shortcut* p_shortcut) const {
    if (!p_shortcut)
        return false;
    const Shortcut::Keys& keysList = p_shortcut->getKeys();
    if (keysList.empty())
        return false;
    const ImGuiIO& io = ImGui::GetIO();
    size_t nKeysMatch = 0;
    for (const Key& curKey : keysList) {
        if ((curKey == ImGuiMod_Shift || curKey == ImGuiKey_LeftShift || curKey == ImGuiKey_RightShift) && io.KeyShift)
            nKeysMatch++;
        else if ((curKey == ImGuiMod_Ctrl || curKey == ImGuiKey_LeftCtrl || curKey == ImGuiKey_RightCtrl) && io.KeyCtrl)
            nKeysMatch++;
        else if (ImGui::IsKeyPressed(curKey.getKeyCode(), p_shortcut->m_bRepeat))
            nKeysMatch++;
    }
    if (keysList.size() == nKeysMatch) {
        return true;
    }
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


ShortcutsMgr::~ShortcutsMgr() {
    done();
}


qd::ShortcutsMgr* ShortcutsMgr::get()
{
    static ref_ptr<ShortcutsMgr> pInst(new ShortcutsMgr());
    return pInst.get();
}


void ShortcutsMgr::init(eastl::span<ShortcutSetupFunc> shortcuts_list) {
    done();
    for (int id = 0; id < (int)shortcuts_list.size(); ++id) {

        ShortcutSetupFunc setupFunc = shortcuts_list[id];
        qd::Shortcut* curShortcut = new qd::Shortcut();
        curShortcut->m_id = id;
        setupFunc(*curShortcut);

        EASTL_ASSERT((int)curShortcut->m_id == id);
        if (getShortcut(id)) {
            ASSERT_F(0, "Shortcut ID:%i already registered", id);
            continue;
        }
        m_shortcuts[id] = curShortcut;
    }

#if 0
    for (int id = 0; id < (int)shortcut::EId::MAX_COUNT; ++id)
    {
        Shortcut* curShortcut = shortcut::makeInstance((shortcut::EId)id);
        if (!curShortcut)
            continue;
        EASTL_ASSERT((int)curShortcut->mId == id);
        if (getShortcut((shortcut::EId)id))
        {
            ASSERT_F(0, "Shortcut ID:%i already registered", id);
            continue;
        }
        mShortcuts[id] = curShortcut;
    }
#endif //

}


void ShortcutsMgr::done() {
    while (!m_shortcuts.empty()) {
        auto& p = m_shortcuts.back();
        delete p.second;
        m_shortcuts.pop_back();
    }
}


void ShortcutsMgr::update(qd::IOperationEnvironment* env, UiOperationMgr* pOperationMgr)
{
    //IUiOperationsProvider* pOperationMgr = pOpMgr;
    assert(pOperationMgr);
    for (UiOperation* pCurOperation : pOperationMgr->getOperationsList()) {
        ShortcutHnd* pShortcuts = pCurOperation->getShortcuts();
        if (!pShortcuts)
            continue;
        for (const Shortcut* curShortcut : pShortcuts->getShortcuts()) {
            if (isShortcutTriggered(curShortcut)) {
                //pCurOperation->doOperation(env); // TODO
            }
        }
    }
}


const qd::Shortcut* ShortcutsMgr::getShortcut(uint32_t shortcut_id) const {
    auto it = m_shortcuts.find((int)shortcut_id);
    if (it == m_shortcuts.end())
        return nullptr;
    return it->second;
}


UiOperation* ShortcutsMgr::findOperationByShortcut(const qd::Shortcut* pShortcut) const {
    //IUiOperationsProvider* pOperationMgr = findParentCompI_<IUiOperationsProvider>();
    UiOperationMgr* pOperationMgr = UiOperationMgr::get();
    assert(pOperationMgr);
    for (UiOperation* pCurOperation : pOperationMgr->getOperationsList()) {
        ShortcutHnd* pShortcuts = pCurOperation->getShortcuts();
        if (!pShortcuts)
            continue;
        for (const Shortcut* curShortcut : pShortcuts->getShortcuts()) {
            if (curShortcut == pShortcut) {
                return pCurOperation;
            }
        }
    }
    return nullptr;
}


};  // namespace qd
