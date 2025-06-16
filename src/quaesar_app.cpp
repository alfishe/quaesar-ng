#include "quaesar_app.h"
#include "SDL.h"
#include "amDebugger/debugger.h"
#include "qd/app/appPartsMgr.h"
#include "qd/imGui/imGuiContextManager.h"
#include "uae_app_part.h"


//////////////////////////////////////////////////////////////////////////
namespace amD {

qd::ThreadEvent* onUaeInitialized = nullptr;


void QuasarApp::onCreate(qd::CreateApplicationParams& in) {
    TSuper::onCreate(in);

    qd::ModuleManager::get()->getModuleInstOrCreate_<qd::ImGuiContextManager>();

    m_pDebugger = getAppParts()->createPart_<amD::Debugger>("Debugger");
    m_pUaeAppPart = getAppParts()->createPart_<UaeAppPart>("Emulator");
}


void QuasarApp::initialize() {
    m_pDebugger->init();
    m_pDebugger->toggleWndVisible(amD::DebuggerMode_Live);
}


namespace uae {
extern void on_app_exit_debug();
extern void on_app_exit_drawing();
};  // namespace uae


void QuasarApp::destroyImp() {
    amD::uae::on_app_exit_debug();
    amD::uae::on_app_exit_drawing();
}


void QuasarApp::onSdlEventProc(SDL_Event& event) {
    TSuper::onSdlEventProc(event);

    switch (event.type) {
        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_ESCAPE) {
                // requestAppToQuit();
                break;
            } else if (event.key.keysym.sym == SDLK_F12) {
                // activate_debugger();
                // qd::Debugger_toggle(m_pDebugger, qd::DebuggerMode_Live);
            }
            break;
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


};  // namespace amD
