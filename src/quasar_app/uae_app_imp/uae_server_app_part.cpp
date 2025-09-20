#include "uae_server_app_part.h"
#include "qd/thread/thread.h"
#include "quaesar_app.h"
#include "uae_server_thread.h"
//#include "uae_vm_imp.h"


namespace qsr {


UaeServerAppPart::UaeServerAppPart() {
}


UaeServerAppPart::~UaeServerAppPart() {
}

//------------------------------------------------------------------------
class UaeSharedConnectionImpl : public amD::IVmServiceConnection {
public:
    ref_ptr<IVm::VM> m_pUaeVm;

    UaeSharedConnectionImpl(ref_ptr<IVm::VM> pUaeVm) : m_pUaeVm(pUaeVm) {
    }

    virtual ref_ptr<IVm::VM> getClientVm() override {
        return m_pUaeVm;
    }

    virtual ref_ptr<IVm::VM> getServerVm() override {
        return m_pUaeVm;
    }
};  // class UaeSharedConnectionImpl
//////////////////////////////////////////////////////////////////////////

struct UaeConnImpl : public amD::IVmConnectionBuilder {
    UaeServerAppPart* m_pUaeAppPart;
    UaeConnImpl(UaeServerAppPart* pApp) : m_pUaeAppPart(pApp) {
    }
    virtual ref_ptr<amD::IVmServiceConnection> createConnection() const override {
        ref_ptr<IVm::VM> vm = m_pUaeAppPart->getVm();
        assert(vm);
        ref_ptr<UaeSharedConnectionImpl> pInst = new UaeSharedConnectionImpl(vm);
        return pInst;
    }
};
//////////////////////////////////////////////////////////////////////////


void UaeServerAppPart::onPartCreate(qd::ApplicationPart::OnCreate_t& prm) {
    TSuper::onPartCreate(prm);

    m_pUaeThread = new UaeServerThread(this);
    m_pUaeThread->initialize();

    m_pConnBuilder = new UaeConnImpl(this);
    QuaesarDebuggerServersMgr* pSvMgr = ((QuasarApp*)getApp())->m_pServersMgr;
    pSvMgr->registerVmServer(EQuaServerId::S_UAE, m_pConnBuilder);
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
