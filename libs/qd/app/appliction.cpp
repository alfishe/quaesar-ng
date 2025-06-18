#include "qd/app/appliction.h"
#include "qd/app/appMessages.h"
#include "qd/app/appPartsMgr.h"
#include "SDL_events.h"


namespace qd {

Application::Application()
{
    g_pInstance = this;
    m_pModuleManager = qd::ModuleManager::get();
    m_pAppParts = m_pModuleManager->getModuleInstOrCreate_<qd::AppPartsManager>();
}


void Application::onCreate(qd::CreateApplicationParams& in) {}


Application::~Application()
{
    SAFE_DELETE(m_pAppParts);
}


void Application::destroy()
{
    destroyImp();
    m_pAppParts->destroy();
}



qd::EFlow Application::requestAppToQuit()
{
    if (hasQuitRequest())
        return EFlow::STOP;

    if (isAppActive())
    {
        qd::appMsg::OnAppRequestToQuit p;
        sendAppEventMsg(p);
        if (!p.allowToQuit)
            return EFlow::REPEAT;
    }

    m_bQuitRequestPosted = true;
    return EFlow::STOP;
}



bool Application::hasQuitRequest() const
{
    return m_bQuitRequestPosted;
}


qd::EFlow Application::onAppEventProcImp(qd::appMsg::BaseMsg& in_msg)
{
    return EFlow::NO_RESULT;
}


void Application::sendAppEventMsg(qd::appMsg::BaseMsg& in_msg)
{
    EFlow f = onAppEventProcImp(in_msg);
    if (f == EFlow::STOP)
        return;
    m_pAppParts->sendAppEventMsg(in_msg);
}


void Application::doMainLoop()
{
    SDL_Event event;
    while (true)
    {
        if (hasQuitRequest())
            break;

        while (SDL_PollEvent(&event) != 0)
        {
            onSdlEventProc(event);
        }

        getAppParts()->update(0, 0);
        getAppParts()->render();
    }
}


void Application::onSdlEventProc(SDL_Event& event)
{
    getAppParts()->onSdlEventProc(event);

    switch (event.type)
    {
    case SDL_QUIT:
    {
        requestAppToQuit();
        break;
    }
    case SDL_WINDOWEVENT:
    {
        Uint8 wndEvent = event.window.event;
        if (wndEvent == SDL_WINDOWEVENT_CLOSE)
        {
            requestAppToQuit();
            break;
        }
        break;
    }
    default:
        break;
    }
}


}; // namespace qd
