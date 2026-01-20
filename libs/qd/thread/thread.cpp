#include "thread.h"
#include "qd/debug/exception.h"
#include "qd/debug/exceptTryCatch.h"
#include "qd/log/log.h"
#include "SDL_thread.h"
#include <SDL_timer.h>


namespace qd {

//////////////////////////////////////////////////////////////////////////
namespace Details {
class CThreadData
{
public:
    // void (*m_pThreadProc)(void* pData) = nullptr;
    Thread::ThreadFunc m_pThreadProc;
    qd::Thread* m_pThread = nullptr; // backPtr
    bool m_bStarted = false;

public:
    CThreadData(qd::Thread* pThread)
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
        G_TRY {
            pThreadData->m_pThreadProc();
        }
        G_CATCH(std::exception & ex) {
            logErr("EXCEPTION WARNING: Thread Exists with Exception: \"%s\"", ex.what());
            assert2(0, "Thread - Exception: %s", ex.what());
        };
        G_CATCH(...) {
            logErr("UNHANDLED THREAD EXCEPTION ERROR HAPPENED!");
            assert2(0, "Thread -Unhandled Exception");
        };
        pThreadData->setOnThreadDone();
    }
    return 0;
}

}; // namespace Details
//////////////////////////////////////////////////////////////////////////


Thread::~Thread() {
    if (m_pSDLThread) {
        SDL_DetachThread(m_pSDLThread);
        m_pSDLThread = nullptr;
    }
    SAFE_DELETE(m_pThreadData);
}


void Thread::create(void (*pThreadProc)(void*), void* pData) {
    auto threadLambda = [pThreadProc, pData]() {
        (*pThreadProc)(pData);
    };
    create(std::move(threadLambda));
}


void Thread::create(Thread::ThreadFunc&& threadProc) {
    assert(!m_pThreadData);
    m_pThreadData = new Details::CThreadData(this);
    m_pThreadData->m_pThreadProc = std::move(threadProc);

    m_pSDLThread = SDL_CreateThread(Details::_threadSDLProcStatic, m_pThreadName.data(), m_pThreadData);
    if (!m_pSDLThread)
        throw qd::Exception("can't create thread");
}


bool Thread::isActive() {
    if (!m_pSDLThread)
        return false;
    return true;
}

void Thread::kill() {
    SDL_DetachThread(m_pSDLThread);
    m_pSDLThread = nullptr;
    m_pThreadData = nullptr;
}


void Thread::join() {
    SDL_WaitThread(m_pSDLThread, nullptr);
    m_pSDLThread = nullptr;
    m_pThreadData = nullptr;
}


bool Thread::waitForDeath(float TimeOut) {
    if (!m_pThreadData || !m_pSDLThread)
        return true;

    if (m_pThreadData->m_bStarted && m_OnDoneEvent.wait((uint32_t)TimeOut)) { // Done Event
        // WAIT IS STILL ALIVE
        if (m_pThreadData && m_pThreadData->m_bStarted) {
            return false;
        }
        SDL_WaitThread(m_pSDLThread, nullptr);
        // SDL_DetachThread(m_pSDLThread);
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



bool Thread::isCurrentThread() {
    return SDL_GetThreadID(m_pSDLThread) == SDL_ThreadID();
}


void Thread::setThreadName(const qd::string_view& pName) {
    m_pThreadName = pName;
}



ThreadEvent::ThreadEvent(bool auto_reset_event)
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


void ThreadEvent::set() {
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


void ThreadEvent::wait() {
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


bool ThreadEvent::wait(uint32_t time_out_ms) {
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


void ThreadEvent::reset() {
    if (SDL_LockMutex(m_pMutex) != 0) {
        logDbg("Cannot reset event");
        return;
    }
    m_bState = false;
    SDL_UnlockMutex(m_pMutex);
}


ThreadEvent::~ThreadEvent() {
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
