#include <qdIce/qdApp/appPart.h>
#include <qdIce/qdApp/appliction.h>
#include <qdIce/qdApp/appPartsMgr.h>


namespace qd {


AppPartsManager* qd::BaseAppPart::getAppParts() const {
    return m_pApp->getAppParts();
}

bool BaseAppPart::setPartActive(bool bActive) {
    if (isPartActive() == bActive)
        return bActive;
    EAppPartEvent::ON_ACTIVE_CHANGE p;
    p.m_bActive = bActive;
    onAppPartMsgProc(&p);
    m_Methods.set(EAppPartMtd::UPDATE, p.m_bActive);
    return p.m_bActive;
}

bool BaseAppPart::setPartVisisble(bool bPartVisisble) {
    if (isPartVisible() == bPartVisisble)
        return bPartVisisble;
    EAppPartEvent::ON_VISIBLE_CHANGE p;
    p.m_bVisible = bPartVisisble;
    onAppPartMsgProc(&p);
    m_Methods.set(EAppPartMtd::RENDER, p.m_bVisible);

    return p.m_bVisible;
}


};  // namespace qd
