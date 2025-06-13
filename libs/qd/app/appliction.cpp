#include "qd/app/appliction.h"
#include "qd/app/appPartsMgr.h"
#include "qd/app/appMessages.h"


namespace qd {

Application::Application()
{
    g_pInstance = this;
    m_pAppParts = new AppPartsManager(&ModuleCreateParams(this));
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


}; // namespace qd
