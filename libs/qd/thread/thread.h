#pragma once
#include "qd/base/base.h"
#include "qd/stl/string.h"
#include <EASTL/fixed_function.h>
#include <SDL_log.h>
#include <SDL_thread.h>
#include <SDL_timer.h>


namespace qd {
FORWARD_DECLARATION_2(Details, CThreadData);

//------------------------------------------------------------------------
class ThreadEvent
{
    SDL_cond* m_pCondition;
    SDL_mutex* m_pMutex;
    volatile bool m_bState;
    bool m_bAutoReset;

public:
    ThreadEvent(bool auto_reset_event = true);
    ~ThreadEvent();
    void reset();
    void set();
    void wait();
    bool wait(uint32_t time_out_ms);

}; // class thread::ThreadEvent
//////////////////////////////////////////////////////////////////////////


//------------------------------------------------------------------------
class Thread
{
    typedef Thread TThis;
    SDL_Thread* m_pSDLThread = nullptr;
    ThreadEvent m_OnDoneEvent = ThreadEvent(false);
    Details::CThreadData* m_pThreadData = nullptr;
    friend Details::CThreadData;
    qd::string_view m_pThreadName;

public:
    using ThreadFunc = eastl::fixed_function<2 * sizeof(void*), void()>;

public:
    Thread() = default;
    ~Thread();

    void create(ThreadFunc&& threadProc);
    void create(void (*pThreadProc)(void* pData), void* pData);

    void kill();
    void join();
    bool isActive();
    bool waitForDeath(float TimeOut);
    inline bool isCurrentThread() { return SDL_GetThreadID(m_pSDLThread) == SDL_ThreadID(); }
    void setThreadName(const qd::string_view& pName);
}; // class Thread
//////////////////////////////////////////////////////////////////////////



inline bool is_main_thread()
{
    return SDL_ThreadID() == SDL_GetThreadID(nullptr);
}


inline void sleep_ms(uint32_t timeMs)
{
    SDL_Delay(timeMs);
}


inline void sleep(float duration_sec)
{
    qd::sleep_ms((uint32_t)(duration_sec * 1000.0f));
}


}; // namespace qd
