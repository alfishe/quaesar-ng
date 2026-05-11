#pragma once
#include "qd/base/base.h"
#include "qd/debug/assert.h"

#if QD_USE_SDL
#include <SDL_mutex.h>
#else
#include <mutex>
#endif
#include <type_traits>


namespace qd {


#if QD_USE_SDL
//------------------------------------------------------------------------
class ImpMutexSdl2
{
    SDL_mutex* mpMutex;

public:
    ImpMutexSdl2() {
        mpMutex = SDL_CreateMutex();
        if (!mpMutex) {
            assert(0 && "cannot create mutex");
            return;
        }
    }

    ImpMutexSdl2(const ImpMutexSdl2&) = delete;
    void operator= (const ImpMutexSdl2&) = delete;

    ~ImpMutexSdl2() { SDL_DestroyMutex(mpMutex); }

    void lock() { SDL_LockMutex(mpMutex); }

    void unlock() { SDL_UnlockMutex(mpMutex); }

    bool tryLock() {
        int r = SDL_TryLockMutex(mpMutex);
        return r == 0;
    }
}; // class ImpMutexSdl2

using Mutex = ImpMutexSdl2;
#else // #if QD_USE_SDL

//------------------------------------------------------------------------
class ImpMutexPosix
{
    std::mutex m_mutex;

public:
    ImpMutexPosix() = default;

    ImpMutexPosix(const ImpMutexPosix&) = delete;
    void operator= (const ImpMutexPosix&) = delete;

    ~ImpMutexPosix() = default;

    void lock() { m_mutex.lock(); }

    void unlock() { m_mutex.unlock(); }

    bool tryLock() { return m_mutex.try_lock(); }
}; // class ImpMutexPosix
using Mutex = ImpMutexPosix;
#endif // #if QD_USE_SDL
//////////////////////////////////////////////////////////////////////////


class DummyMutex
{
public:
    void lock() {}
    bool tryLock() { return true; }
    void unlock() {}
}; // class DummyMutex
//////////////////////////////////////////////////////////////////////////


template<class TMutex>
class MutexLock
{
    std::remove_const_t<TMutex>* m_pMutex;

public:
    explicit MutexLock(TMutex& mutex)
        : m_pMutex(const_cast<std::remove_const_t<TMutex>*>(&mutex)) {
        m_pMutex->lock();
    }
    MutexLock(MutexLock&& rh) noexcept
        : m_pMutex(rh.m_pMutex) {
        rh.m_pMutex = nullptr;
    }
    ~MutexLock() {
        if (m_pMutex)
            m_pMutex->unlock();
    }
    MutexLock() = delete;
    MutexLock(const MutexLock&) = delete;
    MutexLock& operator= (const MutexLock&) = delete;

}; // class MutexLock
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
template<class TMutex>
class Locker_
{
    std::remove_const_t<TMutex>* m_pMutex;

public:
    inline Locker_(TMutex& Mutex, bool bLockNow = true)
        : m_pMutex(const_cast<std::remove_const_t<TMutex>*>(&Mutex)) {
        if (bLockNow)
            m_pMutex->lock();
    }
    Locker_(Locker_&& rh) noexcept
        : m_pMutex(rh.m_pMutex) {
        rh.m_pMutex = nullptr;
    }

    inline ~Locker_() {
        if (m_pMutex)
            m_pMutex->unlock();
    }

    Locker_(const Locker_&) = delete;
    Locker_& operator= (const Locker_&) = delete;
}; // class CLocker_
   //////////////////////////////////////////////////////////////////////////

}; // namespace qd
