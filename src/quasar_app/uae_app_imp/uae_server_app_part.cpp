#include "uae_server_app_part.h"
#include "qd/thread/thread.h"
#include "quaesar_app.h"
#include "uae_server_thread.h"


namespace qsr {


UaeServerAppPart::UaeServerAppPart() {
}


UaeServerAppPart::~UaeServerAppPart() {
}


void UaeServerAppPart::onPartCreate(qd::ApplicationPart::OnCreate_t& prm) {
    TSuper::onPartCreate(prm);

    m_pUaeThread = new UaeServerThread(this);
    m_pUaeThread->initialize();

    QuaesarServersMgr* pSvMgr = ((QuasarApp*)getApp())->m_pServersMgr;
    pSvMgr->registerVmServer(EQuaServerId::S_UAE, m_pUaeThread);
}


void UaeServerAppPart::destroyImp() {
    if (m_pUaeThread) {
        m_pUaeThread->destroy();
        SAFE_DELETE(m_pUaeThread);
    }
    return TSuper::destroyImp();
}


IVm::VM* UaeServerAppPart::getVm() const {
    return m_pUaeThread->getVm();
}


UaeServerThread* UaeServerAppPart::getUaeThread() const {
    return m_pUaeThread;
}


};  // namespace qsr
