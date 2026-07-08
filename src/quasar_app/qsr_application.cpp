#include "qsr_application.h"
#include <vector>
#include "SDL.h"
#include "amDebugger/dbgConnection.h"
#include "amDebugger/debugger.h"
#include "amDebugger/debuggerOps.h"
#include "amDebugger/debuggerServer.h"
#include "amDebugger/debuggerWndApp.h"
#include "qd/app/appPartsMgr.h"
#include "qd/imGui/imGuiContextManager.h"
#include "qd/qui/uiOperation.h"
#include "qsr_main_wnd_client_app.h"
#include "uae_imp/uae_server_app_part.h"
#include "uae_imp/uae_server_thread.h"

// UAE debugger state (defined in debug.cpp)
extern int debugger_active;


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

    // Forward debugger ops to the real emulator. When paused via UAE's
    // internal debugger, route step/continue as console commands directly.
    m_pDebuggerApp->setForwardOpToEmulatorCb([this](qd::operation::BaseOpArgs* args) {
        // Lazy: replace the dummy VM bridge with the real UAE VM.
        // The debugger starts with create_dummy_connection() which creates a
        // separate UaeVmImp instance with m_pUaeThread=nullptr — that instance
        // reads the same global UAE state (so PC/memory values are correct)
        // but CANNOT control the emulator (pause/step/continue are no-ops).
        // As soon as the UAE server thread is up, swap to the real instance.
        if (!m_bDebuggerVmConnected) {
            if (qsr::IVmClientPlayer* pVmPlayer = m_pVmPlayerWndAppPart->getVmProvider()) {
                UaeServerThread* pUae = dynamic_cast<UaeServerThread*>(pVmPlayer);
                if (pUae && pUae->getVm()) {
                    m_pDebuggerApp->getDbg()->setDbgServiceBridge(
                        amD::create_shared_connection(pUae->getVm()));
                    m_bDebuggerVmConnected = true;
                }
            }
        }

        // Mirror debug mode to the Debugger's UI-only VM for menu enable/disable
        // state. Debugger::setDebugMode() only updates local mirrored state and
        // never calls back into the real emulator - the actual pause/continue is
        // applied exactly once below, queued onto the emulator thread.
        amD::Debugger* pDbg = m_pDebuggerApp->getDbg();
        if (pDbg) {
            if (args->cast_<amD::operation::PauseEmulation>() ||
                args->cast_<amD::operation::DebugTraceStart>())
                pDbg->setDebugMode(IVm::EVmDebugMode::Break);
            else if (args->cast_<amD::operation::DebugTraceContinue>())
                pDbg->setDebugMode(IVm::EVmDebugMode::Live);
        }
        if (qsr::IVmClientPlayer* pVmPlayer = m_pVmPlayerWndAppPart->getVmProvider()) {
            UaeServerThread* pUae = dynamic_cast<UaeServerThread*>(pVmPlayer);
            // When UAE's debug_1() is blocking (debugger_active), the ops queue
            // is stuck. Route step/continue directly via execConsoleCmd().
            if (pUae && debugger_active) {
                if (args->cast_<amD::operation::DisasmTraceStepInto>())
                    pUae->execConsoleCmd("t");
                else if (args->cast_<amD::operation::DisasmTraceStepOut>())
                    pUae->execConsoleCmd("z");
                else if (args->cast_<amD::operation::CopperTraceStep>())
                    pUae->execConsoleCmd("ot");
                else if (args->cast_<amD::operation::DebugTraceContinue>())
                    pUae->execConsoleCmd("g");
                // Other ops while paused: clone and push (will be processed after resume)
                else if (qd::operation::BaseOpArgs* pCloned = args->clone())
                    pVmPlayer->pushOperationMsg(qtd::unique_ptr<qd::operation::BaseOpArgs>(pCloned));
            } else if (qd::operation::BaseOpArgs* pCloned = args->clone()) {
                pVmPlayer->pushOperationMsg(qtd::unique_ptr<qd::operation::BaseOpArgs>(pCloned));
            }
        }
    });

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


// Eagerly swap the debugger's dummy VM bridge to the real UAE VM.
// The debugger starts with create_dummy_connection() which creates a
// separate UaeVmImp with m_pUaeThread=nullptr. That instance reads the
// same process-global UAE state (so PC/memory are correct) but cannot
// control the emulator or access the mutex-protected framebuffer.
//
// We check every frame until the swap succeeds — this ensures the real
// VM is wired before ANY debugger window tries to render or lock the
// framebuffer, eliminating the black-screen issue in the Screen window.
//
// After the swap, the dummy instance is fully replaced: all debugger
// windows, operations, and framebuffer access go through the real VM.
void QuaesarApplication::onFrameUpdate(float dt, float time) {
    TSuper::onFrameUpdate(dt, time);

    if (!m_bDebuggerVmConnected) {
        if (qsr::IVmClientPlayer* pVmPlayer = m_pVmPlayerWndAppPart->getVmProvider()) {
            UaeServerThread* pUae = dynamic_cast<UaeServerThread*>(pVmPlayer);
            if (pUae && pUae->getVm()) {
                m_pDebuggerApp->getDbg()->setDbgServiceBridge(
                    amD::create_shared_connection(pUae->getVm()));
                m_bDebuggerVmConnected = true;
            }
        }
    }
}


void* QuaesarApplication::getInterface(const qd::TypeInfo& p_interface) {
    return TSuper::getInterface(p_interface);
}


};  // namespace qsr
