#include "qsr_application.h"
#include <vector>
#include "SDL.h"
#include "amDebugger/dbgConnection.h"
#include "amDebugger/debuggerServer.h"
#include "amDebugger/debuggerWndApp.h"
#include "qd/app/appPartsMgr.h"
#include "qd/imGui/imGuiContextManager.h"
#include "qsr_main_wnd_client_app.h"
#include "uae_imp/uae_server_app_part.h"


namespace amD::uae {
extern void on_app_exit_debug();
extern void on_app_exit_drawing();
};  // namespace amD::uae

qsr::QuaesarApplication* g_pApp = nullptr;


namespace qsr {

namespace plugin_api {

class AppPartServerFactoryListMgr {
    SINGLETON_DECLARATION(AppPartServerFactoryListMgr);

public:
    std::vector<std::unique_ptr<IAppPartServerProviderFactory>> m_appPartServerFactoryList;

public:
    AppPartServerFactoryListMgr() = default;
    ~AppPartServerFactoryListMgr() {
        m_appPartServerFactoryList.clear();
    }

    void addFactory(IAppPartServerProviderFactory* factory) {
        factory->setup();
        m_appPartServerFactoryList.push_back(std::unique_ptr<IAppPartServerProviderFactory>(factory));
    }

    IAppPartServerProviderFactory* findFactoryById(const char* id) {
        for (const auto& factory : m_appPartServerFactoryList) {
            if (factory->id == id)
                return factory.get();
        }
        return nullptr;
    }

};  // class AppPartServerFactoryListMgr


RegOnLoadAppPartServerFactory::RegOnLoadAppPartServerFactory(IAppPartServerProviderFactory* factory) {
    AppPartServerFactoryListMgr::get().addFactory(factory);
}
};  //namespace plugin_api
//////////////////////////////////////////////////////////////////////////


QuaesarApplication::QuaesarApplication() {
    QuaesarApplication::g_pInstance = this;
}


QuaesarApplication::~QuaesarApplication() {
    //SAFE_DELETE(m_pVmServersMgr);
}


void QuaesarApplication::onConstruct(qd::CreateApplicationParams& in) {
    TSuper::onConstruct(in);

    qd::ModuleManager::get()->getModuleInstOrCreate_<qd::ImGuiContextManager>();

    //m_pVmServersMgr = new QuaesarVmServersMgr(this);

    qd::AppPartsManager* pAppParts = getAppParts();

    const char* vmProviderId = nullptr;
    //vmProviderId = "uae";
    vmProviderId = "vamiga";

    IAppPartServerProviderFactory* pFactory =
        plugin_api::AppPartServerFactoryListMgr::get().findFactoryById(vmProviderId);
    assert(pFactory);
    ServerAppPartCreateCtx ctx;
    ctx.app = this;
    if (pFactory->createServerAppPart(ctx)) {
        qsr::BaseVmServerAppPart* pPart = ctx.outPartPtr;

        ProviderItem& item = m_vmServerAppParts.push_back();
        item.pServerApp = pPart;
        item.title = pFactory->guiName.c_str();
        item.id = pFactory->id.c_str();

        qd::ApplicationPart::OnCreate_t prm;
        prm.app = ctx.app;
        prm.name = pFactory->id.c_str();
        pPart->onPartCreate(prm);
        pAppParts->addPart(ctx.outPartPtr);
    }

    // TODO: Error initialization handling

    m_nCurServerFactory = 0;

    qsr::IVmServerThread* pVmIO;
    const QuaesarApplication::ProviderItem* provItem = &m_vmServerAppParts[m_nCurServerFactory];
    pVmIO = provItem->pServerApp->getServerThread();

    m_pUaeClientAppPart = pAppParts->createPart_<qsr::QsrMainClientWndApp>("UAE client app");
    m_pUaeClientAppPart->setVmProvider(pVmIO);

    m_pDebuggerApp = pAppParts->createPart_<amD::DebuggerApp>("Quaesar Debugger");
    m_pDebuggerApp->init();

    ref_ptr<amD::IVmDbgServiceBridge> pCurConnect = pFactory->createConnection();
    assert(pCurConnect);
    amD::Debugger* pDbg = getDbg();
    pDbg->setDbgServiceBridge(pCurConnect);

    m_pUaeClientAppPart->bringWndToFront();
}


void QuaesarApplication::initialize() {
    //m_pDebuggerApp->toggleWndVisible(amD::DebuggerMode_Live);
}


void QuaesarApplication::destroyImp() {
    amD::uae::on_app_exit_debug();
    amD::uae::on_app_exit_drawing();
}


void QuaesarApplication::onSdlEventProc(SDL_Event& event) {
    TSuper::onSdlEventProc(event);
}


amD::Debugger* QuaesarApplication::getDbg() const {
    return m_pDebuggerApp->getDbg();
}


void* QuaesarApplication::getInterface(const qd::TypeInfo& p_interface) {
    //     if (QuaesarVmServersMgr::getStaticTypeInfo().isDerivedFrom(p_interface)) {
    //         return m_pVmServersMgr;
    //     }
    return TSuper::getInterface(p_interface);
}


#if 0
QuaesarVmServersMgr::QuaesarVmServersMgr(QuaesarApplication* pApp) : m_pApp(pApp) {
}


QuaesarVmServersMgr::~QuaesarVmServersMgr() {
    m_pVmServicesList.clear();
}


uint32_t QuaesarVmServersMgr::getNumConnections() {
    return static_cast<uint32_t>(m_pVmServicesList.size());
}


ref_ptr<amD::IVmDbgServiceBridge> QuaesarVmServersMgr::createVmProvider(const char* id) {
    amD::IVmConnectionBuilder* pConnBuilder = findVmConnBuilderByStrId(id);
    if (!pConnBuilder)
        return nullptr;
    return pConnBuilder->createConnection();
}


void QuaesarVmServersMgr::registerVmServer(EQuaServerId id, amD::IVmConnectionBuilder* pBuilder) {
    if (!findVmConnBuilderByStrId(id.toString()))
        m_pVmServicesList.push_back({id, pBuilder});
    else {
        assert(0 && "already registered");
    }
}


amD::IVmConnectionBuilder* QuaesarVmServersMgr::findVmConnBuilderByStrId(const char* id) const {
    for (const QuaesarVmServersMgr::VmServiceItem& item : m_pVmServicesList) {
        const char* curName = item.m_id.toString();
        if (strstr(curName, id))
            return item.m_pConnBuilder;
    }
    return nullptr;
}


const char* EQuaServerId::toString() const {
    switch (mV) {
        case UNDEF:
            return "UNDEF";
        case S_UAE:
            return "UAE";
        case S_VAMIGA:
            return "VAmiga";
        default:
            return "UNKNOWN";
    }
}
#endif  //


};  // namespace qsr
