#include "qd/app/application.h"
#include "qd/app/appMessages.h"
#include "qd/app/appPartsMgr.h"
#include "qd/typeSystem/typeInfoBuilder.h"
#if QD_USE_SDL
#include "SDL_events.h"
#include "SDL_timer.h"
#endif // QD_USE_SDL

namespace qd {

Application::Application() {
    g_pInstance = this;
    m_pModuleManager = qd::ModuleManager::get();
    m_pAppParts = m_pModuleManager->getModuleInstOrCreate_<qd::AppPartsManager>();
    assert(m_pAppParts);
}


void Application::onConstruct(qd::CreateApplicationParams& /*in*/) {}


Application::~Application() {
    // SAFE_DELETE(m_pAppParts);
    m_pModuleManager->destroyModule(qd::typeof_(*m_pAppParts));
}


void Application::destroy() {
    destroyImp();
    SAFE_DESTROY(m_pAppParts);
}



qd::EFlow Application::requestAppToQuit() {
    if (hasQuitRequest())
        return EFlow::STOP;

    if (isAppActive()) {
        qd::appMsg::OnAppRequestToQuit p;
        sendAppEventMsg(p);
        if (!p.allowToQuit)
            return EFlow::REPEAT;
    }

    m_bQuitRequestPosted = true;
    return EFlow::STOP;
}



bool Application::hasQuitRequest() const {
    return m_bQuitRequestPosted;
}



qd::EFlow Application::onAppEventProcImp(qd::appMsg::BaseMsg& /*in_msg*/) {
    return EFlow::NO_RESULT;
}


void Application::sendAppEventMsg(qd::appMsg::BaseMsg& in_msg) {
    EFlow f = onAppEventProcImp(in_msg);
    if (f == EFlow::STOP)
        return;
    if (m_pAppParts)
        m_pAppParts->sendAppEventMsg(in_msg);
}


void Application::onFrameUpdate(float /*dt*/, float /*time*/) {
    if (m_pAppParts)
        m_pAppParts->update(0, 0); // todo delta-time
}


void Application::onFrameRender() {
    if (m_pAppParts)
        m_pAppParts->render();
}


void Application::doMainLoop() {
#if QD_USE_SDL
    const Uint64 perfFreq = SDL_GetPerformanceFrequency();
    const Uint64 frameDuration = perfFreq / 60;  // target 60fps cap
    Uint64 nextFrameTime = SDL_GetPerformanceCounter();

    SDL_Event event;
    for (;;) {
        while (SDL_PollEvent(&event) != 0) {
            onSdlEventProc(event);
        }

        if (hasQuitRequest())
            break;

        onFrameUpdate(0, 0); // todo delta-time
        onFrameRender();

        // Frame pacing: sleep until next 60Hz boundary.
        // This caps CPU usage regardless of whether VSync actually engages.
        nextFrameTime += frameDuration;
        Uint64 now = SDL_GetPerformanceCounter();
        if (now < nextFrameTime) {
            Uint32 msWait = (Uint32)((nextFrameTime - now) * 1000ULL / perfFreq);
            if (msWait > 0)
                SDL_Delay(msWait);
        } else {
            // Fell behind — reset to avoid spiral of catch-up frames
            nextFrameTime = now;
        }
    }
#endif
}


void Application::onSdlEventProc(SDL_Event& event) {
    qd::EFlow f = m_pAppParts->onSdlEventProc(event);
    if (f.isStop())
        return;
#if QD_USE_SDL
    switch (event.type) {
    case SDL_QUIT:
    {
        requestAppToQuit();
        break;
    }
    case SDL_WINDOWEVENT:
    {
        if (event.window.event == SDL_WINDOWEVENT_CLOSE)
            requestAppToQuit();
        break;
    }
    default:
        break;
    }
#endif // QD_USE_SDL
}


}; // namespace qd
