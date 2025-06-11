#include "uiOperation.h"
#include "qdIce/qdUI/shortcutMgr.h"
#include "qdIce/qdUI/shortcutComp.h"


namespace qd {

void UiOperation::addShortcut(int sid) {
    auto pActMgr = findParentComp_<qd::ShortcutsMgr>();
    ASSERT_AND_DO(pActMgr, return, "");
    const Shortcut* pShortcut = pActMgr->getShortcut(sid);
    assert(pShortcut);
    auto pShortComp = createComp_<qd::ShortcutComp>();
    pShortComp->addShortcut(pShortcut);
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
