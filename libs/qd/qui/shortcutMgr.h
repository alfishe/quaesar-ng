#pragma once
#include <EASTL/fixed_function.h>
#include <EASTL/span.h>
#include <EASTL/string.h>
#include <EASTL/fixed_map.h>
#include "qd/qui/shortcut.h"
#include "qd/base/classInfoReg.h"
#include "qd/node/node.h"
#include "qd/typeSystem/typeDeclare.h"
#include "qd/stl/unique_ptr.h"


FORWARD_DECLARATION_4(qd, operation, args, OpDesc);


namespace qd {

class UiOperation;
class UiOperationMgr;
class IOperationEnvironment;




class ShortcutsMgr : public qd::RefCounted
{
    TS_REFLECT_CLASS(qd::ShortcutsMgr, qd::RefCounted);
    constexpr static size_t MAX_SHORTCUTS = 512;
    eastl::fixed_map<ShortcutId, qd::unique_ptr<qd::Shortcut>, MAX_SHORTCUTS, false> m_shortcuts;

public:
    ShortcutsMgr() {}

    static ShortcutsMgr* get();

    void createPredefinedShortcuts(eastl::span<qd::ShortcutInitItem> shortcuts_list);
    void done();
    void update(qd::IOperationEnvironment* env, qd::UiOperationMgr* pOpMgr);

    qd::Shortcut& getShortcut(qd::ShortcutId shortcut_id);

    template<typename T>
    qd::Shortcut& getShortcut(T shortcut_id)
    {
        return getShortcut((uint32_t)shortcut_id);
    }

    const qd::operation::args::OpDesc* findOperationByShortcut(const Shortcut* pShortcut) const;

    bool isShortcutTriggered(const qd::Shortcut* shortcut) const;
    bool triggerShortcut(qd::IOperationEnvironment* env, uint32_t id);
    bool triggerShortcut(qd::IOperationEnvironment* env, const qd::Shortcut* shortcut)
    {
        if (!shortcut)
            return false;
        return triggerShortcut(env, shortcut->getId());
    }

    virtual ~ShortcutsMgr() override;

}; // class ShortcutsMgr
//////////////////////////////////////////////////////////////////////////


}; // namespace qd
