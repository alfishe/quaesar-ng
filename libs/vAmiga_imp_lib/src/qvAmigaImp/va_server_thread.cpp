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

// SDL scancode → Amiga raw keycode translation table.
// Amiga keycodes are the raw 7-bit codes sent by the Amiga keyboard controller.
// Bit 7 is set by the Keyboard class on release, so only 0x00–0x7F here.
// Reference: Amiga Hardware Reference Manual, Appendix C.
static const struct { SDL_Scancode sdl; u8 amiga; } s_sdlToAmigaKey[] = {
    // Row 1: function keys
    {SDL_SCANCODE_ESCAPE,        0x45},
    {SDL_SCANCODE_F1,            0x50},
    {SDL_SCANCODE_F2,            0x51},
    {SDL_SCANCODE_F3,            0x52},
    {SDL_SCANCODE_F4,            0x53},
    {SDL_SCANCODE_F5,            0x54},
    {SDL_SCANCODE_F6,            0x55},
    {SDL_SCANCODE_F7,            0x56},
    {SDL_SCANCODE_F8,            0x57},
    {SDL_SCANCODE_F9,            0x58},
    {SDL_SCANCODE_F10,           0x59},

    // Number row
    {SDL_SCANCODE_BACKSPACE,     0x41},  // Backspace (Delete key on Amiga)
    {SDL_SCANCODE_1,             0x01},
    {SDL_SCANCODE_2,             0x02},
    {SDL_SCANCODE_3,             0x03},
    {SDL_SCANCODE_4,             0x04},
    {SDL_SCANCODE_5,             0x05},
    {SDL_SCANCODE_6,             0x06},
    {SDL_SCANCODE_7,             0x07},
    {SDL_SCANCODE_8,             0x08},
    {SDL_SCANCODE_9,             0x09},
    {SDL_SCANCODE_0,             0x0A},
    {SDL_SCANCODE_MINUS,         0x0B},
    {SDL_SCANCODE_EQUALS,        0x0C},

    // Row 2: QWERTY
    {SDL_SCANCODE_TAB,           0x42},
    {SDL_SCANCODE_Q,             0x10},
    {SDL_SCANCODE_W,             0x11},
    {SDL_SCANCODE_E,             0x12},
    {SDL_SCANCODE_R,             0x13},
    {SDL_SCANCODE_T,             0x14},
    {SDL_SCANCODE_Y,             0x15},
    {SDL_SCANCODE_U,             0x16},
    {SDL_SCANCODE_I,             0x17},
    {SDL_SCANCODE_O,             0x18},
    {SDL_SCANCODE_P,             0x19},
    {SDL_SCANCODE_LEFTBRACKET,   0x1A},
    {SDL_SCANCODE_RIGHTBRACKET,  0x1B},

    // Row 3: ASDF
    {SDL_SCANCODE_CAPSLOCK,      0x62},
    {SDL_SCANCODE_A,             0x20},
    {SDL_SCANCODE_S,             0x21},
    {SDL_SCANCODE_D,             0x22},
    {SDL_SCANCODE_F,             0x23},
    {SDL_SCANCODE_G,             0x24},
    {SDL_SCANCODE_H,             0x25},
    {SDL_SCANCODE_J,             0x26},
    {SDL_SCANCODE_K,             0x27},
    {SDL_SCANCODE_L,             0x28},
    {SDL_SCANCODE_SEMICOLON,     0x29},
    {SDL_SCANCODE_APOSTROPHE,    0x2A},
    {SDL_SCANCODE_GRAVE,         0x2F},  // Backquote → help (Amiga key 0x2F)
    {SDL_SCANCODE_RETURN,        0x44},

    // Row 4: ZXCV
    {SDL_SCANCODE_LSHIFT,        0x60},
    {SDL_SCANCODE_Z,             0x31},
    {SDL_SCANCODE_X,             0x32},
    {SDL_SCANCODE_C,             0x33},
    {SDL_SCANCODE_V,             0x34},
    {SDL_SCANCODE_B,             0x35},
    {SDL_SCANCODE_N,             0x36},
    {SDL_SCANCODE_M,             0x37},
    {SDL_SCANCODE_COMMA,         0x38},
    {SDL_SCANCODE_PERIOD,        0x39},
    {SDL_SCANCODE_SLASH,         0x3A},
    {SDL_SCANCODE_RSHIFT,        0x61},

    // Bottom modifiers
    {SDL_SCANCODE_LCTRL,         0x63},  // Ctrl
    {SDL_SCANCODE_LGUI,          0x66},  // Left Amiga
    {SDL_SCANCODE_LALT,          0x64},  // Left Alt
    {SDL_SCANCODE_SPACE,         0x40},
    {SDL_SCANCODE_RALT,          0x65},  // Right Alt
    {SDL_SCANCODE_RGUI,          0x67},  // Right Amiga
    {SDL_SCANCODE_APPLICATION,   0x67},  // Menu key → Right Amiga
    {SDL_SCANCODE_RCTRL,         0x63},  // Right Ctrl → Ctrl

    // Navigation keys
    {SDL_SCANCODE_DELETE,        0x46},  // Del (Delete Forward)
    {SDL_SCANCODE_HELP,          0x5F},  // Help
    {SDL_SCANCODE_UP,            0x4C},
    {SDL_SCANCODE_DOWN,          0x4D},
    {SDL_SCANCODE_RIGHT,         0x4E},
    {SDL_SCANCODE_LEFT,          0x4F},

    // Enter/Return on numpad
    {SDL_SCANCODE_KP_ENTER,      0x43},

    // Less/Greater keys (European layout)
    {SDL_SCANCODE_NONUSBACKSLASH,0x0D},
    {SDL_SCANCODE_BACKSLASH,     0x0D},

    // Numpad
    {SDL_SCANCODE_KP_0,          0x0F},  // KP ( + ) + 0
    {SDL_SCANCODE_KP_1,          0x1D},
    {SDL_SCANCODE_KP_2,          0x1E},
    {SDL_SCANCODE_KP_3,          0x1F},
    {SDL_SCANCODE_KP_4,          0x2D},
    {SDL_SCANCODE_KP_5,          0x2E},
    {SDL_SCANCODE_KP_6,          0x2F},  // KP 6 (same as GRAVE; rare clash)
    {SDL_SCANCODE_KP_7,          0x3D},
    {SDL_SCANCODE_KP_8,          0x3E},
    {SDL_SCANCODE_KP_9,          0x3F},
    {SDL_SCANCODE_KP_PERIOD,     0x3C},
    {SDL_SCANCODE_KP_PLUS,       0x5E},
    {SDL_SCANCODE_KP_MINUS,      0x4A},
    {SDL_SCANCODE_KP_MULTIPLY,   0x5D},
    {SDL_SCANCODE_KP_DIVIDE,     0x5C},
};

