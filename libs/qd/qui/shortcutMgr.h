#pragma once
#include <EASTL/fixed_function.h>
#include <EASTL/fixed_set.h>
#include <EASTL/span.h>
#include <EASTL/string.h>
#include <imgui/imgui.h>
#include <qd/base/classInfoReg.h>
#include <qd/node/node.h>
#include <qd/typeSystem/typeDeclare.h>



namespace qd {

class Shortcut;
using ShortcutSetupFunc = void (*)(qd::Shortcut&);
class UiOperation;
class UiOperationMgr;
class IOperationEnvironment;


class Key
{
public:
    ImGuiKey mKey = ImGuiKey_None;

public:
    constexpr Key() = default;
    constexpr Key(ImGuiKey key)
        : mKey(key)
    {}

    constexpr ImGuiKey getKeyCode() const { return mKey; }
    eastl::string toString() const { return ImGui::GetKeyName(mKey); }
    bool operator< (const Key& rh) const { return mKey < rh.mKey; }
    bool operator== (ImGuiKey key) const { return mKey == key; }

}; // class Key
//////////////////////////////////////////////////////////////////////////


class Shortcut
{
public:
    int m_id = -1;
    typedef eastl::fixed_set<qd::Key, 4, false> Keys;
    Shortcut::Keys m_keys;
    bool m_bRepeat = false;

public:
    Shortcut() = default;
    ~Shortcut();
    int getId() const { return m_id; }
    const Shortcut::Keys& getKeys() const { return m_keys; }
    ImGuiKeyChord getChord() const;

    Shortcut& addKey(ImGuiKey key);
    Shortcut& setRepeat(bool val = true)
    {
        m_bRepeat = val;
        return *this;
    }

    qd::string toString() const;

}; // class Shortcut
//////////////////////////////////////////////////////////////////////////


class ShortcutsMgr : public qd::RefCounted
{
    TS_REFLECT_CLASS(qd::ShortcutsMgr, qd::RefCounted);
    eastl::vector_map<int /*shortcut::ETypeId*/, Shortcut*> m_shortcuts;

public:
    ShortcutsMgr() {}

    static ShortcutsMgr* get();

    void init(eastl::span<ShortcutSetupFunc> shortcuts_list);
    void done();
    void update(qd::IOperationEnvironment* env, UiOperationMgr* pOpMgr);

    const qd::Shortcut* getShortcut(uint32_t shortcut_id) const;
    template<typename T>
    const Shortcut* getShortcut(T shortcut_id) const
    {
        return getShortcut((uint32_t)shortcut_id);
    }

    qd::UiOperation* findOperationByShortcut(const Shortcut* pShortcut) const;

    bool isShortcutTriggered(const qd::Shortcut* shortcut) const;
    bool triggerShortcut(qd::IOperationEnvironment* env, uint32_t id);
    bool triggerShortcut(qd::IOperationEnvironment* env, const qd::Shortcut* shortcut)
    {
        if (!shortcut)
            return false;
        return triggerShortcut(env, shortcut->getId());
    }

    virtual ~ShortcutsMgr();

}; // class ShortcutsMgr
//////////////////////////////////////////////////////////////////////////


}; // namespace qd
