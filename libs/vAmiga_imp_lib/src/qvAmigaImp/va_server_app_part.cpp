#include "va_server_app_part.h"
#include "va_server_thread.h"
#include "qd/thread/thread.h"
#include "quasar_app/qsr_application.h"



class VAmigaServerProviderFactory : public qsr::IAppPartServerProviderFactory
{
    virtual bool createServerAppPart(qsr::ServerAppPartCreateCtx &ctx) override {
        ctx.outName = "VAmiga";
        ctx.outPartPtr = new qsr::VAmServerAppPart();
        ctx.outTypeInfo = &qd::typeof_<qsr::VAmServerAppPart>();
        return true;
    }
};
static qsr::plugin_api::RegOnLoadAppPartServerFactory reg_me(std::make_unique<VAmigaServerProviderFactory>());



namespace qsr {


VAmServerAppPart::VAmServerAppPart() {
}


VAmServerAppPart::~VAmServerAppPart() {
}

//------------------------------------------------------------------------
class VAmSharedConnectionImpl : public amD::IVmDbgServiceBridge {
public:
    ref_ptr<IVm::VM> m_pVAmVm;

    VAmSharedConnectionImpl(ref_ptr<IVm::VM> pVAmVm) : m_pVAmVm(pVAmVm) {
    }

    virtual ref_ptr<IVm::VM> getClientVm() override {
        return m_pVAmVm;
    }

    virtual ref_ptr<IVm::VM> getServerVm() override {
        return m_pVAmVm;
    }
};  // class VAmSharedConnectionImpl
//////////////////////////////////////////////////////////////////////////


struct VAmConnImpl : public amD::IVmConnectionBuilder {
    VAmServerAppPart *m_pVAmAppPart;
    VAmConnImpl(VAmServerAppPart *pApp) : m_pVAmAppPart(pApp) {
    }
    virtual ref_ptr<amD::IVmDbgServiceBridge> createConnection() const override {
        ref_ptr<IVm::VM> vm = m_pVAmAppPart->getVm();
        assert(vm);
        ref_ptr<VAmSharedConnectionImpl> pInst = new VAmSharedConnectionImpl(vm);
        return pInst;
    }
};
//////////////////////////////////////////////////////////////////////////


void VAmServerAppPart::onPartCreate(qd::ApplicationPart::OnCreate_t &prm) {
    TSuper::onPartCreate(prm);

    m_pVAmThread = new VAmServerThread(this);
    m_pVAmThread->initialize();

    m_pConnBuilder = new VAmConnImpl(this);
    QuaesarVmServersMgr *pSvMgr = ((QuaesarApplication *)getApp())->m_pVmServersMgr;
    pSvMgr->registerVmServer(EQuaServerId::S_VAMIGA, m_pConnBuilder);
}


void VAmServerAppPart::destroyImp() {
    if (m_pVAmThread) {
        m_pVAmThread->destroy();
        SAFE_DELETE(m_pVAmThread);
    }
    return TSuper::destroyImp();
}


IVm::VM *VAmServerAppPart::getVm() const {
    return m_pVAmThread->getVm();
}


qsr::IVmServerThread *VAmServerAppPart::getServerThread() {
    return m_pVAmThread;
}


};  // namespace qsr
