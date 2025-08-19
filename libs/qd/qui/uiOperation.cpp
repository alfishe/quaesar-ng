#include "uiOperation.h"
#include "qd/qui/shortcutHnd.h"
#include "qd/qui/shortcutMgr.h"
#include "qd/typeSystem/typeInfo.h"


namespace qd {


 

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


//////////////////////////////////////////////////////////////////////////
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




}; // namespace operation::args
//////////////////////////////////////////////////////////////////////////

}; // namespace qd
