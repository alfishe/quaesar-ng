#include "uiOperation.h"
#include "qd/qui/shortcutHnd.h"
#include "qd/qui/shortcutMgr.h"
#include "qd/typeSystem/typeInfo.h"


namespace qd {

void UiOperation::addShortcut(int sid)
{
    auto pActMgr = qd::ShortcutsMgr::get();
    ASSERT_AND_DO(pActMgr, return, "");
    const Shortcut* pShortcut = pActMgr->getShortcut(sid);
    assert(pShortcut);

    if (!m_pShortcuts)
        m_pShortcuts = new qd::ShortcutHnd();
    m_pShortcuts->addShortcut(pShortcut);
}

namespace operation::args {

void OpDesc::addShortcut(int sid)
{
    auto pActMgr = qd::ShortcutsMgr::get();
    ASSERT_AND_DO(pActMgr, return, "");
    const Shortcut* pShortcut = pActMgr->getShortcut(sid);
    assert(pShortcut);

    if (!m_pShortcuts)
        m_pShortcuts = new qd::ShortcutHnd();
    m_pShortcuts->addShortcut(pShortcut);
}


bool Base::tryCast(const qd::TypeInfo& msg_type)
{
    const qd::TypeInfo& typeInfo = getTypeInfo();
    return typeInfo.isDerivedFrom(msg_type);
}


bool OperationSupportedMsgVisitor::tryCast(const qd::TypeInfo& msg_type)
{
    msg_type.checkDefined();
    m_pSupportedMtd.push_back(&msg_type);

    return false;
}


}; // namespace operation::args
}; // namespace qd
