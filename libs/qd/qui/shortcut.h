#pragma once
#include "qd/base/base.h"
#include "qd/stl/string.h"
#include <EASTL/fixed_set.h>

typedef int ImGuiKeyChord;
enum ImGuiKey : int;


namespace qd {
class Shortcut;
using ShortcutId = uint32_t;
using ShortcutSetupFunc = void (*)(qd::Shortcut&);

struct ShortcutInitItem
{
    ShortcutId m_shortcutId;
    ShortcutSetupFunc m_setupFunc;
};

class Key
{
public:
    ImGuiKey m_keyId = (ImGuiKey)0; // ImGuiKey_None;

public:
    constexpr Key() = default;
    constexpr Key(ImGuiKey key)
        : m_keyId(key)
    {}

    constexpr ImGuiKey getKeyCode() const { return m_keyId; }
    qd::InlineString toString() const;
    bool operator< (const Key& rh) const { return m_keyId < rh.m_keyId; }
    bool operator== (ImGuiKey key) const { return m_keyId == key; }

}; // class Key
//////////////////////////////////////////////////////////////////////////



class Shortcut
{
public:
    ShortcutId m_id = 0;
    typedef eastl::fixed_set<qd::Key, 4, false> Keys;
    Shortcut::Keys m_keys;
    bool m_bRepeat = false;
    ImGuiKeyChord m_keyChord = 0;

public:
    Shortcut(ShortcutId id)
        : m_id(id)
    {}
    Shortcut(const Shortcut&) = delete;
    Shortcut& operator= (const Shortcut&) = delete;
    ~Shortcut();
    ShortcutId getId() const { return m_id; }
    const Shortcut::Keys& getKeys() const { return m_keys; }

    Shortcut& addKey(ImGuiKey key);
    Shortcut& setRepeat(bool val = true)
    {
        m_bRepeat = val;
        return *this;
    }

    bool empty() const { return m_keys.empty(); }

    const char* toString() const;

}; // class Shortcut
//////////////////////////////////////////////////////////////////////////



}; // namespace qd
