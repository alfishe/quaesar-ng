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
#include "qd/thread/thread.h"
#include "quasar_app/parse_options.h"
#include "quasar_app/quaesar.h"
#include "uae_server_app_part.h"
#include "uae_vm_imp.h"


extern void real_main(int argc, TCHAR** argv);
extern void qs_keyboard_set_translation();

namespace amD {
extern void quae_parse_cmdline(int argc, TCHAR** argv);
};  //namespace amD


class ConsoleQueue {
public:
    std::queue<qd::string> m_consoleCmdQueue;
    qd::ThreadEvent* m_pThreadEvent;
    qd::Mutex* m_pMutex;

public:
    ConsoleQueue() {
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

    bool waitConsoleCmd(eastl::string& out) {
        m_pThreadEvent->wait(100);
        qd::MutexLock ml(*m_pMutex);
        if (m_consoleCmdQueue.empty())
            return false;
        const eastl::string& cmd = m_consoleCmdQueue.front();
        out = eastl::move(cmd);
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

    ~ConsoleQueue() {
        destroy();
    }

};  // class ConsoleQueue
//////////////////////////////////////////////////////////////////////////


int uae_thread_main_func(void*) {
    std::vector<const char*> argv;
    argv.push_back("quaesar.exe");
    argv.reserve(g_initOptions.uaeExtArgs.size() * 2 + 1);
    // pass remain Quaesar CLI args to UAE
    for (const std::string& s : g_initOptions.uaeExtArgs) {
        argv.push_back("-s");
        argv.push_back(s.c_str());
    }
    amD::quae_parse_cmdline((int)argv.size(), const_cast<char**>(&argv[0]));

    ::real_main(0, nullptr);  // call main function of UAE emulator
    return 0;
}


void* UaeServerThread::getOpEnvPtr(const qd::TypeInfo& classType) const {
    assert(0);
    return nullptr;
}


qd::EFlow UaeServerThread::applyOperationMsgProc(qd::operation::args::Base* args) {
    assert(0);
    return qd::EFlow::STOP;
}


UaeServerThread::UaeServerThread(qsr::UaeServerAppPart* pServerApp) : m_pServerApp(pServerApp) {
    m_pVm = new amD::vm::imp::UaeVmImp();
    m_pVm->setServerImp(this);

    assert(!g_pSingleton);
    g_pSingleton = this;
    m_pConsoleQueue = new ConsoleQueue();
}


void UaeServerThread::initialize() {
    ::syncbase = 1000000;
    qs_keyboard_set_translation();
    ::default_prefs(&::currprefs, true, 0);
    ::fixup_prefs(&::currprefs, true);

    const QuaesarOptions& options = g_initOptions;
    if (!options.input.empty()) {
        // TODO: cleanup
        if (qd::ends_with(options.input, ".exe") || !qd::ends_with(options.input, ".adf")) {
            if (FILE* check_file = fopen(options.input.c_str(), "rb")) {
                fclose(check_file);
                Adf::create_for_exefile(options.input.c_str());
                strcpy(::currprefs.floppyslots[0].df, "dummy.adf");
            } else {
                SDL_Log("can't open input file:'%s'", options.input.c_str());
            }
        } else {
            strcpy(::currprefs.floppyslots[0].df, options.input.c_str());
        }
    }

    if (!options.serial_port.empty()) {
        currprefs.use_serial = 1;
        strcpy(currprefs.sername, options.serial_port.c_str());
    }

    // Most compatible mode
    currprefs.cpu_cycle_exact = 1;
    currprefs.cpu_memory_cycle_exact = 1;
    currprefs.blitter_cycle_exact = 1;
    currprefs.floppy_speed = 100;
    //    currprefs.turbo_emulation = 1; // it disables sound
    currprefs.sound_stereo_separation = 0;
    currprefs.uaeboard = 1;
    currprefs.win32_filesystem_mangle_reserved_names = true;  // required for FS
    currprefs.filesys_custom_uaefsdb = false;                 // hack to not implement 'custom_fsdb_*' funcs now

    strcpy(currprefs.romfile, options.kickstart.c_str());


    m_scrWidth = 754;
    m_scrHeight = 576;
    assert(!m_pAmigaBuffer);
    m_pAmigaBuffer = new uint32_t[m_scrWidth * m_scrHeight];
    SDL_AtomicSet(&m_scrFrameNo, 0);

    // wait UAE initialization
    m_onUaeInitialized = new qd::ThreadEvent(true);
    m_uaeThread = SDL_CreateThread(&uae_thread_main_func, "UAE emulator", nullptr);
    m_onUaeInitialized->wait();
    // inite after UAE ready
    m_pVm->init();
}


void UaeServerThread::destroy() {
    SDL_Log("Waiting UAE thread over ...");
    //g_pApp->m_pDebuggerPart->execConsoleCmd("q");

    if (m_pConsoleQueue)
        m_pConsoleQueue->destroy();
    SAFE_DELETE(m_pConsoleQueue);

    // wait UAE done
    SDL_WaitThread(m_uaeThread, nullptr);
    SAFE_DELETE(m_onUaeInitialized);
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


uint32_t* UaeServerThread::lockUaeScreenTexBuf(int amiga_width, int amiga_height) {
    m_UaeScrTextureMutex.lock();

    if (amiga_width > m_scrWidth || amiga_height > m_scrHeight) {
        delete[] m_pAmigaBuffer;
        m_pAmigaBuffer = new uint32_t[m_scrWidth * m_scrHeight];
    }
    m_scrWidth = amiga_width;
    m_scrHeight = amiga_height;
    return m_pAmigaBuffer;
}


void UaeServerThread::unlockUaeScreenTexBuf() {
    m_UaeScrTextureMutex.unlock();
    SDL_AtomicIncRef(&m_scrFrameNo);
}


int UaeServerThread::getScrFrameNo() {
    return SDL_AtomicGet(&m_scrFrameNo);
}


void UaeServerThread::pushSdlEvent(const SDL_Event& event) {
    qd::MutexLock ml(m_eventMutex);
    m_eventQueue.push_back(std::move(event));
}


bool UaeServerThread::onUaeHandleEvents() {
    qd::MutexLock ml(m_eventMutex);
    if (m_eventQueue.empty())
        return false;
    const SDL_Event& event = m_eventQueue.front();
    onSdlEventProc(event);
    m_eventQueue.pop_front();
    return true;
}


IVm::VM* UaeServerThread::getVm() const {
    return m_pVm;
}


class UaeSharedConnectionImp : public amD::IDbgConnection {
public:
    amD::vm::imp::UaeVmImp* m_pUaeVm;

    UaeSharedConnectionImp(amD::vm::imp::UaeVmImp* pUaeVm) : m_pUaeVm(pUaeVm) {
    }

    virtual void pushOperation(amD::operation::OperationArgs*) override {
    }

    virtual amD::operation::OperationArgs* getFrontOperation() override {
        return nullptr;
    }

    virtual void popFrontOperation() override {
    }

    virtual ref_ptr<IVm::VM> getClientVm() override {
        return m_pUaeVm;
    }

    virtual ref_ptr<IVm::VM> getServerVm() override {
        return m_pUaeVm;
    }
};  // class UaeSharedConnectionImp
//////////////////////////////////////////////////////////////////////////


ref_ptr<amD::IDbgConnection> UaeServerThread::createConnection() const {
    ref_ptr<UaeSharedConnectionImp> pInst = new UaeSharedConnectionImp(m_pVm);
    return pInst;
}


void UaeServerThread::onSdlEventProc(const SDL_Event& event) {
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


int UaeServerThread::waitConsoleCmd(char* out, int maxlen) {
    eastl::string cmd;
    if (!m_pConsoleQueue->waitConsoleCmd(cmd))
        return -1;

    const int len = (int)cmd.size();
    if (len < maxlen)
        strcpy(out, cmd.data());
    else
        EASTL_ASSERT(0);
    return len;
}
