#pragma once
#include "qd/stl/span.h"
#include "qd/qui/uiNode.h"
#include "qd/qui/uiOperationMgr.h"
#include "qd/qui/shortcutMgr.h"
#include "qd/typeSystem/typeDeclare.h"


namespace qd {


class UiShortcutMgrComp
    : public qd::UiNodeComp
{
    TS_REFLECT_CLASS(qd::UiShortcutMgrComp, qd::UiNodeComp);
public:
    ref_ptr<qd::ShortcutsMgr> m_pOpMgr;

public:
}; // class UiOperationMgrComp
//////////////////////////////////////////////////////////////////////////



}; // namespace qd
