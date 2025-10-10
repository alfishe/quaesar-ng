#pragma once
#include <memory>  // unique_ptr
#include "amDebugger/debuggerWndApp.h"
#include "qd/app/application.h"
#include "qd/typeSystem/typeDeclare.h"


FORWARD_DECLARATION_1S(SDL_Window);
FORWARD_DECLARATION_1S(SDL_Texture);
FORWARD_DECLARATION_1S(SDL_Renderer);
FORWARD_DECLARATION_2(qsr, UaeClientAppPart);
FORWARD_DECLARATION_2(qsr, QuaesarApplication);
FORWARD_DECLARATION_2(qd, ThreadEvent);
FORWARD_DECLARATION_2(amD, DebuggerApp);
FORWARD_DECLARATION_2(amD, Debugger);
FORWARD_DECLARATION_2(amD, IVmConnectionBuilder);

extern qsr::QuaesarApplication* g_pApp;

namespace qsr {
class BaseVmServerAppPart;
class IAppPartServerProviderFactory;
class QuaesarVmServersMgr;
class IVmServerThread;

//////////////////////////////////////////////////////////////////////////
class QuaesarApplication : public qd::Application {
    typedef qd::Application TSuper;

public:
    amD::DebuggerApp* m_pDebuggerApp = nullptr;
    qsr::UaeClientAppPart* m_pUaeClientAppPart = nullptr;

    //qsr::UaeServerAppPart* m_pUaeServerAppPart = nullptr;
    //qsr::VAmServerAppPart* m_pVAmServerAppPart = nullptr;

    qd::vector<BaseVmServerAppPart*> m_vmServerAppParts;
    QuaesarVmServersMgr* m_pVmServersMgr = nullptr;

    IAppPartServerProviderFactory* m_pCurServerFactory = nullptr;

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

    amD::Debugger* getDbg() const;
    qsr::UaeClientAppPart* getUaeClientApp() const {
        return m_pUaeClientAppPart;
    }

    virtual void* getInterface(const qd::TypeInfo& p_interface) override;

    amD::DebuggerApp* getDebuggerApp() const {
        return m_pDebuggerApp;
    }
    qsr::UaeClientAppPart* getUaeClientAppPart() const {
        return m_pUaeClientAppPart;
    }

};  // class QuaesarApplication
//////////////////////////////////////////////////////////////////////////


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


//------------------------------------------------------------------------
// VM Servers providers manager
//
class QuaesarVmServersMgr : public amD::IVmConnectionsManager {
    TS_REFLECT_CLASS(QuaesarVmServersMgr, amD::IVmConnectionsManager);
    QuaesarApplication* m_pApp = nullptr;

    struct VmServiceItem {
        EQuaServerId m_id;
        amD::IVmConnectionBuilder* m_pConnBuilder;
    };
    qd::vector<VmServiceItem> m_pVmServicesList;

public:
    QuaesarVmServersMgr(QuaesarApplication* pApp);
    virtual ~QuaesarVmServersMgr();

public:
    virtual uint32_t getNumConnections() override;
    virtual ref_ptr<amD::IVmDbgServiceBridge> createVmProvider(const char* id) override;
    void registerVmServer(EQuaServerId id, amD::IVmConnectionBuilder* pBuilder);

    amD::IVmConnectionBuilder* findVmConnBuilderByStrId(const char* id) const;

};  // class QuaesarVmServersMgr
//////////////////////////////////////////////////////////////////////////


struct ServerAppPartCreateCtx;

class BaseVmServerAppPart : public qd::ApplicationPart {
    TS_BEGIN_REFLECT_CLASS(BaseVmServerAppPart, qd::ApplicationPart);
    TS_ATTRIBUTE(qd::tsAttr::Name("Base VM provider"));
    TS_END();

public:
    BaseVmServerAppPart() = default;

    virtual void onVmServerCreate(qsr::ServerAppPartCreateCtx& ctx);
    virtual qsr::IVmServerThread* getServerThread() = 0;

};  // class BaseVmServerAppPart
//////////////////////////////////////////////////////////////////////////


struct ServerAppPartCreateCtx {
    qsr::QuaesarApplication* app = nullptr;
    qd::string outName;
    const qd::TypeInfo* outTypeInfo = nullptr;
    ref_ptr<qsr::BaseVmServerAppPart> outPartPtr = {};
};


class IAppPartServerProviderFactory {
public:
    virtual bool createServerAppPart(qsr::ServerAppPartCreateCtx& prm) = 0;
    virtual ~IAppPartServerProviderFactory() = default;
};


namespace plugin_api {
struct RegOnLoadAppPartServerFactory {
    RegOnLoadAppPartServerFactory(std::unique_ptr<IAppPartServerProviderFactory> factory);
};
};  //namespace plugin_api


};  // namespace qsr
