#pragma once
#include <SDL.h>
#include <deque>
#include "qd/base/base.h"
#include "qd/thread/thread.h"


FORWARD_DECLARATION_2(qd, ThreadEvent);


//////////////////////////////////////////////////////////////////////////
// UAE work-thread
//
class UaeWorker {
    struct SDL_Thread* m_uaeThread = nullptr;  // start UAE in separate thread
    inline static UaeWorker* g_pSingleton = nullptr;
    qd::Mutex m_eventMutex;
    std::deque<SDL_Event> m_eventQueue;

public:
    int m_scrWidth = 754;
    int m_scrHeight = 576;
    qd::Mutex m_UaeScrTextureMutex;
    uint32_t* m_pAmigaBuffer = nullptr;
    qd::ThreadEvent* m_onUaeInitialized = nullptr;  // event to wait for UAE initialization
    SDL_atomic_t m_scrFrameNo = {};

public:
    UaeWorker();
    ~UaeWorker();
    void initialize();
    void destroy();
    void setUaeInitialized(bool);

    uint32_t* lockUaeScreenTexBuf(int amiga_width, int amiga_height);
    void unlockUaeScreenTexBuf();
    int getScrFrameNo();

    void pushSdlEvent(const SDL_Event& event);

    bool onUaeHandleEvents();

public:
    static UaeWorker* get() {
        return g_pSingleton;
    }

protected:
    void onSdlEventProc(const SDL_Event& event);

};  // class UaeWorker
//////////////////////////////////////////////////////////////////////////
