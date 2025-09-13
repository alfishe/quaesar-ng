#pragma once
#include <SDL_mutex.h>
#include <SDL_log.h>


namespace qd {


class Mutex
{
    SDL_mutex* mpMutex;

public:
    Mutex()
    {
        mpMutex = SDL_CreateMutex();
        if (!mpMutex)
        {
            SDL_Log("cannot create mutex");
            return;
        }
    }

    Mutex(const Mutex&) = delete;
    void operator= (const Mutex&) = delete;

    ~Mutex() { SDL_DestroyMutex(mpMutex); }

    void lock() { SDL_LockMutex(mpMutex); }

    void unlock() { SDL_UnlockMutex(mpMutex); }

    bool tryLock()
    {
        int r = SDL_TryLockMutex(mpMutex);
        return r == 0;
    }

}; // class Mutex
//////////////////////////////////////////////////////////////////////////


class DummyMutex
{
public:
    void lock() {}
    bool tryLock() { return true; }
    void unlock() {}
}; // class DummyMutex
//////////////////////////////////////////////////////////////////////////


class MutexLock
{
    Mutex* m_pMutex;

public:
    MutexLock(Mutex& mutex)
        : m_pMutex(&mutex)
    {
        m_pMutex->lock();
    }
    MutexLock(MutexLock&& rh) noexcept
        : m_pMutex(rh.m_pMutex)
    {
        rh.m_pMutex = nullptr;
    }
    ~MutexLock()
    {
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
    TMutex* m_pMutex;

public:
    inline Locker_(TMutex& Mutex, bool bLockNow = true)
        : m_pMutex(&Mutex)
    {
        if (bLockNow)
            m_pMutex->lock();
    }
    Locker_(Locker_&& rh) noexcept
        : m_pMutex(rh.m_pMutex)
    {
        rh.m_pMutex = nullptr;
    }

    inline ~Locker_() { if (m_pMutex) m_pMutex->unlock(); }

    Locker_(const Locker_&) = delete;
    Locker_& operator= (const Locker_&) = delete;
}; // class CLocker_
   //////////////////////////////////////////////////////////////////////////

}; // namespace qd
