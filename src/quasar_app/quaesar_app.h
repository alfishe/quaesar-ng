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
FORWARD_DECLARATION_2(amD, IDebuggerServer);


//////////////////////////////////////////////////////////////////////////
class QuasarApp : public qd::Application {
    typedef qd::Application TSuper;

public:
    amD::DebuggerApp* m_pDebuggerApp = nullptr;
    qsr::UaeClientAppPart* m_pUaeClientAppPart = nullptr;
    qsr::UaeServerAppPart* m_pUaeServerAppPart = nullptr;
    class QuaesarServersMgr* m_pServersMgr = nullptr;

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
    S_UAE = _MAKE4C('_UAE'),
    S_LOCAL_VAMIGA,
    S_REMOTE_VAMIGA,
};


// Debugger Servers providers manager
//
class QuaesarServersMgr : public amD::IDbgConnectionManager {
    TS_REFLECT_CLASS(QuaesarServersMgr, amD::IDbgConnectionManager);
    QuasarApp* m_pApp;
    struct ServerItem {
        EQuaServerId m_id;
        amD::IDebuggerServer* m_server;
    };
    qd::vector<ServerItem> m_pServers;

public:
    QuaesarServersMgr(QuasarApp* pApp);
    virtual ~QuaesarServersMgr();

public:
    virtual uint32_t getNumConnections() override;
    virtual ref_ptr<amD::IDbgConnection> createConnectionByInd(uint32_t idx) override;
    void registerVmServer(EQuaServerId id, amD::IDebuggerServer* pServer);

    amD::IDebuggerServer* getServerById(EQuaServerId id) const;

};  // class QuasarServersMgr
//////////////////////////////////////////////////////////////////////////


namespace amD::uae {
extern void do_console_cmd_immediate(const char* cmd);
};  //namespace amD::uae
