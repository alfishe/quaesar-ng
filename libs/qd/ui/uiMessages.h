#pragma once
#include "qd/typeSystem/typeDeclare.h"
#include "uiNode.h"


namespace qd::uiMsg {

struct OnChildAdded : UI_MSG_BASE(OnChildAdded) {
    qd::UiNode* m_pCtrl;
};


struct OnVisibleChanged : UI_MSG_BASE(OnVisibleChanged) {
    qd::UiNode* m_pCtrl;
    bool m_bVisible = false; // New visibility state
};


}; // namespace qd::uiMsg
