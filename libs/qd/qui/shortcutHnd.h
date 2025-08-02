#pragma once
#include "qd/base/ref_ptr.h"
#include "qd/stl/fixed_vector.h"
#include "qd/stl/vector.h"
#include "qd/typeSystem/typeDeclare.h"
#include <EASTL/span.h>


namespace qd {
class Shortcut;


class ShortcutHnd : public qd::RefCounted
{
    TS_REFLECT_CLASS(qd::ShortcutHnd, qd::RefCounted);

public:
    qd::fixed_vector<const Shortcut*, 2, false> m_pShortcuts;

public:
    virtual ~ShortcutHnd() {}

    eastl::span<const Shortcut* const> getShortcuts() const
    {
        const Shortcut* const* ptrBeg = m_pShortcuts.data();
        eastl::span<const Shortcut* const> sp(ptrBeg, m_pShortcuts.size());
        return sp;
    }

    void addShortcut(const Shortcut* sh)
    {
        EASTL_ASSERT(sh);
        if (!sh)
            return;
        m_pShortcuts.push_back(sh);
    }

    const Shortcut* getShortcut(int ind) const
    {
        if ((size_t)ind >= m_pShortcuts.size())
            return nullptr;
        return m_pShortcuts[ind];
    }

    int getNumShortcuts() const { return (int)m_pShortcuts.size(); }
}; // class ShortcutHnd

}; // namespace qd
