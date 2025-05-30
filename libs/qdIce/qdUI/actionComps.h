#pragma once
#include <EASTL/span.h>
#include <qdIce/qdUI/actionBase.h>
#include <qdIce/qdTypeSystem/typeInfoReflector.h>


namespace qd {
namespace action {
namespace comp {

class ShortcutComp : public qd::NodeComp {
    TS_REFLECT_CLASS(qd::action::comp::ShortcutComp, qd::NodeComp);

public:
    static constexpr uint32_t CLASSID = (uint32_t)EActionCompsClassId::Shortcuts;
    eastl::fixed_vector<const Shortcut*, 2> mShortcuts;

public:
    virtual ~ShortcutComp() {
    }

    eastl::span<const Shortcut* const> getShortcuts() const {
        const Shortcut* const* ptrBeg = mShortcuts.data();
        eastl::span<const Shortcut* const> sp(ptrBeg, mShortcuts.size());
        return sp;
    }

    void addShortcut(const Shortcut* sh) {
        EASTL_ASSERT(sh);
        if (!sh)
            return;
        mShortcuts.push_back(sh);
    }

    const Shortcut* getShortcut(int ind) const {
        if ((size_t)ind >= mShortcuts.size())
            return nullptr;
        return mShortcuts[ind];
    }

    int getNumShortcuts() const {
        return (int)mShortcuts.size();
    }

};  // class Shortcuts
};  // namespace comp
};  // namespace action
};  // namespace qd


