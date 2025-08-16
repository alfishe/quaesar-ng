#pragma once
#include <SDL.h>
#include <deque>
#include "amDebugger/dbgConnection.h"
#include "amDebugger/debuggerServer.h"
#include "qd/base/base.h"
#include "qd/qui/uiOperation.h"
#include "qd/stl/string.h"
#include "qd/thread/mutex.h"
//#include "uae_vm_imp.h"


FORWARD_DECLARATION_2(qd, ThreadEvent);
FORWARD_DECLARATION_4(amD, vm, imp, UaeVmImp);


//////////////////////////////////////////////////////////////////////////
// UAE's parent server-work-thread
// (it works in the same thread as UAE)
class UaeServerThread : public amD::IDebuggerServer {
    struct SDL_Thread* m_uaeThread = nullptr;  // start UAE in separate thread
    inline static UaeServerThread* g_pSingleton = nullptr;
    qd::Mutex m_eventMutex;
    std::deque<SDL_Event> m_eventQueue;
    class ConsoleQueue* m_pConsoleQueue = nullptr;

public:
    int m_scrWidth = 754;
    int m_scrHeight = 576;
    qd::Mutex m_UaeScrTextureMutex;
    uint32_t* m_pAmigaBuffer = nullptr;
    qd::ThreadEvent* m_onUaeInitialized = nullptr;  // event to wait for UAE initialization
    SDL_atomic_t m_scrFrameNo = {};
    ref_ptr<amD::vm::imp::UaeVmImp> m_pVm;  // create shared VM

    virtual void* getOpEnvPtr(const qd::TypeInfo& classType) const override;
    virtual qd::EFlow applyOperationMsg(qd::operation::args::Base* args) override;

public:
    UaeServerThread();
    ~UaeServerThread();
    void initialize();
    void destroy();
    void setUaeInitialized(bool);

    uint32_t* lockUaeScreenTexBuf(int amiga_width, int amiga_height);
    void unlockUaeScreenTexBuf();
    int getScrFrameNo();
    void pushSdlEvent(const SDL_Event& event);
    bool onUaeHandleEvents();

    IVm::VM* getVm() const;
    virtual ref_ptr<amD::IDbgConnection> createConnection() const override;

    void execConsoleCmd(qd::string&& cmd);
    void applyImmediateConsoleCmd(qd::string&& cmd);
    int waitConsoleCmd(char* out, int maxlen);

public:
    static UaeServerThread* get() {
        return g_pSingleton;
    }

protected:
    void onSdlEventProc(const SDL_Event& event);

};  // class UaeServerThread
//////////////////////////////////////////////////////////////////////////
