#include "uiOperation.h"
#include "qd/qui/shortcutHnd.h"
#include "qd/qui/shortcutMgr.h"
#include "qd/typeSystem/typeInfo.h"


namespace qd {


qd::EFlow IOperationEnvironment::applyOperationMsgProc(qd::operation::args::Base* args)
{
    qd::EFlow f;
    f = applyOperationMsgProcImp(args);
    if (f.isDone())
        return f;

    IOperationEnvironment* pParentEnd = getOpEnvParent();
    while (pParentEnd)
    {
        f = pParentEnd->applyOperationMsgProcImp(args);
        if (f.isDone())
            return f;
        pParentEnd = pParentEnd->getOpEnvParent();
    }
    return qd::EFlow::NO_RESULT;
}


qd::EFlow IOperationEnvironment::setupDefaultOperationArgs(qd::operation::args::Base* args) const
{
    qd::EFlow f;
    f = setupDefaultOperationArgsImp(args);
    if (f.isDone())
        return f;

    const IOperationEnvironment* pParentEnv = getOpEnvParent();
    while (pParentEnv)
    {
        f = pParentEnv->setupDefaultOperationArgsImp(args);
        if (f.isDone())
            return f;
        pParentEnv = pParentEnv->getOpEnvParent();
    }
    return qd::EFlow::NO_RESULT;
}



//////////////////////////////////////////////////////////////////////////
namespace operation::args {

void OpDesc::addShortcut(uint32_t sid)
{
    auto pShMgr = qd::ShortcutsMgr::get();
    ASSERT_AND_DO(pShMgr, return, "");
    const qd::Shortcut& shortcut = pShMgr->getShortcut(sid);

    if (!m_pShortcuts)
        m_pShortcuts = new qd::ShortcutsHnd();
    m_pShortcuts->addShortcut(&shortcut);
}




}; // namespace operation::args
//////////////////////////////////////////////////////////////////////////

}; // namespace qd
