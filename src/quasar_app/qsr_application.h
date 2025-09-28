#pragma once
#include "amDebugger/debuggerWndApp.h"
#include "qd/app/application.h"
#include "qd/typeSystem/typeDeclare.h"


FORWARD_DECLARATION_1S(SDL_Window);
FORWARD_DECLARATION_1S(SDL_Texture);
FORWARD_DECLARATION_1S(SDL_Renderer);
FORWARD_DECLARATION_2(qsr, UaeClientAppPart);
FORWARD_DECLARATION_2(qsr, UaeServerAppPart);
FORWARD_DECLARATION_2(qsr, VAmServerAppPart);
FORWARD_DECLARATION_2(qd, ThreadEvent);
FORWARD_DECLARATION_2(amD, DebuggerApp);
FORWARD_DECLARATION_2(amD, Debugger);
FORWARD_DECLARATION_2(amD, IVmConnectionBuilder);


//////////////////////////////////////////////////////////////////////////
class QuaesarApplication : public qd::Application {
    typedef qd::Application TSuper;

public:
    amD::DebuggerApp* m_pDebuggerApp = nullptr;
    qsr::UaeClientAppPart* m_pUaeClientAppPart = nullptr;
    qsr::UaeServerAppPart* m_pUaeServerAppPart = nullptr;
    qsr::VAmServerAppPart* m_pVAmServerAppPart = nullptr;
    class QuaesarVmServersMgr* m_pVmServersMgr = nullptr;

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
    ENUM_DECLARE_BASE(::, EQuaServerId, Type, UNDEF);
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
    virtual ref_ptr<amD::IVmServiceProvider> createVmProvider(const char* id) override;
    void registerVmServer(EQuaServerId id, amD::IVmConnectionBuilder* pBuilder);

    amD::IVmConnectionBuilder* findVmConnBuilderByStrId(const char* id) const;

};  // class QuaesarVmServersMgr
//////////////////////////////////////////////////////////////////////////


namespace amD::uae {
extern void do_console_cmd_immediate(const char* cmd);
};  //namespace amD::uae
