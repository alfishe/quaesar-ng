// clang-format off
#include "sysconfig.h"
#include "sysdeps.h"
#include "uae/time.h"
#include "options.h"
#include "adf.h"
#include "uae.h"
#include "inputdevice.h"
// clang-format on

#include "uae_server_thread.h"
#include <SDL.h>
#include <queue>
#include "qd/debug/assert.h"
#include "qd/log/log.h"
#include "qd/thread/thread.h"
#include "qsr_config.h"
#include "quasar_app/quaesar.h"
#include "uae_server_app_part.h"
#include "uae_vm_imp.h"


extern void real_main(int argc, TCHAR** argv);
extern void qs_keyboard_set_translation();
extern void quae__parseCmdLine(int argc, TCHAR** argv);

namespace amD::uae {
extern void do_console_cmd_immediate(const char* cmd);
};  //namespace amD::uae


class UaeConsoleQueue {
public:
    std::queue<qd::string> m_consoleCmdQueue;
    qd::ThreadEvent* m_pThreadEvent;
    qd::Mutex* m_pMutex;

public:
    UaeConsoleQueue() {
        m_pThreadEvent = new qd::ThreadEvent(true);
        m_pMutex = new qd::Mutex();
    }

    void addCmdToQueue(eastl::string cmd) {
        if (cmd.empty())
            return;
        m_pMutex->lock();
        m_consoleCmdQueue.push(eastl::move(cmd));
        m_pMutex->unlock();
        m_pThreadEvent->set();
    }

    bool popConsoleCmdWait(eastl::string& out_cmd) {
        m_pThreadEvent->wait(100);
        qd::MutexLock ml(*m_pMutex);
        if (m_consoleCmdQueue.empty())
            return false;
        const eastl::string& cmd = m_consoleCmdQueue.front();
        out_cmd = eastl::move(cmd);
        m_consoleCmdQueue.pop();
        return true;
    }

    void destroy() {
        m_consoleCmdQueue = {};
        if (m_pThreadEvent) {
            m_pThreadEvent->set();
            SAFE_DELETE(m_pThreadEvent);
        }
        SAFE_DELETE(m_pMutex);
    }

    ~UaeConsoleQueue() {
        destroy();
    }

};  // class UaeConsoleQueue
//////////////////////////////////////////////////////////////////////////


static int uae_thread_main_func(void*) {
    std::vector<const char*> argv;
    argv.push_back("quaesar.exe");
    argv.reserve(g_cfg_startup->uaeExtArgs.size() * 2 + 1);
    // pass remain Quaesar CLI args to UAE
    for (const auto& s : g_cfg_startup->uaeExtArgs) {
        argv.push_back("-s");
        argv.push_back(s.c_str());
    }
    quae__parseCmdLine((int)argv.size(), const_cast<char**>(&argv[0]));

    ::real_main(0, nullptr);  // call main function of UAE emulator
    return 0;
}


UaeServerThread::UaeServerThread(qsr::UaeServerAppPart* pServerApp) : m_pServerApp(pServerApp) {
    m_pVm = new IVm::imp::UaeVmImp();
    m_pVm->setServerImp(this);

    assert(!g_pSingleton);
    g_pSingleton = this;
    m_pConsoleQueue = new UaeConsoleQueue();
}


void UaeServerThread::initialize() {
    ::syncbase = 1000000;
    qs_keyboard_set_translation();
    ::default_prefs(&::currprefs, true, 0);
    ::fixup_prefs(&::currprefs, true);

    //const CfgQsrStartup& options = ;
    if (!g_cfg_startup->input.empty()) {
        if (qd::ends_with(g_cfg_startup->input, ".exe") || !qd::ends_with(g_cfg_startup->input, ".adf")) {
            if (FILE* check_file = fopen(g_cfg_startup->input.c_str(), "rb")) {
                fclose(check_file);
                Adf::create_for_exefile(g_cfg_startup->input.c_str());
                strcpy(::currprefs.floppyslots[0].df, "dummy.adf");
            } else {
                SDL_Log("can't open input file:'%s'", g_cfg_startup->input.c_str());
            }
        } else {
            strcpy(::currprefs.floppyslots[0].df, g_cfg_startup->input.c_str());
        }
    }

    if (!g_cfg_startup->serialPort.empty()) {
        currprefs.use_serial = 1;
        strcpy(currprefs.sername, g_cfg_startup->serialPort.c_str());
    }

    // Most compatible mode
    currprefs.cpu_cycle_exact = true;
    currprefs.cpu_memory_cycle_exact = true;
    currprefs.blitter_cycle_exact = true;
    currprefs.floppy_speed = 100;
    //    currprefs.turbo_emulation = 1; // it disables sound
    currprefs.sound_stereo_separation = 0;
    currprefs.uaeboard = 1;
    currprefs.win32_filesystem_mangle_reserved_names = true;  // required for FS
    currprefs.filesys_custom_uaefsdb = false;                 // hack to not implement 'custom_fsdb_*' funcs now

    strcpy(currprefs.romfile, g_cfg_startup->kickRomPath.c_str());

    m_scrWidth = 754;
    m_scrHeight = 576;
    assert(!m_pAmigaBuffer);
    m_pAmigaBuffer = new uint32_t[m_scrWidth * m_scrHeight];
    SDL_AtomicSet(&m_scrFrameNo, 0);

    // start UAE Thread
    m_onUaeInitialized = new qd::ThreadEvent(true);
    m_uaeThread = SDL_CreateThread(&uae_thread_main_func, "UAE emulator", nullptr);

    // wait UAE initialization
    m_onUaeInitialized->wait();

    // initialize after UAE is ready
    m_pVm->init();
}


