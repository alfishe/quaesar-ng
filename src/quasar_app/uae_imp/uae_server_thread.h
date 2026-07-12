#pragma once
#include <SDL.h>
#include <deque>
#include "amDebugger/dbgConnection.h"
#include "amDebugger/debuggerServer.h"
#include "qd/base/base.h"
#include "qd/qui/uiOperation.h"
#include "qd/stl/string.h"
#include "qd/thread/mutex.h"
#include "qsr_app_interfaces.h"


#include "qd/thread/thread.h"
FORWARD_DECLARATION_3(IVm, imp, UaeVmImp);
FORWARD_DECLARATION_2(qsr, UaeServerAppPart);


//------------------------------------------------------------------------
// UAE's parent server-work-thread
// (it works in the same thread as UAE)
//
class UaeServerThread : public qsr::IVmClientPlayer {
    struct SDL_Thread* m_uaeThread = nullptr;  // start UAE in separate thread
    inline static UaeServerThread* g_pSingleton = nullptr;
    qd::Mutex m_eventMutex;
    std::deque<SDL_Event> m_sdlEventsQueue;
    class UaeConsoleQueue* m_pConsoleQueue = nullptr;
    std::deque<qtd::unique_ptr<qd::operation::BaseOpArgs>> m_pClientOpsStack;
    bool m_isDestroying = false;
    bool m_isPaused = false;
    qd::ThreadEvent* m_pauseEvent = nullptr;

public:
    int m_scrWidth = 754;
    int m_scrHeight = 576;
    qd::Mutex m_UaeScrTextureMutex;
    uint32_t* m_pAmigaBuffer = nullptr;
    qd::ThreadEvent* m_onUaeInitialized = nullptr;  // event to wait for UAE initialization
    SDL_atomic_t m_scrFrameNo = {};
    ref_ptr<IVm::imp::UaeVmImp> m_pVm;  // create shared VM
    qsr::UaeServerAppPart* m_pServerApp = nullptr;

public:
    UaeServerThread(qsr::UaeServerAppPart* pServerApp);
    virtual ~UaeServerThread();
    void initialize();
    void destroy();
    void setUaeInitialized(bool);

    virtual int getScrFrameNo() override;
    virtual IVm::VM* getVm() const override;
    virtual bool lockDisplayTexBuf(int* width, int* height, uint32_t** out_pixels) override;
    virtual void unlockDisplayTexBuf() override;
    virtual void pushSdlEvent(const SDL_Event& event) override;
    virtual void pushOperationMsg(qtd::unique_ptr<qd::operation::BaseOpArgs> args) override;
    bool onUaeHandleEvents();

    void pauseEmulation();
    void resumeEmulation();
    bool isPaused() const {
        return m_isPaused;
    }

    void execConsoleCmd(qtd::string&& cmd);
    int uaeWaitConsoleCmdImpl(char* out, int maxlen);

public:
    //
    // Back-end functions for UAE VM implementation
    //

    static UaeServerThread* get() {
        return g_pSingleton;
    }
    uint32_t* _lockUaeScreenTexBuf(int amiga_width, int amiga_height);
    void _unlockUaeScreenTexBuf();

protected:
    void applySdlEventProc(const SDL_Event& event);
    void applyImmediateConsoleCmd(qtd::string&& cmd);

};  // class UaeServerThread
//////////////////////////////////////////////////////////////////////////
