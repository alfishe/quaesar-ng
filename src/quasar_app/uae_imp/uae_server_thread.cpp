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
#include "qd/debug/assert.h"
#include "qd/log/log.h"
#include "qd/thread/thread.h"
#include "qsr_config.h"
#include "quasar_app/quaesar.h"
#include "uae_server_app_part.h"
#include "uae_vm_imp.h"
#include <SDL.h>
#include <queue>

extern void real_main(int argc, TCHAR **argv);
extern void qs_keyboard_set_translation();
extern void quae__parseCmdLine(int argc, TCHAR **argv);

namespace amD::uae {
extern void do_console_cmd_immediate(const char *cmd);
}; // namespace amD::uae

class UaeConsoleQueue {
public:
  std::queue<qtd::string> m_consoleCmdQueue;
  qd::ThreadEvent *m_pThreadEvent;
  qd::Mutex *m_pMutex;

public:
  UaeConsoleQueue() {
    m_pThreadEvent = new qd::ThreadEvent(true);
    m_pMutex = new qd::Mutex();
  }

  void addCmdToQueue(qtd::string cmd) {
    if (cmd.empty())
      return;
    m_pMutex->lock();
    m_consoleCmdQueue.push(std::move(cmd));
    m_pMutex->unlock();
    m_pThreadEvent->set();
  }

  bool popConsoleCmdWait(qtd::string &out_cmd) {
    for (;;) {
      m_pThreadEvent->wait();  // block until set() by addCmdToQueue
      qd::MutexLock ml(*m_pMutex);
      if (!m_consoleCmdQueue.empty()) {
        out_cmd = std::move(m_consoleCmdQueue.front());
        m_consoleCmdQueue.pop();
        return true;
      }
      // Spurious wake — reset and wait again
      m_pThreadEvent->reset();
    }
  }

  void destroy() {
    m_consoleCmdQueue = {};
    if (m_pThreadEvent) {
      m_pThreadEvent->set();
      SAFE_DELETE(m_pThreadEvent);
    }
    SAFE_DELETE(m_pMutex);
  }

  ~UaeConsoleQueue() { destroy(); }

}; // class UaeConsoleQueue
//////////////////////////////////////////////////////////////////////////

static int uae_thread_main_func(void *) {
  // Apply Quaesar user-facing defaults BEFORE -s parsing so the user can
  // override any of them from the command line.
  currprefs.cpu_cycle_exact = true;
  currprefs.cpu_memory_cycle_exact = true;
  currprefs.blitter_cycle_exact = true;
  currprefs.sound_stereo_separation = 0;

  std::vector<const char *> argv;
  argv.push_back("quaesar.exe");
  argv.reserve(g_cfg_startup.uaeExtArgs.size() * 2 + 1);
  // pass remain Quaesar CLI args to UAE
  for (const auto &s : g_cfg_startup.uaeExtArgs) {
    argv.push_back("-s");
    argv.push_back(s.c_str());
  }
  quae__parseCmdLine((int)argv.size(), const_cast<char **>(&argv[0]));

  // Reconcile: cpu_speed=max (m68k_speed<0) is semantically incompatible with
  // cpu_cycle_exact. The UAE config parser treats them as independent fields,
  // so cpu_speed=max only sets m68k_speed=-1 without clearing cycle-exact —
  // leaving the CPU locked to real 68000 bus cycles (negligible speedup).
  // When the user requests max speed, honor that intent by switching to
  // cpu_compatible mode: chipset timing stays precise (copper/DMA/blitter via
  // the event scheduler), CPU runs ~21x faster (cycles_mult = CYCLES_DIV / 21).
  if (currprefs.m68k_speed < 0) {
    currprefs.cpu_cycle_exact = false;
    currprefs.cpu_memory_cycle_exact = false;
    currprefs.blitter_cycle_exact = false;
    currprefs.cpu_compatible = true;
  }

  ::real_main(0, nullptr); // call main function of UAE emulator
  return 0;
}

UaeServerThread::UaeServerThread(qsr::UaeServerAppPart *pServerApp)
    : m_pServerApp(pServerApp) {
  m_pVm = new IVm::imp::UaeVmImp();
  m_pVm->setServerImp(this);

  assert(!g_pSingleton);
  g_pSingleton = this;
  m_pConsoleQueue = new UaeConsoleQueue();
  m_pauseEvent = new qd::ThreadEvent(true);  // auto-reset event
}

void UaeServerThread::initialize() {
  ::syncbase = 1000000;
  qs_keyboard_set_translation();
  ::default_prefs(&::currprefs, true, 0);
  ::fixup_prefs(&::currprefs, true);

  // const CfgQsrStartup& options = ;
  if (!g_cfg_startup.input.empty()) {
    if (qd::ends_with(g_cfg_startup.input, ".exe") ||
        !qd::ends_with(g_cfg_startup.input, ".adf")) {
      if (FILE *check_file = fopen(g_cfg_startup.input.c_str(), "rb")) {
        fclose(check_file);
        Adf::create_for_exefile(g_cfg_startup.input.c_str());
        strcpy(::currprefs.floppyslots[0].df, "dummy.adf");
      } else {
        SDL_Log("can't open input file:'%s'", g_cfg_startup.input.c_str());
      }
    } else {
      strcpy(::currprefs.floppyslots[0].df, g_cfg_startup.input.c_str());
    }
  }

  if (!g_cfg_startup.serialPort.empty()) {
    currprefs.use_serial = 1;
    strcpy(currprefs.sername, g_cfg_startup.serialPort.c_str());
  }

  // Infrastructure settings (not user-overridable via -s)
  currprefs.uaeboard = 1;
  currprefs.win32_filesystem_mangle_reserved_names = true; // required for FS
  currprefs.filesys_custom_uaefsdb = false; // hack to not implement 'custom_fsdb_*' funcs now

  strcpy(currprefs.romfile, g_cfg_startup.kickRomPath.c_str());

  m_scrWidth = 754;
  m_scrHeight = 576;
  assert(!m_pAmigaBuffer);
  m_pAmigaBuffer = new uint32_t[m_scrWidth * m_scrHeight];
  SDL_AtomicSet(&m_scrFrameNo, 0);

  // start UAE Thread
  m_onUaeInitialized = new qd::ThreadEvent(true);
  m_uaeThread =
      SDL_CreateThread(&uae_thread_main_func, "UAE emulator", nullptr);

  // wait UAE initialization
  m_onUaeInitialized->wait();

  // initialize after UAE is ready
  m_pVm->init();
}

