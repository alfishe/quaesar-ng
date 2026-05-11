#pragma once
#include "qd/base/base.h"
#include "qd/stl/string.h"
#include "qd/stl/fixed_function.h"

#if QD_USE_SDL
struct SDL_cond;
struct SDL_mutex;
struct SDL_Thread;
#else
#include <thread>
#include <mutex>
#include <condition_variable>
#endif


namespace qd {


//------------------------------------------------------------------------
// ThreadEvent
//------------------------------------------------------------------------

class ImpThreadEventSdl2
{
#if QD_USE_SDL
    SDL_cond* m_pCondition;
    SDL_mutex* m_pMutex;
    volatile bool m_bState;
    bool m_bAutoReset;

public:
    ImpThreadEventSdl2(bool auto_reset_event = true);
    ~ImpThreadEventSdl2();
    void reset();
    void set();
    void wait();
    bool wait(uint32_t time_out_ms);
#endif
}; // class ImpThreadEventSdl2
//////////////////////////////////////////////////////////////////////////


class ImpThreadEventPosix
{
#if !QD_USE_SDL
    std::condition_variable m_condition;
    std::mutex m_mutex;
    volatile bool m_bState = false;
    bool m_bAutoReset;

public:
    ImpThreadEventPosix(bool auto_reset_event = true);
    ~ImpThreadEventPosix();
    void reset();
    void set();
    void wait();
    bool wait(uint32_t time_out_ms);
#endif
}; // class ImpThreadEventPosix
//////////////////////////////////////////////////////////////////////////


#if QD_USE_SDL
using ThreadEvent = ImpThreadEventSdl2;
#else
using ThreadEvent = ImpThreadEventPosix;
#endif


//------------------------------------------------------------------------
// Thread
//------------------------------------------------------------------------

FORWARD_DECLARATION_2(Details, CThreadData);

class ImpThreadSdl2
{
#if QD_USE_SDL
    typedef ImpThreadSdl2 TThis;
    SDL_Thread* m_pSDLThread = nullptr;
    ThreadEvent m_OnDoneEvent = ThreadEvent(false);
    Details::CThreadData* m_pThreadData = nullptr;
    friend Details::CThreadData;
    qtd::string_view m_pThreadName;

public:
    using ThreadFunc = qtd::fixed_function<2 * sizeof(void*), void()>;

public:
    ImpThreadSdl2() = default;
    ~ImpThreadSdl2();

    void create(ThreadFunc&& threadProc);
    void create(void (*pThreadProc)(void* pData), void* pData);

    void kill();
    void join();
    bool isActive();
    bool waitForDeath(float TimeOut);
    bool isCurrentThread();
    void setThreadName(const qtd::string_view& pName);
#endif
}; // class ImpThreadSdl2
//////////////////////////////////////////////////////////////////////////


class ImpThreadPosix
{
#if !QD_USE_SDL
    typedef ImpThreadPosix TThis;
    std::thread m_thread;
    ThreadEvent m_OnDoneEvent = ThreadEvent(false);
    Details::CThreadData* m_pThreadData = nullptr;
    friend Details::CThreadData;
    qtd::string_view m_pThreadName;

public:
    using ThreadFunc = qtd::fixed_function<2 * sizeof(void*), void()>;

public:
    ImpThreadPosix() = default;
    ~ImpThreadPosix();

    void create(ThreadFunc&& threadProc);
    void create(void (*pThreadProc)(void* pData), void* pData);

    void kill();
    void join();
    bool isActive();
    bool waitForDeath(float TimeOut);
    bool isCurrentThread();
    void setThreadName(const qtd::string_view& pName);
#endif
}; // class ImpThreadPosix
//////////////////////////////////////////////////////////////////////////


#if QD_USE_SDL
using Thread = ImpThreadSdl2;
#else
using Thread = ImpThreadPosix;
#endif


//------------------------------------------------------------------------
bool is_main_thread();

void sleep_ms(uint32_t timeMs);

inline void sleep(float duration_sec) {
    qd::sleep_ms((uint32_t)(duration_sec * 1000.0f));
}


}; // namespace qd
