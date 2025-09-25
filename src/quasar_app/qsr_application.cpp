#include "qsr_application.h"
#include "SDL.h"
#include "amDebugger/dbgConnection.h"
#include "amDebugger/debuggerServer.h"
#include "amDebugger/debuggerWndApp.h"
#include "qd/app/appPartsMgr.h"
#include "qd/imGui/imGuiContextManager.h"
#include "qsr_main_wnd_client_app.h"
#include "uae_imp/uae_server_app_part.h"
#include "vamiga_imp/va_server_app_part.h"


namespace amD::uae {
extern void on_app_exit_debug();
extern void on_app_exit_drawing();
};  // namespace amD::uae


QuasarApp::QuasarApp() {
    QuasarApp::g_pInstance = this;
}


QuasarApp::~QuasarApp() {
    SAFE_DELETE(m_pServersMgr);
}


void QuasarApp::onConstruct(qd::CreateApplicationParams& in) {
    TSuper::onConstruct(in);

    qd::ModuleManager::get()->getModuleInstOrCreate_<qd::ImGuiContextManager>();

    m_pServersMgr = new QuaesarDebuggerServersMgr(this);

    qd::AppPartsManager* appParts = getAppParts();
    m_pUaeServerAppPart = appParts->createPart_<qsr::UaeServerAppPart>("UAE server thread");
    m_pVAmServerAppPart = appParts->createPart_<qsr::VAmServerAppPart>("VAmiga server thread");

    qsr::IVmServerThread* pVmIO;
    //pVmIO = m_pUaeServerAppPart->getUaeThread();
    pVmIO = m_pVAmServerAppPart->getVAmThread();

    m_pUaeClientAppPart = appParts->createPart_<qsr::UaeClientAppPart>("UAE client app");
    m_pUaeClientAppPart->setVmProvider(pVmIO);

    m_pDebuggerApp = appParts->createPart_<amD::DebuggerApp>("Quaesar Debugger");
    m_pDebuggerApp->init();
    m_pUaeClientAppPart->bringWndToFront();
}


void QuasarApp::initialize() {
    //m_pDebuggerApp->toggleWndVisible(amD::DebuggerMode_Live);
}


void QuasarApp::destroyImp() {
    amD::uae::on_app_exit_debug();
    amD::uae::on_app_exit_drawing();
}


void QuasarApp::onSdlEventProc(SDL_Event& event) {
    TSuper::onSdlEventProc(event);
}


amD::Debugger* QuasarApp::getDbg() const {
    return m_pDebuggerApp->getDbg();
}


void* QuasarApp::getInterface(const qd::TypeInfo& p_interface) {
    if (QuaesarDebuggerServersMgr::getStaticTypeInfo().isDerivedFrom(p_interface)) {
        return m_pServersMgr;
    }
    return TSuper::getInterface(p_interface);
}


QuaesarDebuggerServersMgr::QuaesarDebuggerServersMgr(QuasarApp* pApp) : m_pApp(pApp) {
}


QuaesarDebuggerServersMgr::~QuaesarDebuggerServersMgr() {
    m_pVmServicesList.clear();
}


uint32_t QuaesarDebuggerServersMgr::getNumConnections() {
    return static_cast<uint32_t>(m_pVmServicesList.size());
}


ref_ptr<amD::IVmServiceConnection> QuaesarDebuggerServersMgr::createVmConnectionByInd(uint32_t idx) {
    if (idx >= m_pVmServicesList.size())
        return nullptr;
    amD::IVmConnectionBuilder* pServer = m_pVmServicesList[idx].m_pConnBuilder;
    if (!pServer)
        return nullptr;
    return pServer->createConnection();
}


void QuaesarDebuggerServersMgr::registerVmServer(EQuaServerId id, amD::IVmConnectionBuilder* pBuilder) {
    if (!getVmConnBuilderById(id))
        m_pVmServicesList.push_back({id, pBuilder});
    else {
        assert(0 && "already registered");
    }
}


amD::IVmConnectionBuilder* QuaesarDebuggerServersMgr::getVmConnBuilderById(EQuaServerId id) const {
    for (const QuaesarDebuggerServersMgr::VmServiceItem& item : m_pVmServicesList)
        if (item.m_id == id)
            return item.m_pConnBuilder;
    return nullptr;
}
