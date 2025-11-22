#pragma once
#include "qd/app/appMessages.h"
#include "qd/base/eFlow.h"
#include "qd/stl/ref_ptr.h"
#include "qd/typeSystem/typeInfoBuilder.h"


FORWARD_DECLARATION_2(qd, AppPartsManager);
FORWARD_DECLARATION_2(qd, ModuleManager);
FORWARD_DECLARATION_2(qd, TypeInfo);
FORWARD_DECLARATION_2S(qd, CreateApplicationParams);
FORWARD_DECLARATION_3S(qd, appMsg, BaseMsg);

union SDL_Event;

namespace qd {

class Application
{
    ModuleManager* m_pModuleManager;
    AppPartsManager* m_pAppParts = nullptr;
    bool m_bQuitRequestPosted = false;
    bool m_bActive = true;


public:
    Application();
    virtual ~Application();
    inline static Application* g_pInstance = nullptr;
    static Application* get() { return g_pInstance; }

    virtual void onConstruct(qd::CreateApplicationParams& in);

    qd::AppPartsManager* getAppParts() const { return m_pAppParts; }

    virtual void destroyImp() {}
    void destroy();

    qd::EFlow requestAppToQuit();
    bool hasQuitRequest() const;

    template<class T>
    T* getInterface_()
    {
        return reinterpret_cast<T*>(getInterface(qd::typeof_<T>()));
    }
    virtual void* getInterface(const qd::TypeInfo& /*p_interface*/) { return nullptr; }
    virtual qd::EFlow onAppEventProcImp(qd::appMsg::BaseMsg& in_msg);

    void sendAppEventMsg(qd::appMsg::BaseMsg& in_msg);

    void doMainLoop();
    virtual void onFrameUpdate(float dt, float time);
    virtual void onFrameRender();

    virtual void onSdlEventProc(SDL_Event& event);

    bool isAppActive() const { return m_bActive; }
    void setAppActive(bool Active) { m_bActive = Active; }
}; // Application
//////////////////////////////////////////////////////////////////////////



struct CreateApplicationParams {};


}; // namespace qd
