#pragma once
#include "qd/app/appMessages.h"
#include "qd/base/flowEnum.h"
#include "qd/base/ref_ptr.h"


FORWARD_DECLARATION_2(qd, AppPartsManager);
FORWARD_DECLARATION_2(qd, ModuleManager);
FORWARD_DECLARATION_2S(qd, CreateApplicationParams);
FORWARD_DECLARATION_3S(qd, appMsg, BaseMsg);


namespace qd {

class Application
{
    ModuleManager* m_pModuleManager;
    AppPartsManager* m_pAppParts = nullptr;
    bool m_bQuitRequestPosted = false;
    bool m_bActive = true;


public:
    Application();
    inline static Application* g_pInstance = nullptr;
    static Application* get() { return g_pInstance; }

    virtual void onCreate(qd::CreateApplicationParams& in);

    virtual ~Application();


    AppPartsManager* getAppParts() const { return m_pAppParts; }

    virtual void destroyImp() {}
    void destroy();

    qd::EFlow requestAppToQuit();
    bool hasQuitRequest() const;

    virtual qd::EFlow onAppEventProcImp(qd::appMsg::BaseMsg& in_msg);

    void sendAppEventMsg(qd::appMsg::BaseMsg& in_msg);

    bool isAppActive() const { return m_bActive; }
    void setAppActive(bool Active) { m_bActive = Active; }
}; // Application
//////////////////////////////////////////////////////////////////////////



struct CreateApplicationParams {};


}; // namespace qd
