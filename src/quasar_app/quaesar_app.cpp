#include "quaesar_app.h"
#include "SDL.h"
#include "amDebugger/dbgConnection.h"
#include "amDebugger/debuggerApp.h"
#include "amDebugger/debuggerServer.h"
#include "qd/app/appPartsMgr.h"
#include "qd/imGui/imGuiContextManager.h"
#include "quasar_app/uae_app_imp/uae_client_app_part.h"
#include "uae_app_imp/uae_server_app_part.h"


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

    m_pServersMgr = new QuaesarServersMgr(this);

    m_pDebuggerPart = getAppParts()->createPart_<amD::DebuggerApp>("Amiga Debugger");

    m_pUaeServerAppPart = getAppParts()->createPart_<qsr::UaeServerAppPart>("UAE server");
    AbsVM::VM* vm = m_pUaeServerAppPart->getVm();
    assert(vm);
    m_pUaeClientAppPart = getAppParts()->createPart_<qsr::UaeClientAppPart>("UAE client", vm);

    m_pDebuggerPart->init();
    m_pUaeClientAppPart->bringWndToFront();
}


void QuasarApp::initialize() {
    //m_pDebuggerPart->toggleWndVisible(amD::DebuggerMode_Live);
}


void QuasarApp::destroyImp() {
    amD::uae::on_app_exit_debug();
    amD::uae::on_app_exit_drawing();
}


void QuasarApp::onSdlEventProc(SDL_Event& event) {
    TSuper::onSdlEventProc(event);

    switch (event.type) {
        case SDL_WINDOWEVENT: {
            Uint8 wndEvent = event.window.event;
            if (wndEvent == SDL_WINDOWEVENT_CLOSE) {
                requestAppToQuit();
                break;
            }
            break;
        }
        default:
            break;
    }
}


amD::Debugger* QuasarApp::getDbg() const {
    return m_pDebuggerPart->getDbg();
}


void* QuasarApp::getInterface(const qd::TypeInfo& p_interface) {
    if (QuaesarServersMgr::getStaticTypeInfo().isDerivedFrom(p_interface)) {
        return m_pServersMgr;
    }
    return TSuper::getInterface(p_interface);
}


QuaesarServersMgr::QuaesarServersMgr(QuasarApp* pApp) : m_pApp(pApp) {
}


QuaesarServersMgr::~QuaesarServersMgr() {
    m_pServers.clear();
}


uint32_t QuaesarServersMgr::getNumConnections() {
    return static_cast<uint32_t>(m_pServers.size());
}


amD::IDbgConnection* QuaesarServersMgr::getConnectionByNo(uint32_t idx) {
    if (idx >= m_pServers.size())
        return nullptr;
    amD::IDebuggerServer* pServer = m_pServers[idx].m_server;
    if (!pServer)
        return nullptr;
    return pServer->getConnection();
}


void QuaesarServersMgr::registerVmServer(EQuaServerId id, amD::IDebuggerServer* pServer) {
    if (!getServerById(id))
        m_pServers.push_back({id, pServer});
    else {
        assert(0 && "already registered");
    }
}


amD::IDebuggerServer* QuaesarServersMgr::getServerById(EQuaServerId id) const {
    for (const auto& item : m_pServers)
        if (item.m_id == id)
            return item.m_server;
    return nullptr;
}
