#include "qd/base/base.h"
#if QD_USE_SDL

#include "thread.h"
#include "qd/debug/exception.h"
#include "qd/debug/exceptTryCatch.h"
#include "qd/log/log.h"
#include "SDL_thread.h"
#include <SDL_timer.h>
#include "qd/stl/algorithm.h"


namespace qd {

//////////////////////////////////////////////////////////////////////////
namespace Details {
class CThreadData
{
public:
    ImpThreadSdl2::ThreadFunc m_pThreadProc;
    qd::ImpThreadSdl2* m_pThread = nullptr; // backPtr
    bool m_bStarted = false;

public:
    CThreadData(qd::ImpThreadSdl2* pThread)
        : m_pThread(pThread) {}

    ~CThreadData() {}

    void setOnThreadDone() {
        m_bStarted = false;
        m_pThread->m_OnDoneEvent.set();
    }
}; // struct CThreadData


int _threadSDLProcStatic(void* _pData) {
    qd::Details::CThreadData* pThreadData = static_cast<qd::Details::CThreadData*>(_pData);
    if (pThreadData) {
        pThreadData->m_bStarted = true;
        QD_TRY {
            pThreadData->m_pThreadProc();
        }
        QD_CATCH(std::exception & ex) {
            logErr("EXCEPTION WARNING: Thread Exists with Exception: \"%s\"", ex.what());
            ASSERT_F(0, "Thread - Exception: %s", ex.what());
        };
        QD_CATCH(...) {
            logErr("UNHANDLED THREAD EXCEPTION ERROR HAPPENED!");
            ASSERT_F(0, "Thread -Unhandled Exception");
        };
        pThreadData->setOnThreadDone();
    }
    return 0;
}

}; // namespace Details
//////////////////////////////////////////////////////////////////////////


ImpThreadSdl2::~ImpThreadSdl2() {
    if (m_pSDLThread) {
        SDL_DetachThread(m_pSDLThread);
        m_pSDLThread = nullptr;
    }
    SAFE_DELETE(m_pThreadData);
}


void ImpThreadSdl2::create(void (*pThreadProc)(void*), void* pData) {
    auto threadLambda = [pThreadProc, pData]() {
        (*pThreadProc)(pData);
    };
    create(qtd::move(threadLambda));
}


void ImpThreadSdl2::create(ImpThreadSdl2::ThreadFunc&& threadProc) {
    assert(!m_pThreadData);
    m_pThreadData = new Details::CThreadData(this);
    m_pThreadData->m_pThreadProc = qtd::move(threadProc);

    m_pSDLThread = SDL_CreateThread(Details::_threadSDLProcStatic, m_pThreadName.data(), m_pThreadData);
    if (!m_pSDLThread)
        throw qd::Exception("can't create thread");
}


bool ImpThreadSdl2::isActive() {
    if (!m_pSDLThread)
        return false;
    return true;
}

void ImpThreadSdl2::kill() {
    SDL_DetachThread(m_pSDLThread);
    m_pSDLThread = nullptr;
    m_pThreadData = nullptr;
}


void ImpThreadSdl2::join() {
    SDL_WaitThread(m_pSDLThread, nullptr);
    m_pSDLThread = nullptr;
    m_pThreadData = nullptr;
}


bool ImpThreadSdl2::waitForDeath(float TimeOut) {
    if (!m_pThreadData || !m_pSDLThread)
        return true;

    if (m_pThreadData->m_bStarted && m_OnDoneEvent.wait((uint32_t)TimeOut)) { // Done Event
        // WAIT IS STILL ALIVE
        if (m_pThreadData && m_pThreadData->m_bStarted) {
            return false;
        }
        SDL_WaitThread(m_pSDLThread, nullptr);
        m_pSDLThread = nullptr;
        m_pThreadData = nullptr;
        return true;
    }
    else {
        if (m_pThreadData->m_bStarted)
            return false;
        else
            return true;
    }
}



bool ImpThreadSdl2::isCurrentThread() {
    return SDL_GetThreadID(m_pSDLThread) == SDL_ThreadID();
}


void ImpThreadSdl2::setThreadName(const qtd::string_view& pName) {
    m_pThreadName = pName;
}



ImpThreadEventSdl2::ImpThreadEventSdl2(bool auto_reset_event)
    : m_pCondition(nullptr)
    , m_pMutex(nullptr)
    , m_bState(false)
    , m_bAutoReset(auto_reset_event) {
    m_pCondition = SDL_CreateCond();
    if (!m_pCondition) {
        logDbg("Thread::ThreadEvent::create() : FAILED");
        return;
    }
    m_pMutex = SDL_CreateMutex();
    if (!m_pMutex) {
        SDL_DestroyCond(m_pCondition);
        m_pCondition = nullptr;
        logDbg("Thread::ThreadEvent::create() (mutex)");
        return;
    }
}


void ImpThreadEventSdl2::set() {
    if (SDL_LockMutex(m_pMutex)) {
        logDbg("cannot signal event (lock)");
        return;
    }
    m_bState = true;
    if (SDL_CondBroadcast(m_pCondition) != 0) {
        SDL_UnlockMutex(m_pMutex);
        logDbg("cannot signal event");
        return;
    }
    SDL_UnlockMutex(m_pMutex);
    SDL_CondSignal(m_pCondition);
}


void ImpThreadEventSdl2::wait() {
    if (SDL_LockMutex(m_pMutex) != 0) {
        logDbg("wait for event failed (lock)");
        return;
    }
    while (!m_bState) {
        if (SDL_CondWait(m_pCondition, m_pMutex) != 0) {
            SDL_UnlockMutex(m_pMutex);
            logDbg("wait for event failed");
            return;
        }
    }
    if (m_bAutoReset)
        m_bState = false;
    SDL_UnlockMutex(m_pMutex);
}


bool ImpThreadEventSdl2::wait(uint32_t time_out_ms) {
    if (SDL_LockMutex(m_pMutex) != 0) {
        logDbg("wait for event failed (lock)");
        return false;
    }

    int rc = 0;
    while (!m_bState) {
        rc = SDL_CondWaitTimeout(m_pCondition, m_pMutex, time_out_ms);
        if (rc != 0) {
            if (rc == SDL_MUTEX_TIMEDOUT)
                break;
            SDL_UnlockMutex(m_pMutex);
            logDbg("cannot wait for event");
            return false;
        }
    }
    if (rc == 0 && m_bAutoReset)
        m_bState = false;
    SDL_UnlockMutex(m_pMutex);
    return rc == 0;
}


void ImpThreadEventSdl2::reset() {
    if (SDL_LockMutex(m_pMutex) != 0) {
        logDbg("Cannot reset event");
        return;
    }
    m_bState = false;
    SDL_UnlockMutex(m_pMutex);
}


ImpThreadEventSdl2::~ImpThreadEventSdl2() {
    SDL_DestroyCond(m_pCondition);
    SDL_DestroyMutex(m_pMutex);
}


bool is_main_thread() {
    return SDL_ThreadID() == SDL_GetThreadID(nullptr);
}


void sleep_ms(uint32_t timeMs) {
    SDL_Delay(timeMs);
}


}; // namespace qd

#endif // QD_USE_SDL
