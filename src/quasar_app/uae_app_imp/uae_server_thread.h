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


FORWARD_DECLARATION_2(qd, ThreadEvent);
FORWARD_DECLARATION_4(amD, vm, imp, UaeVmImp);
FORWARD_DECLARATION_2(qsr, UaeServerAppPart);


//------------------------------------------------------------------------
// UAE's parent server-work-thread
// (it works in the same thread as UAE)
//
class UaeServerThread {
    struct SDL_Thread* m_uaeThread = nullptr;  // start UAE in separate thread
    inline static UaeServerThread* g_pSingleton = nullptr;
    qd::Mutex m_eventMutex;
    std::deque<SDL_Event> m_sdlEventsQueue;
    class UaeConsoleQueue* m_pConsoleQueue = nullptr;
    std::deque<qd::unique_ptr<qd::operation::BaseOpArgs>> m_pClientOpsStack;


public:
    int m_scrWidth = 754;
    int m_scrHeight = 576;
    qd::Mutex m_UaeScrTextureMutex;
    uint32_t* m_pAmigaBuffer = nullptr;
    qd::ThreadEvent* m_onUaeInitialized = nullptr;  // event to wait for UAE initialization
    SDL_atomic_t m_scrFrameNo = {};
    ref_ptr<amD::vm::imp::UaeVmImp> m_pVm;  // create shared VM
    qsr::UaeServerAppPart* m_pServerApp = nullptr;

public:
    UaeServerThread(qsr::UaeServerAppPart* pServerApp);
    ~UaeServerThread();
    void initialize();
    void destroy();
    void setUaeInitialized(bool);

    uint32_t* lockUaeScreenTexBuf(int amiga_width, int amiga_height);
    void unlockUaeScreenTexBuf();

    int getScrFrameNo();
    void pushSdlEvent(const SDL_Event& event);
    void pushOperationMsg(qd::unique_ptr<qd::operation::BaseOpArgs> args);

    bool onUaeHandleEvents();

    IVm::VM* getVm() const;

    void execConsoleCmd(qd::string&& cmd);
    int uaeWaitConsoleCmdImpl(char* out, int maxlen);

public:
    static UaeServerThread* get() {
        return g_pSingleton;
    }

protected:
    void applySdlEventProc(const SDL_Event& event);
    void applyImmediateConsoleCmd(qd::string&& cmd);

};  // class UaeServerThread
//////////////////////////////////////////////////////////////////////////
