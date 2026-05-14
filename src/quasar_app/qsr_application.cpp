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


QuaesarApplication::QuaesarApplication() {
    QuaesarApplication::g_pInstance = this;
}


QuaesarApplication::~QuaesarApplication() {
    //SAFE_DELETE(m_pVmServersMgr);
}


void QuaesarApplication::onConstruct(qd::CreateApplicationParams& in) {
    TSuper::onConstruct(in);

    qd::ModuleManager::get()->getModuleInstOrCreate_<qd::ImGuiContextManager>();

    m_pVmPlayerWndAppPart = getAppParts()->createPart_<qsr::QsrMainClientWndApp>("VM client app");

    m_pDebuggerApp = getAppParts()->createPart_<amD::DebuggerApp>("Quaesar Debugger");
    m_pDebuggerApp->init();

    // Wire the debugger to the real UAE VM via the shared-memory connection bridge.
    // The main client window's init() has already called activateVmPlayerByIdStr()
    // which created the UaeServerThread and its UaeVmImp.
    qsr::VmPlayersSelector& vmSel = m_pVmPlayerWndAppPart->getVmSelector();
    int vmPlayerId = m_pVmPlayerWndAppPart->getCurVmPlayerId();
    if (auto pBridge = vmSel.createVmDebuggerConnection(vmPlayerId)) {
        m_pDebuggerApp->getDbg()->setDbgServiceBridge(pBridge);
    } else {
        SDL_Log("Quaesar: no VM debugger connection available — debugger uses dummy connection");
    }

    m_pVmPlayerWndAppPart->bringWndToFront();
}


void QuaesarApplication::initialize() {
}


void QuaesarApplication::destroyImp() {
    amD::uae::on_app_exit_debug();
    amD::uae::on_app_exit_drawing();
}


void QuaesarApplication::onSdlEventProc(SDL_Event& event) {
    TSuper::onSdlEventProc(event);
}


void* QuaesarApplication::getInterface(const qd::TypeInfo& p_interface) {
    return TSuper::getInterface(p_interface);
}


};  // namespace qsr
