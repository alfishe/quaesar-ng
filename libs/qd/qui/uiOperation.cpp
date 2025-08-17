#include "uiOperation.h"
#include "qd/qui/shortcutHnd.h"
#include "qd/qui/shortcutMgr.h"
#include "qd/typeSystem/typeInfo.h"


namespace qd {


 UiOperation::~UiOperation()
{
    SAFE_DELETE(m_pShortcuts);
}


void UiOperation::addShortcut(qd::ShortcutId sid)
{
    auto pActMgr = qd::ShortcutsMgr::get();
    ASSERT_AND_DO(pActMgr, return, "");
    const Shortcut& shortcut = pActMgr->getShortcut(sid);

    if (!m_pShortcuts)
        m_pShortcuts = new qd::ShortcutsHnd();
    m_pShortcuts->addShortcut(&shortcut);
}

namespace operation::args {

void OpDesc::addShortcut(uint32_t sid)
{
    auto pShMgr = qd::ShortcutsMgr::get();
    ASSERT_AND_DO(pShMgr, return, "");
    const Shortcut& shortcut = pShMgr->getShortcut(sid);

    if (!m_pShortcuts)
        m_pShortcuts = new qd::ShortcutsHnd();
    m_pShortcuts->addShortcut(&shortcut);
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
//////////////////////////////////////////////////////////////////////////


qd::EFlow IOperationEnvironment::applyOperationMsgProc(qd::operation::args::Base* args)
{
    IOperationEnvironment* pEnv = getOpEnvParent();
    while (pEnv)
    {
        qd::EFlow f = pEnv->applyOperationMsgProc(args);
        if (f.isDone())
            return f;
        pEnv = pEnv->getOpEnvParent();
    }
    return qd::EFlow::NO_RESULT;
}


}; // namespace qd
