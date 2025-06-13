#include "quaesar_app.h"
#include "SDL.h"
#include "amDebugger/debugger.h"
#include "qd/app/appPartsMgr.h"
#include "uae_app_part.h"


//////////////////////////////////////////////////////////////////////////
namespace qd {

qd::ThreadEvent* onUaeInitialized = nullptr;


void QuasarApp::onCreate(qd::CreateApplicationParams& in) {
    TSuper::onCreate(in);

    m_pDebugger = getAppParts()->createPart_<qd::Debugger>("Debugger");
    m_pUaeAppPart = getAppParts()->createPart_<UaeAppPart>("Emulator");
}


void QuasarApp::initialize() {
    m_pUaeAppPart->createUaeWindow();

    m_pDebugger->init();
    m_pDebugger->toggleWndVisible(qd::DebuggerMode_Live);
}


void QuasarApp::doMainLoop() {
    SDL_Event event;
    while (true) {
        if (hasQuitRequest()) {
            break;
        }

        while (SDL_PollEvent(&event) != 0) {
            switch (event.type) {
                case SDL_QUIT: {
                    requestAppToQuit();
                    break;
                }
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
            m_pDebugger->sdlEventProc(&event);
        }

        if (m_pDebugger->isVisible()) {
            m_pDebugger->update();
            m_pDebugger->render();
        }

        m_pUaeAppPart->renderUaeWindow();
    }
}

namespace uae {
extern void on_app_exit_debug();
extern void on_app_exit_drawing();
};  // namespace uae


void QuasarApp::destroyImp() {
    qd::uae::on_app_exit_debug();
    qd::uae::on_app_exit_drawing();
}


};  // namespace qd
