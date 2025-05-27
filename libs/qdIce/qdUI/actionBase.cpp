#include "actionBase.h"
#include <qdIce/qdUI/shortcutMgr.h>
#include <qdIce/qdUI/actionComps.h>


namespace qd {

void UiAction::addShortcut(int sid) {
    auto pActMgr = findParentMixin_<ShortcutsMgr>();
    assert(pActMgr);
    const Shortcut* pShortcut = pActMgr->getShortcut(sid);
    assert(pShortcut);
    auto pShortComp = createComp_<qd::action::comp::ShortcutComp>();
    pShortComp->addShortcut(pShortcut);
}


qd::EFlow UiAction::_applyMsgProcDefImp(action::msg::Base* pBaseMtd) {
    for (Node* curComp : m_pComps) {
        EFlow flow = curComp->onNodeMessageProc(pBaseMtd);
        if (flow != EFlow::NO_RESULT)
            return flow;
    }
    return EFlow::NO_RESULT;
}


UiAction::~UiAction() {
}


};  // namespace qd
