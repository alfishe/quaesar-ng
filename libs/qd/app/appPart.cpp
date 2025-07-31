#include <qd/app/appliction.h>
#include <qd/app/appPart.h>
#include <qd/app/appPartsMgr.h>
#include "qd/app/appMessages.h"


namespace qd {


 AppPart::~AppPart(void)
 {
     c_def(this);
 }


void AppPart::onPartCreate(AppPart::OnCreate_t& prm)
{
    m_pApp = prm.app;

    qd::string name = prm.name;
    if (name.empty() && prm.typeInfo)
    {
    }
    setPartName(name);
}


AppPartsManager* AppPart::getAppParts() const
{
    return m_pApp->getAppParts();
}


void AppPart::setZOrder(const float& zOrder)
{
    if (m_ZOrder == zOrder)
        return;
    qd::appMsg::ON_Z_ORDER_CHANGE p;
    p.m_ZOrder = zOrder;
    onAppEventProcImp(p);
    m_ZOrder = zOrder;
}


bool AppPart::setPartActive(bool bActive)
{
    if (isPartActive() == bActive)
        return bActive;
    qd::appMsg::ON_ACTIVE_CHANGE p;
    p.m_bActive = bActive;
    onAppEventProcImp(p);
    m_Methods.set(EAppPartMtd::UPDATE, p.m_bActive);
    return p.m_bActive;
}


bool AppPart::setPartVisisble(bool bPartVisisble)
{
    if (isPartVisible() == bPartVisisble)
        return bPartVisisble;
    qd::appMsg::ON_VISIBLE_CHANGE p;
    p.m_bVisible = bPartVisisble;
    onAppEventProcImp(p);
    m_Methods.set(EAppPartMtd::RENDER, p.m_bVisible);

    return p.m_bVisible;
}


qd::EFlow AppPart::onAppEventProcImp(qd::appMsg::BaseMsg& in_msg)
{
    switch (in_msg.id)
    {
    case qd::appMsg::ON_VISIBLE_CHANGE::CID:
        break;
    default:
        break;
    }
    return EFlow::NO_RESULT;
}



void AppPart::destroy()
{
    destroyImp();
}


}; // namespace qd
