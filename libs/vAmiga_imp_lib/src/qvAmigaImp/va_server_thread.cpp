#include "va_server_thread.h"

#include <Media/RomFiles/RomFile.h>
#include <SDL.h>
#include <VAmiga.h>

#include <queue>

#include "SDL_log.h"
#include "qd/debug/assert.h"
#include "qd/log/log.h"
#include "qd/thread/thread.h"
#include "quasar_app/qsr_config.h"
#include "quasar_app/quaesar.h"
#include "va_server_app_part.h"
#include "va_vm_imp.h"

namespace vamiga {
// Workaround to forward declare 'typedef struct vamiga::Message'
struct MessageFwd : vamiga::Message {};

};  // namespace vamiga

class VAmConsoleQueue {
public:
    std::queue<qtd::string> m_consoleCmdQueue;
    qd::ThreadEvent *m_pThreadEvent;
    qd::Mutex *m_pMutex;

public:
    VAmConsoleQueue() {
        m_pThreadEvent = new qd::ThreadEvent(true);
        m_pMutex = new qd::Mutex();
    }

    void addCmdToQueue(qtd::string cmd) {
        if (cmd.empty()) return;
        m_pMutex->lock();
        m_consoleCmdQueue.push(std::move(cmd));
        m_pMutex->unlock();
        m_pThreadEvent->set();
    }