void UaeServerThread::destroy() {
    if (m_pConsoleQueue) {
        qd::logInfo("Waiting UAE thread over ...");
        execConsoleCmd("q");
        // wait UAE done
        SDL_WaitThread(m_uaeThread, nullptr);

        if (m_pConsoleQueue)
            m_pConsoleQueue->destroy();
        SAFE_DELETE(m_pConsoleQueue);
        SAFE_DELETE(m_onUaeInitialized);
    }
    delete[] m_pAmigaBuffer;
    m_pAmigaBuffer = nullptr;
}


UaeServerThread::~UaeServerThread() {
    destroy();
    assert(g_pSingleton == this);
    g_pSingleton = nullptr;
}


void UaeServerThread::setUaeInitialized(bool) {
    ASSERT_AND_DO(m_onUaeInitialized, return);
    m_onUaeInitialized->set();
}


uint32_t* UaeServerThread::_lockUaeScreenTexBuf(int amiga_width, int amiga_height) {
    //
    m_UaeScrTextureMutex.lock();
    if (amiga_width != m_scrWidth || amiga_height != m_scrHeight) {
        delete[] m_pAmigaBuffer;
        m_scrWidth = amiga_width;
        m_scrHeight = amiga_height;
        m_pAmigaBuffer = new uint32_t[m_scrWidth * m_scrHeight];
    }
    return m_pAmigaBuffer;
}


void UaeServerThread::_unlockUaeScreenTexBuf() {
    m_UaeScrTextureMutex.unlock();
    SDL_AtomicIncRef(&m_scrFrameNo);
}


int UaeServerThread::getScrFrameNo() {
    return (int)SDL_AtomicGet(&m_scrFrameNo);
}


void UaeServerThread::pushSdlEvent(const SDL_Event& event) {
    qd::MutexLock ml(m_eventMutex);
    m_sdlEventsQueue.push_back(event);
}


void UaeServerThread::pushOperationMsg(qd::unique_ptr<qd::operation::BaseOpArgs> args) {
    qd::MutexLock ml(m_eventMutex);
    m_pClientOpsStack.push_back(std::move(args));
}


bool UaeServerThread::onUaeHandleEvents() {
    qd::MutexLock ml(m_eventMutex);
    while (!m_sdlEventsQueue.empty()) {
        const SDL_Event& event = m_sdlEventsQueue.front();
        applySdlEventProc(event);
        m_sdlEventsQueue.pop_front();
    }

    while (!m_pClientOpsStack.empty()) {
        qd::operation::BaseOpArgs* pCurOpMsg = m_pClientOpsStack.front().get();
        m_pVm->applyOperationMsgProc(pCurOpMsg);
        m_pClientOpsStack.pop_front();
    }
    return false;
}


IVm::VM* UaeServerThread::getVm() const {
    return m_pVm;
}


bool UaeServerThread::lockDisplayTexBuf(int* width, int* height, uint32_t** out_pixels) {
    if (!m_UaeScrTextureMutex.tryLock())
        return false;
    if (!m_pAmigaBuffer) {
        unlockDisplayTexBuf();
        return false;
    }
    *width = m_scrWidth;
    *height = m_scrHeight;
    *out_pixels = m_pAmigaBuffer;
    return true;
}


void UaeServerThread::unlockDisplayTexBuf() {
    m_UaeScrTextureMutex.unlock();
}


void UaeServerThread::applySdlEventProc(const SDL_Event& event) {
    switch (event.type) {
        case SDL_KEYDOWN: {
            const SDL_Keycode scancode = event.key.keysym.scancode;
            const int keyboard = 0;
            const bool newstate = true;
            const bool alwaysrelease = true;
            inputdevice_translatekeycode(keyboard, scancode, newstate, alwaysrelease);
        } break;
        case SDL_KEYUP: {
            const SDL_Keycode scancode = event.key.keysym.scancode;
            const int keyboard = 0;
            const bool newstate = false;
            const bool alwaysrelease = false;
            inputdevice_translatekeycode(keyboard, scancode, newstate, alwaysrelease);
        } break;
        default:
            break;
    }
}


void UaeServerThread::applyImmediateConsoleCmd(qd::string&& cmd) {
    amD::uae::do_console_cmd_immediate(cmd.c_str());
}


void UaeServerThread::execConsoleCmd(qd::string&& cmd) {
    m_pConsoleQueue->addCmdToQueue(std::move(cmd));
}


int UaeServerThread::uaeWaitConsoleCmdImpl(char* out, int maxlen) {
    eastl::string cmd;
    if (!m_pConsoleQueue->popConsoleCmdWait(cmd))
        return -1;

    const int len = (int)cmd.size();
    if (len < maxlen)
        strcpy(out, cmd.data());
    else
        EASTL_ASSERT(0);
    return len;
}
