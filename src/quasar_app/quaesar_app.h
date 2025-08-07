#pragma once
#include "qd/app/appliction.h"


struct SDL_Window;
struct SDL_Texture;
struct SDL_Renderer;
class UaeWndAppPart;
FORWARD_DECLARATION_2(qd, ThreadEvent);
FORWARD_DECLARATION_2(amD, DebuggerApp);
FORWARD_DECLARATION_2(amD, Debugger);


//////////////////////////////////////////////////////////////////////////
class QuasarApp : public qd::Application {
    typedef qd::Application TSuper;

public:
    amD::DebuggerApp* m_pDebuggerPart = nullptr;
    UaeWndAppPart* m_pUaeAppPart = nullptr;

public:
    QuasarApp() {
        QuasarApp::g_pInstance = this;
    }
    virtual void onConstruct(qd::CreateApplicationParams& in) override;

    inline static QuasarApp* g_pInstance = nullptr;
    static QuasarApp* get() {
        return g_pInstance;
    }
    void initialize();
    virtual void destroyImp() override;

    virtual void onSdlEventProc(SDL_Event& event) override;

    amD::Debugger* getDbg() const;
    UaeWndAppPart* getUaeApp() const {
        return m_pUaeAppPart;
    }

};  // class App
//////////////////////////////////////////////////////////////////////////


namespace amD {
namespace uae {
extern void do_console_cmd_immediate(const char* cmd);
};
};  //namespace amD
