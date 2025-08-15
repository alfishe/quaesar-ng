#include "thread.h"
#include "qd/debug/exceptTryCatch.h"
#include "qd/log/log.h"
#include "qd/debug/exception.h"
#include "SDL_thread.h"


namespace qd {

//////////////////////////////////////////////////////////////////////////
namespace Details {
class CThreadData
{
public:
    //void (*m_pThreadProc)(void* pData) = nullptr;
    Thread::ThreadFunc m_pThreadProc;
    qd::Thread* m_pThread = nullptr; // backPtr
    bool m_bStarted = false;

public:
    CThreadData(qd::Thread* pThread)
        : m_pThread(pThread)
    {}

    ~CThreadData()
    {
    }

    void setOnThreadDone()
    {
        m_bStarted = false;
        m_pThread->m_OnDoneEvent.set();
    }
}; // struct CThreadData


int _threadSDLProcStatic(void* _pData)
{
    qd::Details::CThreadData* pThreadData = static_cast<qd::Details::CThreadData*>(_pData);
    if (pThreadData)
    {
        pThreadData->m_bStarted = true;
        G_TRY
        {
            pThreadData->m_pThreadProc();
        }
        G_CATCH(std::exception & ex)
        {
            log_error("EXCEPTION WARNING: Thread Exists with Exception: \"%s\"", ex.what());
            assert2(0, "Thread - Exception: %s", ex.what());
        };
        G_CATCH(...)
        {
            log_error("UNHANDLED THREAD EXCEPTION ERROR HAPPENED!");
            assert2(0, "Thread -Unhandled Exception");
        };
        pThreadData->setOnThreadDone();
    }
    return 0;
}

}; // namespace Details
//////////////////////////////////////////////////////////////////////////


 Thread::~Thread()
{
    if (m_pSDLThread)
    {
        SDL_DetachThread(m_pSDLThread);
        m_pSDLThread = nullptr;
    }
    SAFE_DELETE(m_pThreadData);
}


 void Thread::create(void (*pThreadProc)(void*), void* pData, uint32_t nStackSize /*= 0*/)
 {
     create([pThreadProc, pData]() { (*pThreadProc)(pData); }, nStackSize);
 }


void Thread::create(Thread::ThreadFunc&& threadProc, uint32_t nStackSize /*= 0*/)
{
    assert(!m_pThreadData);
    m_pThreadData = new Details::CThreadData(this);
    m_pThreadData->m_pThreadProc = std::move(threadProc);
    //m_pThreadData->m_pData = pData;

    m_pSDLThread = SDL_CreateThread(Details::_threadSDLProcStatic, m_pThreadName.data(), m_pThreadData);
    if (!m_pSDLThread)
        throw qd::Exception("can't create thread");
}


bool Thread::isActive()
{
    if (!m_pSDLThread)
        return false;
    return true;
}

void Thread::kill()
{
    SDL_DetachThread(m_pSDLThread);
    m_pSDLThread = nullptr;
    m_pThreadData = nullptr;
}


void Thread::join()
{
    SDL_WaitThread(m_pSDLThread, nullptr);
    m_pSDLThread = nullptr;
    m_pThreadData = nullptr;
}


bool Thread::waitForDeath(float TimeOut)
{
    if (!m_pThreadData || !m_pSDLThread)
        return true;

    if (m_pThreadData->m_bStarted && m_OnDoneEvent.wait((uint32_t)TimeOut))
    { // Done Event
        // WAIT IS STILL ALIVE
        if (m_pThreadData && m_pThreadData->m_bStarted)
        {
            return false;
        }
        SDL_WaitThread(m_pSDLThread, nullptr);
        // SDL_DetachThread(m_pSDLThread);
        m_pSDLThread = nullptr;
        m_pThreadData = nullptr;
        return true;
    }
    else
    {
        if (m_pThreadData->m_bStarted)
            return false;
        else
            return true;
    }
}



void Thread::setThreadName(const qd::string_view& pName)
{
    m_pThreadName = pName;
}



ThreadEvent::ThreadEvent(bool auto_reset_event)
    : mbState(false)
    , mbAutoReset(auto_reset_event)
    , mpMutex(nullptr)
{
    mpCondition = SDL_CreateCond();
    if (!mpCondition)
    {
        SDL_Log("Thread::ThreadEvent::create() : FAILED");
        return;
    }
    mpMutex = SDL_CreateMutex();
    if (!mpMutex)
    {
        SDL_DestroyCond(mpCondition);
        mpCondition = nullptr;
        SDL_Log("Thread::ThreadEvent::create() (mutex)");
        return;
    }
}


void ThreadEvent::set()
{
    if (SDL_LockMutex(mpMutex))
    {
        SDL_Log("cannot signal event (lock)");
        return;
    }
    mbState = true;
    if (SDL_CondBroadcast(mpCondition) != 0)
    {
        SDL_UnlockMutex(mpMutex);
        SDL_Log("cannot signal event");
        return;
    }
    SDL_UnlockMutex(mpMutex);
    SDL_CondSignal(mpCondition);
}


void ThreadEvent::wait()
{
    if (SDL_LockMutex(mpMutex) != 0)
    {
        SDL_Log("wait for event failed (lock)");
        return;
    }
    while (!mbState)
    {
        if (SDL_CondWait(mpCondition, mpMutex) != 0)
        {
            SDL_UnlockMutex(mpMutex);
            SDL_Log("wait for event failed");
            return;
        }
    }
    if (mbAutoReset)
        mbState = false;
    SDL_UnlockMutex(mpMutex);
}


bool ThreadEvent::wait(uint32_t time_out_ms)
{
    if (SDL_LockMutex(mpMutex) != 0)
    {
        SDL_Log("wait for event failed (lock)");
        return false;
    }

    int rc = 0;
    while (!mbState)
    {
        rc = SDL_CondWaitTimeout(mpCondition, mpMutex, time_out_ms);
        if (rc != 0)
        {
            if (rc == SDL_MUTEX_TIMEDOUT)
                break;
            SDL_UnlockMutex(mpMutex);
            SDL_Log("cannot wait for event");
            return false;
        }
    }
    if (rc == 0 && mbAutoReset)
        mbState = false;
    SDL_UnlockMutex(mpMutex);
    return rc == 0;
}


void ThreadEvent::reset()
{
    if (SDL_LockMutex(mpMutex) != 0)
    {
        SDL_Log("Cannot reset event");
        return;
    }
    mbState = false;
    SDL_UnlockMutex(mpMutex);
}


ThreadEvent::~ThreadEvent()
{
    SDL_DestroyCond(mpCondition);
    SDL_DestroyMutex(mpMutex);
}


}; // namespace qd
