#pragma once
#include "qd/app/appliction.h"


FORWARD_DECLARATION_2(qd, ThreadEvent);
FORWARD_DECLARATION_2(amD, Debugger);

struct SDL_Window;
struct SDL_Texture;
struct SDL_Renderer;

class UaeAppPart;

namespace amD {
extern qd::ThreadEvent* onUaeInitialized;


//////////////////////////////////////////////////////////////////////////
class QuasarApp : public qd::Application {
    typedef qd::Application TSuper;

public:
    amD::Debugger* m_pDebugger = nullptr;
    UaeAppPart* m_pUaeAppPart = nullptr;

public:
    QuasarApp() {
        QuasarApp::g_pInstance = this;
    }
    virtual void onCreate(qd::CreateApplicationParams& in) override;

    inline static QuasarApp* g_pInstance = nullptr;
    ;
    static QuasarApp* get() {
        return g_pInstance;
    }
    void initialize();
    virtual void destroyImp() override;

    virtual void onSdlEventProc(SDL_Event& event) override;

    amD::Debugger* getDbg() const {
        return m_pDebugger;
    }
    UaeAppPart* getUaeApp() const {
        return m_pUaeAppPart;
    }

};  // class App
//////////////////////////////////////////////////////////////////////////


namespace uae {
extern void do_console_cmd_immediate(const char* cmd);
};

};  //namespace amD
