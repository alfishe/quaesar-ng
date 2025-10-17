#include "va_server_app_part.h"
#include "va_server_thread.h"
#include "qd/thread/thread.h"
#include "quasar_app/vm_player_selector.h"

//------------------------------------------------------------------------
namespace qsr {


//------------------------------------------------------------------------
class VAmSharedConnectionImpl : public amD::IVmDbgServiceBridge {
public:
    ref_ptr<IVm::VM> m_pVAmVm;

    VAmSharedConnectionImpl(ref_ptr<IVm::VM> pVAmVm) : m_pVAmVm(pVAmVm) {}

    virtual ref_ptr<IVm::VM> getClientVm() override {
        return m_pVAmVm;
    }

    virtual ref_ptr<IVm::VM> getServerVm() override {
        return m_pVAmVm;
    }
};  // class VAmSharedConnectionImpl
//////////////////////////////////////////////////////////////////////////



class VAmigaServerProviderFactory : public qsr::IAppPartServerProviderFactory
{
    qsr::VAmServerAppPart *m_pVAmAppPart = nullptr;
public:
    virtual void setup() override {
        id = "vamiga";
        guiName = "vAmiga emulator";
    }

    virtual bool createServerAppPart(qsr::ServerAppPartCreateCtx &ctx) override {
        m_pVAmAppPart = new qsr::VAmServerAppPart();
        ctx.outPartPtr = m_pVAmAppPart;
        return true;
    }

    virtual ref_ptr<amD::IVmDbgServiceBridge> createVmDebuggerConnection() override {
        ref_ptr<IVm::VM> vm = m_pVAmAppPart->getVm();
        assert(vm);
        ref_ptr<qsr::VAmSharedConnectionImpl> pInst = new qsr::VAmSharedConnectionImpl(vm);
        return pInst;
    }

};
static qsr::plugin_api::RegOnLoadAppPartServerFactory reg_me(new VAmigaServerProviderFactory());
//////////////////////////////////////////////////////////////////////////


VAmServerAppPart::VAmServerAppPart() {
}


VAmServerAppPart::~VAmServerAppPart() {
}



/*
struct VAmConnImpl : public amD::IVmConnectionBuilder {
    VAmServerAppPart *m_pVAmAppPart;
    VAmConnImpl(VAmServerAppPart *pApp) : m_pVAmAppPart(pApp) {
    }
};
//////////////////////////////////////////////////////////////////////////
*/


void VAmServerAppPart::onPartCreate(qd::ApplicationPart::OnCreate_t &prm) {
    TSuper::onPartCreate(prm);

    m_pVAmThread = new VAmServerThread(this);
    m_pVAmThread->initialize();

//     m_pConnBuilder = new VAmConnImpl(this);
//     QuaesarVmServersMgr *pSvMgr = ((QuaesarApplication *)getApp())->m_pVmServersMgr;
//     pSvMgr->registerVmServer(EQuaServerId::S_VAMIGA, m_pConnBuilder);
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


qsr::IVmClientPlayer *VAmServerAppPart::getVmPlayer() {
    return m_pVAmThread;
}


};  // namespace qsr
