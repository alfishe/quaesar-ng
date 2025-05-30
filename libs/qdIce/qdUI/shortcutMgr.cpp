#include "shortcutMgr.h"
#include <qdIce/qdUi/actionComps.h>
#include <qdIce/qdUi/actionMgr.h>
#include <qdIce/qdBase/base.h>
#include <qdIce/qdThread/thread.h>
#include <qdIce/qdUi/actionMsg.h>
#include <qdIce/qdUi/actionBase.h>


namespace qd {


qd::Shortcut& Shortcut::addKey(ImGuiKey key) {
    mKeys.insert(key);
    return *this;
}


eastl::string Shortcut::toString() const {
    eastl::string result;
    for (auto it = mKeys.begin(); it != mKeys.end(); ++it) {
        const qd::Key& curKey = *it;
        if (it != mKeys.begin())
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
        else if (ImGui::IsKeyPressed(curKey.getKeyCode(), p_shortcut->mRepeat))
            nKeysMatch++;
    }
    if (keysList.size() == nKeysMatch) {
        return true;
    }
    return false;
}


bool ShortcutsMgr::triggerShortcut(uint32_t id) {
    const Shortcut* pShortcut = getShortcut(id);
    if (UiAction* pAction = findActionByShortcut(pShortcut)) {
        action::msg::DoAction t;
        pAction->applyActionMsgProc(&t);
        return true;
    }
    return false;
}


ShortcutsMgr::~ShortcutsMgr() {
    done();
}


void ShortcutsMgr::init(eastl::span<ShortcutSetupFunc> shortcuts_list) {
    done();
    for (int id = 0; id < (int)shortcuts_list.size(); ++id) {

        ShortcutSetupFunc setupFunc = shortcuts_list[id];
        qd::Shortcut* curShortcut = new qd::Shortcut();
        curShortcut->mId = id;
        setupFunc(*curShortcut);

        EASTL_ASSERT((int)curShortcut->mId == id);
        if (getShortcut(id)) {
            ASSERT_F(0, "Shortcut ID:%i already registered", id);
            continue;
        }
        mShortcuts[id] = curShortcut;
    }
}


void ShortcutsMgr::done() {
    while (!mShortcuts.empty()) {
        auto& p = mShortcuts.back();
        delete p.second;
        mShortcuts.pop_back();
    }
}


void ShortcutsMgr::update() {
    auto pActionMgr = findParentComp_<UiActionMgr>();
    assert(pActionMgr);
    for (UiAction* pCurAction : pActionMgr->getActions()) {
        action::comp::ShortcutComp* pShortcuts = pCurAction->getComp_<action::comp::ShortcutComp>();
        if (!pShortcuts)
            continue;
        for (const Shortcut* curShortcut : pShortcuts->getShortcuts()) {
            if (isShortcutTriggered(curShortcut)) {
                action::msg::DoAction t;
                pCurAction->onNodeMessageProc(&t);
            }
        }
    }
}


const qd::Shortcut* ShortcutsMgr::getShortcut(uint32_t shortcut_id) const {
    auto it = mShortcuts.find((int)shortcut_id);
    if (it == mShortcuts.end())
        return nullptr;
    return it->second;
}


UiAction* ShortcutsMgr::findActionByShortcut(const qd::Shortcut* pShortcut) const {
    auto pActionMgr = findParentComp_<UiActionMgr>();
    assert(pActionMgr);
    for (UiAction* pCurAction : pActionMgr->getActions()) {
        action::comp::ShortcutComp* pShortcuts = pCurAction->getComp_<action::comp::ShortcutComp>();
        if (!pShortcuts)
            continue;
        for (const Shortcut* curShortcut : pShortcuts->getShortcuts()) {
            if (curShortcut == pShortcut) {
                return pCurAction;
            }
        }
    }
    return nullptr;
}


};  // namespace qd