void UaeServerThread::destroy() {
  m_isDestroying = true;
  uae_quit();
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

  // If paused, resume to unblock the UAE thread
  if (m_isPaused)
    resumeEmulation();

  delete[] m_pAmigaBuffer;
  m_pAmigaBuffer = nullptr;
  SAFE_DELETE(m_pauseEvent);
}

UaeServerThread::~UaeServerThread() {
  destroy();
  assert(g_pSingleton == this);
  g_pSingleton = nullptr;
}

void UaeServerThread::setUaeInitialized(bool) {
  ASSERT_AND_DO(m_onUaeInitialized, return );
  m_onUaeInitialized->set();
}

uint32_t *UaeServerThread::_lockUaeScreenTexBuf(int amiga_width,
                                                int amiga_height) {
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

void UaeServerThread::pushSdlEvent(const SDL_Event &event) {
  qd::MutexLock ml(m_eventMutex);
  m_sdlEventsQueue.push_back(event);
}

void UaeServerThread::pushOperationMsg(
    qtd::unique_ptr<qd::operation::BaseOpArgs> args) {
  qd::MutexLock ml(m_eventMutex);
  m_pClientOpsStack.push_back(qtd::move(args));
}

bool UaeServerThread::onUaeHandleEvents() {
  {
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
  }

  // If paused, block here until resumed. This halts the UAE emulation loop.
  // The resumeEmulation() call from the main thread will set m_isPaused = false
  // and signal m_pauseEvent, unblocking us.
  // We use a timeout so we periodically wake up to process queued operations
  // (e.g., the resume/continue operation itself).
  while (m_isPaused) {
    m_pauseEvent->wait(50);  // wake every 50ms to process queued ops
    
    // Process any queued operations that arrived while paused
    qd::MutexLock ml(m_eventMutex);
    while (!m_pClientOpsStack.empty()) {
      qd::operation::BaseOpArgs *pCurOpMsg = m_pClientOpsStack.front().get();
      m_pVm->applyOperationMsgProc(pCurOpMsg);
      m_pClientOpsStack.pop_front();
    }
  }

  return false;
}

void UaeServerThread::pauseEmulation() {
  m_isPaused = true;
  m_pauseEvent->reset();
}

void UaeServerThread::resumeEmulation() {
  m_isPaused = false;
  m_pauseEvent->set();
}

IVm::VM *UaeServerThread::getVm() const { return m_pVm; }

bool UaeServerThread::lockDisplayTexBuf(int *width, int *height,
                                        uint32_t **out_pixels) {
  m_UaeScrTextureMutex.lock();
  if (!m_pAmigaBuffer) {
    unlockDisplayTexBuf();
    return false;
  }
  *width = m_scrWidth;
  *height = m_scrHeight;
  *out_pixels = m_pAmigaBuffer;
  return true;
}

void UaeServerThread::unlockDisplayTexBuf() { m_UaeScrTextureMutex.unlock(); }

void UaeServerThread::applySdlEventProc(const SDL_Event &event) {
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
  case SDL_MOUSEMOTION: {
    setmousestate(0, 0, event.motion.xrel, 0);
    setmousestate(0, 1, event.motion.yrel, 0);
  } break;
  case SDL_MOUSEBUTTONDOWN:
  case SDL_MOUSEBUTTONUP: {
    int button = 0;
    if (event.button.button == SDL_BUTTON_LEFT) button = JOYBUTTON_1;
    else if (event.button.button == SDL_BUTTON_RIGHT) button = JOYBUTTON_2;
    else if (event.button.button == SDL_BUTTON_MIDDLE) button = JOYBUTTON_3;
    setmousebuttonstate(0, button, event.type == SDL_MOUSEBUTTONDOWN ? 1 : 0);
  } break;
  case SDL_MOUSEWHEEL: {
    if (event.wheel.y > 0)
        setmousebuttonstate(0, 3, 1); // Wheel Up
    else if (event.wheel.y < 0)
        setmousebuttonstate(0, 4, 1); // Wheel Down
    // Note: Wheel events are usually instantly released in UAE
    if (event.wheel.y != 0) {
        setmousebuttonstate(0, 3, 0);
        setmousebuttonstate(0, 4, 0);
    }
  } break;
  default:
    break;
  }
}

void UaeServerThread::applyImmediateConsoleCmd(qtd::string &&cmd) {
  amD::uae::do_console_cmd_immediate(cmd.c_str());
}

void UaeServerThread::execConsoleCmd(qtd::string &&cmd) {
  m_pConsoleQueue->addCmdToQueue(std::move(cmd));
}

int UaeServerThread::uaeWaitConsoleCmdImpl(char *out, int maxlen) {
  qtd::string cmd;
  if (!m_pConsoleQueue->popConsoleCmdWait(cmd))
    return -1;

  const int len = (int)cmd.size();
  if (len < maxlen)
    strcpy(out, cmd.data());
  else
    assert(0);
  return len;
}
