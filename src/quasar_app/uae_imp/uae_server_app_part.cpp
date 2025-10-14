#include "uae_server_app_part.h"
#include "qd/log/log.h"
#include "qd/thread/thread.h"
#include "qsr_application.h"
#include "uae_server_thread.h"


namespace qsr {


UaeServerAppPart::UaeServerAppPart() {
}


UaeServerAppPart::~UaeServerAppPart() {
}


//------------------------------------------------------------------------
class UaeSharedConnectionImpl : public amD::IVmDbgServiceBridge {
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


/*
struct UaeConnImpl : public amD::IVmConnectionBuilder {
    UaeServerAppPart* m_pUaeAppPart;
    UaeConnImpl(UaeServerAppPart* pApp) : m_pUaeAppPart(pApp) {
        assert(pApp);
    }
    virtual ref_ptr<amD::IVmDbgServiceBridge> createConnection() const override {
        ref_ptr<IVm::VM> pVm = m_pUaeAppPart->getVm();
        assert(pVm);
        ref_ptr<UaeSharedConnectionImpl> pInst = new UaeSharedConnectionImpl(pVm);
        return pInst;
    }
};
*/


//------------------------------------------------------------------------
class UaeServerProviderFactory : public qsr::IAppPartServerProviderFactory {
    qsr::UaeServerAppPart* m_pUaeAppPart = nullptr;

public:
    virtual void setup() override {
        id = "uae";
        guiName = "UAEmu";
    }

    virtual bool createServerAppPart(qsr::ServerAppPartCreateCtx& ctx) override {
        m_pUaeAppPart = new qsr::UaeServerAppPart();
        ctx.outPartPtr = m_pUaeAppPart;
        return true;
    }
    virtual ref_ptr<amD::IVmDbgServiceBridge> createConnection() override {
        ref_ptr<IVm::VM> pVm = m_pUaeAppPart->getVm();
        assert(pVm);
        ref_ptr<UaeSharedConnectionImpl> pInst = new UaeSharedConnectionImpl(pVm);
        return pInst;
    }
};
static qsr::plugin_api::RegOnLoadAppPartServerFactory reg_me(new UaeServerProviderFactory());
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////

void UaeServerAppPart::onPartCreate(qd::ApplicationPart::OnCreate_t& prm) {
    TSuper::onPartCreate(prm);

    //     m_pConnBuilder = new UaeConnImpl(this);
    //     QuaesarVmServersMgr* pSvMgr = ((QuaesarApplication*)getApp())->m_pVmServersMgr;
    //     pSvMgr->registerVmServer(EQuaServerId::S_UAE, m_pConnBuilder);

    createUaeThread();
}


void UaeServerAppPart::createUaeThread() {
    if (!m_pUaeThread) {
        qd::logInfo("Creating UaeServerThread...");
        m_pUaeThread = new UaeServerThread(this);
        m_pUaeThread->initialize();
    }
}


void UaeServerAppPart::destroyImp() {
    if (m_pUaeThread) {
        m_pUaeThread->destroy();
        SAFE_DELETE(m_pUaeThread);
    }
    return TSuper::destroyImp();
}


IVm::VM* UaeServerAppPart::getVm() const {
    if (!m_pUaeThread)
        return nullptr;
    return m_pUaeThread->getVm();
}


qsr::IVmServerThread* UaeServerAppPart::getServerThread() {
    return m_pUaeThread;
}


void UaeServerAppPart::update(float dt, float time) {
    TSuper::update(dt, time);

    if (m_vmActive > 0) {
        createUaeThread();
    }
}


};  // namespace qsr
