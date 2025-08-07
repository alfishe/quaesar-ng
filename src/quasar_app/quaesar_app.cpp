#include "quaesar_app.h"
#include "SDL.h"
#include "amDebugger/debuggerApp.h"
#include "qd/app/appPartsMgr.h"
#include "qd/imGui/imGuiContextManager.h"
#include "uae_wnd_app_part.h"


namespace amD::uae {
extern void on_app_exit_debug();
extern void on_app_exit_drawing();
};  // namespace amD::uae


void QuasarApp::onConstruct(qd::CreateApplicationParams& in) {
    TSuper::onConstruct(in);

    qd::ModuleManager::get()->getModuleInstOrCreate_<qd::ImGuiContextManager>();

    m_pDebuggerPart = getAppParts()->createPart_<amD::DebuggerApp>("Amiga Debugger");
    m_pUaeAppPart = getAppParts()->createPart_<UaeWndAppPart>("UAE Amiga emulator");

    m_pDebuggerPart->init();
    m_pUaeAppPart->bringWndToFront();
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