static u8 sdlScancodeToAmigaKey(SDL_Scancode sc) {
    for (auto &e : s_sdlToAmigaKey) {
        if (e.sdl == sc) return e.amiga;
    }
    return 0xFF; // invalid / unmapped
}
} // namespace vamiga

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
    if (!m_pVAmiga) return;

    switch (event.type) {
        case SDL_KEYDOWN:
        case SDL_KEYUP: {
            if (event.key.repeat) break;
            vamiga::u8 amigaKey = vamiga::sdlScancodeToAmigaKey(event.key.keysym.scancode);
            if (amigaKey != 0xFF) {
                if (event.type == SDL_KEYDOWN)
                    m_pVAmiga->keyboard.press(amigaKey);
                else
                    m_pVAmiga->keyboard.release(amigaKey);
            }
        } break;
        case SDL_MOUSEMOTION: {
            // Forward relative mouse movement to vAmiga
            m_pVAmiga->controlPort1.mouse.setDxDy(
                static_cast<double>(event.motion.xrel),
                static_cast<double>(event.motion.yrel));
        } break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: {
            bool pressed = (event.type == SDL_MOUSEBUTTONDOWN);
            using GA = vamiga::GamePadAction;
            switch (event.button.button) {
                case SDL_BUTTON_LEFT:
                    m_pVAmiga->controlPort1.mouse.trigger(
                        pressed ? GA::PRESS_LEFT : GA::RELEASE_LEFT);
                    break;
                case SDL_BUTTON_RIGHT:
                    m_pVAmiga->controlPort1.mouse.trigger(
                        pressed ? GA::PRESS_RIGHT : GA::RELEASE_RIGHT);
                    break;
                case SDL_BUTTON_MIDDLE:
                    m_pVAmiga->controlPort1.mouse.trigger(
                        pressed ? GA::PRESS_MIDDLE : GA::RELEASE_MIDDLE);
                    break;
                default:
                    break;
            }
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

#include "va_config_bridge.h"

namespace vamiga { extern int HDR_ACCEPT_ALL; }

void VAmServerThread::onVAmigaThreadMain() {
    try {
      // using namespace vamiga;
        logDbg("VAmigaServerThread: Initializing...");

        m_pVAmiga = new vamiga::VAmiga();

        struct VAmigaExtConfig ext_cfg;
        qsr_bridge_get_vamiga_config(&ext_cfg);

        if (ext_cfg.cpu_model != 68000) {
            logWarn("VAmiga Server: Requested CPU %d, but vAmiga only supports 68000. Forcing 68000.", ext_cfg.cpu_model);
        }
        
        m_pVAmiga->set(vamiga::ConfigScheme::A500_OCS_1MB);

        // Bypass hardcoded geometry limitations (e.g. max 16 heads or 504MB size limits)
        // By default vAmiga will refuse to mount disks like the 30GB OS 3.2.3 VHD with 32 heads
        vamiga::HDR_ACCEPT_ALL = 1;

        for (int i = 0; i < ext_cfg.num_hds; i++) {
            if (ext_cfg.hd_paths[i]) {
                try {
                    // Connect the HdController (Zorro board) so the drive is visible
                    // to AmigaOS via autoconf. HDC_CONNECT defaults to true only for
                    // slot 0; slots 1-3 default to false and need explicit connection.
                    m_pVAmiga->set(vamiga::Opt::HDC_CONNECT, 1, (long)i);

                    if (ext_cfg.hd_types[i] == 0) { // HDF/VHD
                        logDbg("VAmiga Server: Attaching file-backed hard drive: %s", ext_cfg.hd_paths[i]);
                        m_pVAmiga->hd[i]->attachFileBacked(ext_cfg.hd_paths[i]);
                    } else if (ext_cfg.hd_types[i] == 1) { // DIR
                        logDbg("VAmiga Server: Importing directory to virtual hard drive: %s", ext_cfg.hd_paths[i]);
                        // Create a 500MB virtual drive: 1000 Cylinders, 16 Heads, 63 Sectors (1000 * 16 * 63 * 512 = ~504 MB)
                        m_pVAmiga->hd[i]->attach(1000, 16, 63, 512); 
                        m_pVAmiga->hd[i]->format(vamiga::FSFormat::FFS, ext_cfg.hd_volnames[i] ? ext_cfg.hd_volnames[i] : "DH");
                        m_pVAmiga->hd[i]->importFiles(ext_cfg.hd_paths[i]);
                    }
                } catch (const std::exception &ex) {
                    logErr("VAmiga Server: Failed to attach hard drive '%s'. Reason: %s", ext_cfg.hd_paths[i], ex.what());
                } catch (...) {
                    logErr("VAmiga Server: Failed to attach hard drive '%s'. Reason: Unknown error", ext_cfg.hd_paths[i]);
                }
                
                free((void*)ext_cfg.hd_paths[i]);
                if (ext_cfg.hd_volnames[i]) {
                    free((void*)ext_cfg.hd_volnames[i]);
                }
            }
        }

        // Queue a hard reset so HdController re-evaluates pluggedIn() with HDC_CONNECT
        // set to true for all populated slots. During initialize(), revertToDefaultConfig()
        // resets HDC_CONNECT to defaults (true for slot 0, false for slots 1-3). The
        // HDC_CONNECT commands queued above are processed after initialize(). This
        // hardReset ensures _didReset() runs again with the correct HDC_CONNECT values,
        // setting all HdController boards with attached drives to AUTOCONF state.
        if (ext_cfg.num_hds > 0) {
            m_pVAmiga->hardReset();
        }

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
            // Insert floppy disk (ADF/IMG/DMS) into df0 if provided via CLI
            if (!g_cfg_startup.input.empty()) {
                const std::string &inp = g_cfg_startup.input;
                bool isAdf = qd::ends_with(inp, ".adf") || qd::ends_with(inp, ".img") ||
                             qd::ends_with(inp, ".dms");
                bool isExe = qd::ends_with(inp, ".exe");

                if (isAdf) {
                    try {
                        logDbg("VAmiga Server: Inserting floppy '%s' into df0", inp.c_str());
                        m_pVAmiga->df0.insert(inp.c_str(), false);
                    } catch (const std::exception &ex) {
                        logErr("VAmiga Server: Failed to insert floppy '%s'. Reason: %s", inp.c_str(), ex.what());
                    }
                } else if (isExe) {
                    // For .exe files, UAE creates a bootable ADF via ADFlib.
                    // vAmiga can't do that directly — require user to convert to ADF first.
                    logWarn("VAmiga Server: .exe input not supported by vAmiga. Convert to ADF first.");
                }
            }

            // Serial port: vAmiga uses internal serial devices (NULLMODEM, LOOPBACK, etc.)
            // and does not support host serial paths like UAE. The --serial_port CLI arg
            // is a UAE-specific feature and is not applicable here.

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
        m_onVAmInitialized->set();  // Unblock main thread on failure
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
