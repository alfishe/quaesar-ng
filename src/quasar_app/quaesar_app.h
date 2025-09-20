#pragma once
#include "amDebugger/debuggerApp.h"
#include "qd/app/application.h"
#include "qd/typeSystem/typeDeclare.h"


FORWARD_DECLARATION_1S(SDL_Window);
FORWARD_DECLARATION_1S(SDL_Texture);
FORWARD_DECLARATION_1S(SDL_Renderer);
FORWARD_DECLARATION_2(qsr, UaeClientAppPart);
FORWARD_DECLARATION_2(qsr, UaeServerAppPart);
FORWARD_DECLARATION_2(qd, ThreadEvent);
FORWARD_DECLARATION_2(amD, DebuggerApp);
FORWARD_DECLARATION_2(amD, Debugger);
FORWARD_DECLARATION_2(amD, IVmConnectionBuilder);


//////////////////////////////////////////////////////////////////////////
class QuasarApp : public qd::Application {
    typedef qd::Application TSuper;

public:
    amD::DebuggerApp* m_pDebuggerApp = nullptr;
    qsr::UaeClientAppPart* m_pUaeClientAppPart = nullptr;
    qsr::UaeServerAppPart* m_pUaeServerAppPart = nullptr;
    class QuaesarDebuggerServersMgr* m_pServersMgr = nullptr;

public:
    QuasarApp();
    virtual ~QuasarApp();
    virtual void onConstruct(qd::CreateApplicationParams& in) override;

    inline static QuasarApp* g_pInstance = nullptr;
    static QuasarApp* get() {
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

};  // class App
//////////////////////////////////////////////////////////////////////////


enum class EQuaServerId {
    UNDEF = 0,
    S_UAE = _MAKE4C("QUAE"),
    S_VAMIGA = _MAKE4C("VAMI"),
};


//------------------------------------------------------------------------
// Debugger Servers providers manager
//
class QuaesarDebuggerServersMgr : public amD::IVmConnectionsManager {
    TS_REFLECT_CLASS(QuaesarDebuggerServersMgr, amD::IVmConnectionsManager);
    QuasarApp* m_pApp = nullptr;

    struct VmServiceItem {
        EQuaServerId m_id;
        amD::IVmConnectionBuilder* m_pConnBuilder;
    };
    qd::vector<VmServiceItem> m_pVmServicesList;

public:
    QuaesarDebuggerServersMgr(QuasarApp* pApp);
    virtual ~QuaesarDebuggerServersMgr();

public:
    virtual uint32_t getNumConnections() override;
    virtual ref_ptr<amD::IVmServiceConnection> createVmConnectionByInd(uint32_t idx) override;
    void registerVmServer(EQuaServerId id, amD::IVmConnectionBuilder* pBuilder);

    amD::IVmConnectionBuilder* getVmConnBuilderById(EQuaServerId id) const;

};  // class QuaesarDebuggerServersMgr
//////////////////////////////////////////////////////////////////////////


namespace amD::uae {
extern void do_console_cmd_immediate(const char* cmd);
};  //namespace amD::uae