    bool popConsoleCmdWait(qtd::string &out_cmd) {
        m_pThreadEvent->wait(100);
        qd::MutexLock ml(*m_pMutex);
        if (m_consoleCmdQueue.empty()) return false;
        const qtd::string &cmd = m_consoleCmdQueue.front();
        out_cmd = std::move(cmd);
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

    ~VAmConsoleQueue() { destroy(); }

};  // class VAmConsoleQueue
//////////////////////////////////////////////////////////////////////////

static int vamiga_thread_main_func(void *pThreadData) {
    VAmServerThread *pThis = static_cast<VAmServerThread *>(pThreadData);
    pThis->onVAmigaThreadMain();
    return pThis->m_threadErr;
}

VAmServerThread::VAmServerThread(qsr::VAmServerAppPart *pServerApp)
    : m_pServerApp(pServerApp) {
    assert(!g_pSingleton);
    g_pSingleton = this;
    m_pConsoleQueue = new VAmConsoleQueue();

    // Visible screen dimensions after extracting from the raw 912x313 texture.
    // vAmiga's raw texture includes hblank/vblank areas filled with a checkerboard.
    // We only copy the visible portion, matching what UAE produces.
    // xstart = (HBLANK_MAX + 1) * 4, ystart = VBLANK_MAX + 1 (PAL)
    m_scrWidth = vamiga::HPIXELS - (vamiga::HBLANK_MAX + 1) * 4;   // 912 - 144 = 768
    m_scrHeight = (vamiga::VPIXELS - 2 - (vamiga::PAL::VBLANK_MAX + 1));  // ~285
    m_pAmigaBuffer = new uint32_t[m_scrWidth * m_scrHeight];
    SDL_AtomicSet(&m_scrFrameNo, 0);
}

void VAmServerThread::initialize() {
  // m_pAmigaBuffer and screen dimensions are set up in the constructor.
  // start VAMIGA Thread
    m_onVAmInitialized = new qd::ThreadEvent(true);
    m_uaeThread =
        SDL_CreateThread(&vamiga_thread_main_func, "VAMIGA emulator", this);

    // wait VAMIGA initialization
    m_onVAmInitialized->wait();
    if (m_threadErr >= 0) return;
    // initialize after VAMIGA is ready
    m_pVm->init();
}

void VAmServerThread::destroy() {
    if (m_pConsoleQueue) {
        qd::logDbg("Waiting VAMIGA thread over ...");

        // Signal the thread loop to exit. The loop checks m_bRequestToQuit
        // at the top of each iteration (worst-case latency: one SDL_Delay(5)
        // = 5ms). The old code used execConsoleCmd("q") but the thread loop
        // never read from the console queue, so the signal was lost and
        // SDL_WaitThread hung indefinitely.
        m_bRequestToQuit = true;

        // Stop vAmiga's internal emulator thread so it stops producing
        // frames. This ensures the thread loop's texture-poll path exits
        // quickly instead of blocking on getTexture()/lockTexture().
        if (m_pVAmiga)
            m_pVAmiga->powerOff();

        // wait VAMIGA done
        SDL_WaitThread(m_uaeThread, nullptr);

        if (m_pConsoleQueue) m_pConsoleQueue->destroy();
        SAFE_DELETE(m_pConsoleQueue);
        SAFE_DELETE(m_onVAmInitialized);
    }
    delete[] m_pAmigaBuffer;
    m_pAmigaBuffer = nullptr;
}

VAmServerThread::~VAmServerThread() {
    destroy();
    assert(g_pSingleton == this);
    g_pSingleton = nullptr;
}

void VAmServerThread::setVAmInitialized(bool) {
    ASSERT_AND_DO(m_onVAmInitialized, return);
}

int VAmServerThread::getScrFrameNo() {
    return (int)SDL_AtomicGet(&m_scrFrameNo);
}

void VAmServerThread::pushSdlEvent(const SDL_Event &event) {
    qd::MutexLock ml(m_eventMutex);
    m_sdlEventsQueue.push_back(event);
}

void VAmServerThread::pushOperationMsg(
    qtd::unique_ptr<qd::operation::BaseOpArgs> args) {
    qd::MutexLock ml(m_eventMutex);
    m_pClientOpsStack.push_back(std::move(args));
}

bool VAmServerThread::onVAmHandleEvents() {
    qd::MutexLock ml(m_eventMutex);
    while (!m_sdlEventsQueue.empty()) {
        const SDL_Event &event = m_sdlEventsQueue.front();
        applySdlEventProc(event);
        m_sdlEventsQueue.pop_front();
    }

    while (!m_pClientOpsStack.empty()) {
        qd::operation::BaseOpArgs *pCurOpMsg = m_pClientOpsStack.front().get();
        m_pVm->applyOperationMsgProc(pCurOpMsg);
        m_pClientOpsStack.pop_front();
    }
    return false;
}

IVm::VM *VAmServerThread::getVm() const { return m_pVm; }

void VAmServerThread::applySdlEventProc(const SDL_Event &event) {
    switch (event.type) {
        case SDL_KEYDOWN:
        {
// const SDL_Keycode scancode = event.key.keysym.scancode;
//              const int keyboard = 0;
//              const bool newstate = true;
//              const bool alwaysrelease = true;
//              inputdevice_translatekeycode(keyboard, scancode, newstate,
//              alwaysrelease);
        } break;
        case SDL_KEYUP:
        {
// const SDL_Keycode scancode = event.key.keysym.scancode;
//              const int keyboard = 0;
//              const bool newstate = false;
//              const bool alwaysrelease = false;
//              inputdevice_translatekeycode(keyboard, scancode, newstate,
//              alwaysrelease);
        } break;
        default:
        break;
    }
}

void VAmServerThread::applyImmediateConsoleCmd(qtd::string &&cmd) {
  // amD::uae::do_console_cmd_immediate(cmd.c_str());
}

void VAmServerThread::fetchScreenBufferToTexture(
    const uint32_t *pCurDisplayTexBuf, bool lof) {
    using namespace vamiga;

#if 0
    //  Visible area
    constexpr int xstart = (HBLANK_MAX + 1) * 4;
    constexpr int xend = HPIXELS;
    constexpr int ystart = 0x1B;
    constexpr int yend = 0x137;
    static_assert(xend - xstart <= HPIXELS);
    static_assert(yend - ystart <= VPIXELS);
    constexpr int audio_sample_rate = 48000;
    constexpr int screen_width = xend - xstart;
    constexpr int screen_height = 2 * (yend - ystart);

    uint32_t *destScrBuf = m_pAmigaBuffer;
    std::memcpy(destScrBuf, ptr, HPIXELS * VPIXELS * sizeof(uint32_t));

    void *pixels;
    int pitch;
    if (SDL_LockTexture(mTexture, nullptr, &pixels, &pitch))
        SDL_Log("SDL_LockTexture ERROR");
    uint8_t *dest1 = reinterpret_cast<uint8_t *>(pixels) + !lof * pitch;
    uint8_t *dest2 = reinterpret_cast<uint8_t *>(pixels) + lof * pitch;
    const uint32_t *src1 = destScrBuf;
    const uint32_t *src2 = (lof == last_frame_type_) ? destScrBuf : &last_frame_[0];

    src1 += HPIXELS * ystart + HBLANK_MAX * 4;  // xstart;
    src2 += HPIXELS * ystart + HBLANK_MAX * 4;  // xstart;
    for (uint32_t y = 0; y < screen_height / 2; ++y) {
        std::memcpy(dest1, src1, screen_width * sizeof(uint32_t));
        std::memcpy(dest2, src2, screen_width * sizeof(uint32_t));
        dest1 += 2 * pitch;
        dest2 += 2 * pitch;
        src1 += HPIXELS;
        src2 += HPIXELS;
    }
    SDL_UnlockTexture(mTexture);

    //std::swap(current_frame_, last_frame_);
    last_frame_type_ = lof;
#endif  //
}

void VAmServerThread::execConsoleCmd(qtd::string &&cmd) {
    m_pConsoleQueue->addCmdToQueue(std::move(cmd));
}

int VAmServerThread::uaeWaitConsoleCmdImpl(char *out, int maxlen) { return -1; }

void VAmServerThread::onVAmigaThreadMain() {
    try {
      // using namespace vamiga;
        logDbg("VAmigaServerThread: Initializing...");

        m_pVAmiga = new vamiga::VAmiga();
        m_pVAmiga->set(vamiga::ConfigScheme::A500_OCS_1MB);

        auto vaimga_delegate_cb = [](const void *ptr, vamiga::Message in_msg) {
            auto pThis = reinterpret_cast<VAmServerThread *>(const_cast<void *>(ptr));
            const auto &msg = static_cast<vamiga::MessageFwd &>(in_msg);
            pThis->vAmigaMsgQueueProc(msg);
            };
        m_pVAmiga->launch(this, vaimga_delegate_cb);

        if (!g_cfg_startup.kickRomPath.empty()) {
            logDbg("VAMIGA: Loading Kick.rom from '%s' ...",
                g_cfg_startup.kickRomPath.c_str());
            try {
                vamiga::RomFile rom {g_cfg_startup.kickRomPath.c_str()};
                m_pVAmiga->mem.loadRom(rom);
            }
            catch (...) {
                m_threadErrStr = logErr("VAmiga load kickRom:'%s' ERROR!",
                    g_cfg_startup.kickRomPath
                    .c_str()) /*->ASSERT_DLG()*/->getLogStr();
                m_threadErr = 1;
                SAFE_DELETE(m_pVAmiga);
                m_onVAmInitialized->set();  // sync with main thread
                // Fall through to cleanup below (m_pVAmiga is already nullptr)
            }
        }
        else
            logWarn("VAMIGA: Loading Kick.rom not set - skipped");

        // Skip initialization if ROM load failed (m_pVAmiga was deleted)
        if (m_pVAmiga) {
            m_pVAmiga->powerOn();
            m_pVAmiga->run();

            m_pVm = new IVm::imp::VAmVmImp(this, m_pVAmiga);

            setVAmInitialized(true);
            m_onVAmInitialized->set();  // sync with main thread

            for (;;) {
                if (m_bRequestToQuit) break;

                // Process debugger operations and keyboard events
                onVAmHandleEvents();

                vamiga::VAmiga *pVAmiga = m_pVAmiga;
                vamiga::VideoPortAPI &vVideoPort = pVAmiga->videoPort;
                bool lof, prevlof;
                vamiga::isize nr;

                vVideoPort.lockTexture();
                const uint32_t *pCurDisplayTexBuf =
                    vVideoPort.getTexture(&nr, &lof, &prevlof);
                if (getScrFrameNo() == (int)nr) {
                    vVideoPort.unlockTexture();
                    SDL_Delay(5);
                }
                else {
                    copyVisibleArea(pCurDisplayTexBuf, vamiga::HPIXELS, vamiga::VPIXELS, lof);
                    SDL_AtomicSet(&m_scrFrameNo, (int)nr);
                    vVideoPort.unlockTexture();
                    pVAmiga->wakeUp();
                }
            }
        }
    }
    catch (const std::exception &ex) {
        char const *errStr = ex.what();
        qd::logErr("VAmiga threadInit exception: '%s'", errStr).ASSERT_DLG();
        m_threadErrStr = ex.what();
        m_threadErr = 1;
        //assert2(0, "VAmiga thread Exception: '%s'", ex.what());
    }
    // --- Cleanup (reached on both normal loop exit and exception) ---
    // Halt vAmiga's internal emulator thread and delete instances.
    // m_pVAmiga and m_pVm were created in this thread; they must be
    // destroyed here, not in destroy() (which runs on the main thread).
    // halt() is idempotent — safe even if powerOff() already stopped it.
    if (m_pVAmiga) {
        m_pVAmiga->halt();   // sends HALT cmd + joins internal thread
        SAFE_DELETE(m_pVAmiga);
    }
    m_pVm = nullptr;  // release ref_ptr

    qd::logDbg("VAmigaServerThread: Done...");
}

void VAmServerThread::copyVisibleArea(const uint32_t* pSrc, int rawWidth,
                                       int rawHeight, bool lof) {
    using namespace vamiga;

    // Visible area boundaries in the raw texture
    const int xstart = (HBLANK_MAX + 1) * 4;       // 144
    const int ystart = PAL::VBLANK_MAX + 1;          // 26
    const int yend   = rawHeight - 2;                // 311

    const int visWidth  = rawWidth - xstart;          // 768
    const int visHeight = yend - ystart;             // 285

    m_VAmScrTextureMutex.lock();

    // Reallocate if dimensions changed (e.g. PAL <-> NTSC switch)
    if (m_scrWidth != visWidth || m_scrHeight != visHeight) {
        delete[] m_pAmigaBuffer;
        m_scrWidth = visWidth;
        m_scrHeight = visHeight;
        m_pAmigaBuffer = new uint32_t[m_scrWidth * m_scrHeight];
    }

    // Plain memcpy per scanline — no per-pixel R/B swap.
    // SDL textures use ABGR8888 (matching vAmiga's native format), so the
    // GPU handles channel conversion for free.
    const size_t rowBytes = (size_t)visWidth * sizeof(uint32_t);
    for (int y = 0; y < visHeight; y++) {
        memcpy(m_pAmigaBuffer + y * visWidth,
               pSrc + (ystart + y) * rawWidth + xstart,
               rowBytes);
    }

    m_VAmScrTextureMutex.unlock();
}


bool VAmServerThread::lockDisplayTexBuf(int *out_width, int *out_height,
                                        uint32_t **out_pixels) {
    m_VAmScrTextureMutex.lock();
    if (!m_pAmigaBuffer) {
        m_VAmScrTextureMutex.unlock();
        return false;
    }
    *out_width = m_scrWidth;
    *out_height = m_scrHeight;
    *out_pixels = m_pAmigaBuffer;
    return true;
}

void VAmServerThread::unlockDisplayTexBuf() { m_VAmScrTextureMutex.unlock(); }


void VAmServerThread::vAmigaMsgQueueProc(const vamiga::MessageFwd &msg) {

    using namespace vamiga;
    switch (msg.type) {
        case Msg::RSH_UPDATE:
        case Msg::DRIVE_SELECT:
        case Msg::DRIVE_STEP:
        case Msg::DRIVE_POLL:
        case Msg::DISK_INSERT:
        case Msg::DISK_EJECT:
        case Msg::DRIVE_LED:
        case Msg::DRIVE_MOTOR:
        case Msg::SER_IN:
        case Msg::HDR_READ:
        case Msg::HDR_WRITE:
        case Msg::HDR_IDLE:
        case Msg::HDR_STEP:
        case Msg::HDC_STATE:
        case Msg::HDC_CONNECT:
        case Msg::VIEWPORT:
        case Msg::CONFIG:
        case Msg::POWER_LED_ON:
        case Msg::POWER_LED_OFF:
        case Msg::POWER_LED_DIM:
        case Msg::DRIVE_CONNECT:
        case Msg::MEM_LAYOUT:
        case Msg::OVERCLOCKING:
        case Msg::VIDEO_FORMAT:
        case Msg::DMA_DEBUG:
        case Msg::MUTE:
        case Msg::RUN:
        case Msg::PAUSE:
        case Msg::RESET:
        return;
        case Msg::ABORT:
        m_bRequestToQuit = true;
        power_is_on_ = false;
        break;
        case Msg::POWER:
        if (msg.value) {
            power_is_on_ = true;
        }
        else {
            power_is_on_ = false;
            // std::memset(&current_frame_[0], 0, sizeof(uint32_t) *
            // current_frame_.size()); std::memset(&last_frame_[0], 0,
            // sizeof(uint32_t) * last_frame_.size());
        }
        return;

        case Msg::RECORDING_STOPPED:
        break;
        default:
        break;
    }
    //     std::cerr << "MsgQueue: type=" << (long)msg->type << "(" <<
    //     MsgEnum::key(msg->type) << ") value=" << msg->value
    //               << "\n";
}
