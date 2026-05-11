#include "qd/base/base.h"
#if !QD_USE_SDL

#include "thread.h"
#include "qd/debug/exception.h"
#include "qd/debug/exceptTryCatch.h"
#include "qd/log/log.h"
#include "qd/stl/algorithm.h"
#include <chrono>


namespace qd {

static std::thread::id s_mainThreadId = std::this_thread::get_id();

//////////////////////////////////////////////////////////////////////////
namespace Details {
class CThreadData
{
public:
    ImpThreadPosix::ThreadFunc m_pThreadProc;
    qd::ImpThreadPosix* m_pThread = nullptr; // backPtr
    bool m_bStarted = false;

public:
    CThreadData(qd::ImpThreadPosix* pThread)
        : m_pThread(pThread) {}

    ~CThreadData() {}

    void setOnThreadDone() {
        m_bStarted = false;
        m_pThread->m_OnDoneEvent.set();
    }
}; // struct CThreadData
}; // namespace Details
//////////////////////////////////////////////////////////////////////////


static void _threadProcStatic(qd::Details::CThreadData* pThreadData) {
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
}


ImpThreadPosix::~ImpThreadPosix() {
    if (m_thread.joinable()) {
        m_thread.detach();
    }
    SAFE_DELETE(m_pThreadData);
}


void ImpThreadPosix::create(void (*pThreadProc)(void*), void* pData) {
    auto threadLambda = [pThreadProc, pData]() {
        (*pThreadProc)(pData);
    };
    create(qtd::move(threadLambda));
}


void ImpThreadPosix::create(ImpThreadPosix::ThreadFunc&& threadProc) {
    assert(!m_pThreadData);
    m_pThreadData = new Details::CThreadData(this);
    m_pThreadData->m_pThreadProc = qtd::move(threadProc);

    m_thread = std::thread(_threadProcStatic, m_pThreadData);
    if (!m_thread.joinable())
        throw qd::Exception("can't create thread");
}


bool ImpThreadPosix::isActive() {
    return m_thread.joinable();
}

void ImpThreadPosix::kill() {
    if (m_thread.joinable())
        m_thread.detach();
    m_pThreadData = nullptr;
}


void ImpThreadPosix::join() {
    if (m_thread.joinable())
        m_thread.join();
    m_pThreadData = nullptr;
}


bool ImpThreadPosix::waitForDeath(float TimeOut) {
    if (!m_pThreadData || !m_thread.joinable())
        return true;

    if (m_pThreadData->m_bStarted && m_OnDoneEvent.wait((uint32_t)TimeOut)) {
        if (m_pThreadData && m_pThreadData->m_bStarted) {
            return false;
        }
        if (m_thread.joinable())
            m_thread.join();
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



bool ImpThreadPosix::isCurrentThread() {
    return m_thread.get_id() == std::this_thread::get_id();
}


void ImpThreadPosix::setThreadName(const qtd::string_view& pName) {
    m_pThreadName = pName;
}



ImpThreadEventPosix::ImpThreadEventPosix(bool auto_reset_event)
    : m_bState(false)
    , m_bAutoReset(auto_reset_event) {
}


void ImpThreadEventPosix::set() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_bState = true;
    }
    m_condition.notify_all();
}


void ImpThreadEventPosix::wait() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_condition.wait(lock, [this]() { return (bool)m_bState; });
    if (m_bAutoReset)
        m_bState = false;
}


bool ImpThreadEventPosix::wait(uint32_t time_out_ms) {
    std::unique_lock<std::mutex> lock(m_mutex);
    bool result = m_condition.wait_for(lock, std::chrono::milliseconds(time_out_ms),
        [this]() { return (bool)m_bState; });
    if (result && m_bAutoReset)
        m_bState = false;
    return result;
}


void ImpThreadEventPosix::reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_bState = false;
}


ImpThreadEventPosix::~ImpThreadEventPosix() {
}


bool is_main_thread() {
    return std::this_thread::get_id() == s_mainThreadId;
}


void sleep_ms(uint32_t timeMs) {
    std::this_thread::sleep_for(std::chrono::milliseconds(timeMs));
}


}; // namespace qd

#endif // !QD_USE_SDL
