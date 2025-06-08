#pragma once
#include "qdIce/qdTypeSystem/typeDeclare.h"
#include "uiNode.h"


namespace qd::uiMsg {

struct OnChildAdded : qd::UiNodeMessage {
    TS_REFLECT_CLASS(qd::uiMsg::OnChildAdded, qd::UiNodeMessage);
    qd::UiNode* m_pCtrl;
};



}; // namespace qd::uiMsg
