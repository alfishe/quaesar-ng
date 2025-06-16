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
    int mId = -1;
    typedef eastl::fixed_set<qd::Key, 4, false> Keys;
    Keys mKeys;
    bool mRepeat = false;

public:
    Shortcut() = default;
    ~Shortcut();
    int getId() const { return mId; }
    const Shortcut::Keys& getKeys() const { return mKeys; }
    ImGuiKeyChord getChord() const;

    Shortcut& addKey(ImGuiKey key);
    Shortcut& setRepeat(bool val = true)
    {
        mRepeat = val;
        return *this;
    }

    eastl::string toString() const;

}; // class Shortcut
//////////////////////////////////////////////////////////////////////////


class ShortcutsMgr : public qd::NodeComp
{
    TS_REFLECT_CLASS(qd::ShortcutsMgr, qd::Node);
    eastl::vector_map<int /*shortcut::ETypeId*/, Shortcut*> mShortcuts;

public:
    ShortcutsMgr() {}

    void init(eastl::span<ShortcutSetupFunc> shortcuts_list);
    void done();
    void update();

    virtual void onNodeCreated(NodeCreator* mk) override
    {
        TSuper::onNodeCreated(mk);
    }

    const qd::Shortcut* getShortcut(uint32_t shortcut_id) const;
    template<typename T>
    const Shortcut* getShortcut(T shortcut_id) const
    {
        return getShortcut((uint32_t)shortcut_id);
    }

    qd::UiOperation* findOperationByShortcut(const Shortcut* pShortcut) const;

    bool isShortcutTriggered(const qd::Shortcut* shortcut) const;
    bool triggerShortcut(uint32_t id);
    bool triggerShortcut(const qd::Shortcut* shortcut)
    {
        if (!shortcut)
            return false;
        return triggerShortcut(shortcut->getId());
    }

    virtual ~ShortcutsMgr();

}; // class ShortcutsManager
//////////////////////////////////////////////////////////////////////////


}; // namespace qd
