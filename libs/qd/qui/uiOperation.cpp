#include "uiOperation.h"
#include "qd/qui/shortcutMgr.h"
#include "qd/qui/shortcutHnd.h"


namespace qd {

void UiOperation::addShortcut(int sid) {
    auto pActMgr = qd::ShortcutsMgr::get();
    ASSERT_AND_DO(pActMgr, return, "");
    const Shortcut* pShortcut = pActMgr->getShortcut(sid);
    assert(pShortcut);

    if (!m_pShortcuts)
        m_pShortcuts = new qd::ShortcutHnd();
    m_pShortcuts->addShortcut(pShortcut);
}


qd::EFlow UiOperation::_applyMsgProcDefImp(operation::msg::Base* pBaseMtd) {
/*
    for (Node* curComp : m_pComps) {
        EFlow flow = curComp->onNodeMessageProc(pBaseMtd);
        if (flow != EFlow::NO_RESULT)
            return flow;
    }
*/
    return EFlow::NO_RESULT;
}



};  // namespace qd
