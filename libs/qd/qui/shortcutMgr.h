#pragma once
#include <qd/stl/fixed_function.h>
#include <qd/stl/span.h>
#include <qd/stl/string.h>
#include <qd/stl/vector_map.h>
#include "qd/qui/shortcut.h"
#include "qd/base/classInfoReg.h"
#include "qd/node/node.h"
#include "qd/typeSystem/typeDeclare.h"
#include "qd/stl/unique_ptr.h"


FORWARD_DECLARATION_3(qd, operation, OpDesc);


namespace qd {

class UiOperation;
class OperationsRegistry;
class IOperationEnvironment;




class ShortcutsMgr : public qd::RefCounted
{
    TS_REFLECT_CLASS(qd::ShortcutsMgr, qd::RefCounted);
    constexpr static size_t MAX_SHORTCUTS = 512;
    qtd::vector_map<ShortcutId, qtd::unique_ptr<qd::Shortcut>> m_shortcuts;

public:
    ShortcutsMgr() {}

    static ShortcutsMgr* get();

    void createPredefinedShortcuts(qtd::span<qd::ShortcutInitItem> shortcuts_list);
    void done();

    qd::Shortcut& getShortcut(qd::ShortcutId shortcut_id);

    template<typename T>
    qd::Shortcut& getShortcut(T shortcut_id)
    {
        return getShortcut((uint32_t)shortcut_id);
    }

    const qd::operation::OpDesc* findOperationByShortcut(const Shortcut* pShortcut) const;

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
