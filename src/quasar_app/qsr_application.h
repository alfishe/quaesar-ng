#pragma once
#include <memory>  // unique_ptr
#include <string>
#include "amDebugger/debuggerWndApp.h"
#include "qd/app/application.h"
#include "qd/typeSystem/typeDeclare.h"


FORWARD_DECLARATION_1S(SDL_Window);
FORWARD_DECLARATION_1S(SDL_Texture);
FORWARD_DECLARATION_1S(SDL_Renderer);
FORWARD_DECLARATION_2(qsr, QsrMainClientWndApp);
FORWARD_DECLARATION_2(qsr, QuaesarApplication);
FORWARD_DECLARATION_2(amD, DebuggerApp);
FORWARD_DECLARATION_2(amD, Debugger);
FORWARD_DECLARATION_2(amD, IVmConnectionBuilder);
extern qsr::QuaesarApplication* g_pApp;


namespace qsr {
class BaseVmServerAppPart;
class IAppPartServerProviderFactory;
class QuaesarVmServersMgr;
class IVmClientPlayer;
struct ServerAppPartCreateCtx;


//////////////////////////////////////////////////////////////////////////
class QuaesarApplication : public qd::Application {
    typedef qd::Application TSuper;

public:
    amD::DebuggerApp* m_pDebuggerApp = nullptr;
    qsr::QsrMainClientWndApp* m_pVmPlayerWndAppPart = nullptr;
    bool m_bDebuggerVmConnected = false; // true once the real UAE VM replaces the dummy bridge

public:
    QuaesarApplication();
    virtual ~QuaesarApplication() override;
    virtual void onConstruct(qd::CreateApplicationParams& in) override;

    inline static QuaesarApplication* g_pInstance = nullptr;
    static QuaesarApplication* get() {
        return g_pInstance;
    }
    void initialize();
    virtual void destroyImp() override;

    virtual void onSdlEventProc(SDL_Event& event) override;

    virtual void* getInterface(const qd::TypeInfo& p_interface) override;

    amD::DebuggerApp* getDebuggerApp() const {
        return m_pDebuggerApp;
    }
    qsr::QsrMainClientWndApp* getUaeClientAppPart() const {
        return m_pVmPlayerWndAppPart;
    }

};  // class QuaesarApplication
//////////////////////////////////////////////////////////////////////////


/*
struct EQuaServerId {
    enum Type {
        UNDEF = 0,
        S_UAE = _MAKE4C("QUAE"),
        S_VAMIGA = _MAKE4C("VAMI"),
    };
    ENUM_DECLARE_BASE(qsr::, EQuaServerId, Type, UNDEF);
    const char* toString() const;
};

inline const char* to_string(EQuaServerId v) {
    return v.toString();
}
*/


class BaseVmServerAppPart : public qd::ApplicationPart {
    TS_BEGIN_REFLECT_CLASS(BaseVmServerAppPart, qd::ApplicationPart);
    TS_ATTRIBUTE(qd::tsAttr::Name("Base VM provider"));
    TS_END();

public:
    BaseVmServerAppPart() = default;

    virtual qsr::IVmClientPlayer* getVmPlayer() = 0;

};  // class BaseVmServerAppPart
//////////////////////////////////////////////////////////////////////////


};  // namespace qsr
