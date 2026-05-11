#pragma once
#include "qd/stl/ref_ptr.h"
#include "qd/stl/fixed_vector.h"
#include "qd/stl/vector.h"
#include "qd/typeSystem/typeDeclare.h"
#include <qd/stl/span.h>
#include <cassert>


namespace qd {
class Shortcut;


class ShortcutsHnd : public qd::RefCounted
{
    TS_REFLECT_CLASS(qd::ShortcutsHnd, qd::RefCounted);

public:
    qtd::fixed_vector<const Shortcut*, 2, false> m_pShortcuts;

public:
    virtual ~ShortcutsHnd() {}

    qtd::span<const Shortcut* const> getShortcuts() const
    {
        const Shortcut* const* ptrBeg = m_pShortcuts.data();
        qtd::span<const Shortcut* const> sp(ptrBeg, m_pShortcuts.size());
        return sp;
    }

    void addShortcut(const Shortcut* sh)
    {
        assert(sh);
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
}; // class ShortcutsHnd

}; // namespace qd
