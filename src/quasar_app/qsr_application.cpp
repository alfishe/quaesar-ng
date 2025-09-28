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


QuaesarApplication::QuaesarApplication() {
    QuaesarApplication::g_pInstance = this;
}


QuaesarApplication::~QuaesarApplication() {
    SAFE_DELETE(m_pVmServersMgr);
}


void QuaesarApplication::onConstruct(qd::CreateApplicationParams& in) {
    TSuper::onConstruct(in);

    qd::ModuleManager::get()->getModuleInstOrCreate_<qd::ImGuiContextManager>();

    m_pVmServersMgr = new QuaesarVmServersMgr(this);

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
    ref_ptr<amD::IVmServiceProvider> pCurConnect = m_pVmServersMgr->createVmProvider(to_string(EQuaServerId::S_VAMIGA));
    assert(pCurConnect);
    getDbg()->setConnection(pCurConnect);

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
    if (QuaesarVmServersMgr::getStaticTypeInfo().isDerivedFrom(p_interface)) {
        return m_pVmServersMgr;
    }
    return TSuper::getInterface(p_interface);
}


QuaesarVmServersMgr::QuaesarVmServersMgr(QuaesarApplication* pApp) : m_pApp(pApp) {
}


QuaesarVmServersMgr::~QuaesarVmServersMgr() {
    m_pVmServicesList.clear();
}


uint32_t QuaesarVmServersMgr::getNumConnections() {
    return static_cast<uint32_t>(m_pVmServicesList.size());
}


ref_ptr<amD::IVmServiceProvider> QuaesarVmServersMgr::createVmProvider(const char* id) {
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
