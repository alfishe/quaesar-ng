#pragma once
#include <EASTL/span.h>
#include <qdIce/qdUI/actionBase.h>
#include <qdIce/qdTypeSystem/typeDeclare.h>
#include <qdIce/qdSTL/vector.h>


namespace qd {
class Shortcut;

namespace action {
namespace comp {

class ShortcutComp : public qd::NodeComp {
    TS_REFLECT_CLASS(qd::action::comp::ShortcutComp, qd::NodeComp);

public:
    static constexpr uint32_t CLASSID = (uint32_t)EActionCompsClassId::Shortcuts;
    qd::vector<const Shortcut*> m_pShortcuts;

public:
    virtual ~ShortcutComp() {
    }

    eastl::span<const Shortcut* const> getShortcuts() const {
        const Shortcut* const* ptrBeg = m_pShortcuts.data();
        eastl::span<const Shortcut* const> sp(ptrBeg, m_pShortcuts.size());
        return sp;
    }

    void addShortcut(const Shortcut* sh) {
        EASTL_ASSERT(sh);
        if (!sh)
            return;
        m_pShortcuts.push_back(sh);
    }

    const Shortcut* getShortcut(int ind) const {
        if ((size_t)ind >= m_pShortcuts.size())
            return nullptr;
        return m_pShortcuts[ind];
    }

    int getNumShortcuts() const {
        return (int)m_pShortcuts.size();
    }

};  // class Shortcuts
};  // namespace comp
};  // namespace action
};  // namespace qd


