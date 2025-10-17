#pragma once
#include <SDL.h>
#include <deque>
#include "amDebugger/dbgConnection.h"
#include "amDebugger/debuggerServer.h"
#include "qd/base/base.h"
#include "qd/qui/uiOperation.h"
#include "qd/stl/string.h"
#include "qd/stl/unique_ptr.h"
#include "qd/thread/mutex.h"
#include "quasar_app/qsr_app_interfaces.h"


FORWARD_DECLARATION_2(qd, ThreadEvent);
FORWARD_DECLARATION_3(IVm, imp, VAmVmImp);
FORWARD_DECLARATION_2(qsr, VAmServerAppPart);
FORWARD_DECLARATION_2(vamiga, VAmiga);
FORWARD_DECLARATION_2S(vamiga, MessageFwd);


//------------------------------------------------------------------------
// VAMIGA's parent server-work-thread
// (it works in the same thread as VAMIGA)
//
class VAmServerThread : public qsr::IVmClientPlayer {
    struct SDL_Thread* m_uaeThread = nullptr;  // start VAMIGA in separate thread
    inline static VAmServerThread* g_pSingleton = nullptr;
    qd::Mutex m_eventMutex;
    std::deque<SDL_Event> m_sdlEventsQueue;
    class VAmConsoleQueue* m_pConsoleQueue = nullptr;
    std::deque<qd::unique_ptr<qd::operation::BaseOpArgs>> m_pClientOpsStack;
    vamiga::VAmiga* m_pVAmiga = nullptr;
    bool m_bRequestToQuit = false;
    bool power_is_on_ = true;
    qd::string m_threadErrStr;

public:
    int m_scrWidth = 754;
    int m_scrHeight = 576;
    qd::Mutex m_VAmScrTextureMutex;
    uint32_t* m_pAmigaBuffer = nullptr;
    qd::ThreadEvent* m_onVAmInitialized = nullptr;  // event to wait for VAMIGA initialization
    SDL_atomic_t m_scrFrameNo = {};
    ref_ptr<IVm::imp::VAmVmImp> m_pVm;  // create shared VM
    qsr::VAmServerAppPart* m_pServerApp = nullptr;
    int m_threadErr = -1;

public:
    VAmServerThread(qsr::VAmServerAppPart* pServerApp);
    ~VAmServerThread();
    void initialize();
    void destroy();
    void setVAmInitialized(bool);

    //uint32_t* lockVAmScreenTexBuf(int amiga_width, int amiga_height);
    //void unlockVAmScreenTexBuf();

    virtual IVm::VM* getVm() const override;
    virtual int getScrFrameNo() override;
    virtual void pushSdlEvent(const SDL_Event& event) override;
    virtual void pushOperationMsg(qd::unique_ptr<qd::operation::BaseOpArgs> args) override;
    virtual bool lockDisplayTexBuf(int* out_width, int* out_height, uint32_t** out_pixels) override;
    virtual void unlockDisplayTexBuf() override;

    void vAmigaMsgQueueProc(const vamiga::MessageFwd& msg);
    bool onVAmHandleEvents();

    void execConsoleCmd(qd::string&& cmd);
    int uaeWaitConsoleCmdImpl(char* out, int maxlen);

public:
    static VAmServerThread* get() {
        return g_pSingleton;
    }

    void onVAmigaThreadMain();

protected:
    void applySdlEventProc(const SDL_Event& event);
    void applyImmediateConsoleCmd(qd::string&& cmd);

private:
    void fetchScreenBufferToTexture(const uint32_t* pCurDisplayTexBuf, bool lof);
};  // class VAmServerThread
//////////////////////////////////////////////////////////////////////////
