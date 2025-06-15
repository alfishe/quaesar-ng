#include "debugger.h"
#include <EASTL/queue.h>
#include <EASTL/sort.h>
#include <SDL.h>
#include <capstone/capstone.h>
#include <imgui/backends/imgui_impl_sdl2.h>
#include <imgui/backends/imgui_impl_sdlrenderer2.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <amDebugger/dbgOperation.h>
#include <amDebugger/msg_list.h>
#include <amDebugger/vm/vm.h>
#include "qd/thread/thread.h"
#include <amDebugger/ui/gui_manager.h>
#include <amDebugger/ui/ui_style.h>
#include "qd/ui/uiOperationManager.h"
#include "qd/imGui/imGuiManager.h"
#include "qd/app/moduleManager.h"


namespace qd {


//////////////////////////////////////////////////////////////////////////
namespace imp {
class ConsoleQueue {
public:
    eastl::queue<eastl::string> m_consoleCmdQueue;
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
ConsoleQueue console_queue;


};  // namespace imp
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////

void Debugger::init() {
    mbInit = true;
    createRenderWindow();
    initImGui();
    vm = VM::setVmInst(createByFactory<qd::VM>());
    vm->init();

    NodeCreator mk;
    mk.parent = nullptr;
    gui = mk.make_<GuiManager>(this);

    m_pOperations = gui->getOperationMgr();
    assert(m_pOperations);

    assert(m_pOperations->getNumChild());

    m_pCapstone = new csh();

    // TODO: Pick correct CPU depending on starting CPU
    cs_err err = cs_open(CS_ARCH_M68K, (cs_mode)(CS_MODE_BIG_ENDIAN | CS_MODE_M68K_000), m_pCapstone);
    if (err) {
        printf("Failed on cs_open() with error returned: %u\n", err);
        abort();
    }
    cs_option(*m_pCapstone, CS_OPT_DETAIL, CS_OPT_ON);
}


void Debugger::createRenderWindow() {
    uint32_t window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_HIDDEN;
    SDL_Window* window =
        SDL_CreateWindow("Quaesar: Debugger", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    if (!window) {
        fprintf(stderr, "Error creating window.\n");
        return;
    }

    m_pRenderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (!m_pRenderer) {
        SDL_DestroyWindow(window);
        SDL_Log("Error creating SDL_Renderer!");
        return;
    }
    m_pWindow = window;

}


void Debugger::initImGui() {

    auto pImGuiMgr = qd::ModuleManager::get()->getModuleInstOrCreate_<qd::ImGuiManager>();
    m_pImGui = pImGuiMgr->createContextImGui(m_pWindow, m_pRenderer);

    // Setup Dear ImGui context
    ImGuiIO& io = m_pImGui->getIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Setup Dear ImGui style
    qd::UiStyle::get()->applyImGuiDarkStyle();
}


Debugger::~Debugger() {
    assert(!mbInit);
}


void Debugger::setDebugMode(DebuggerMode debug_mode) {
    vm->emu->setDebugMode(debug_mode);
    return;
}

qd::EFlow Debugger::applyOperationMsg(qd::operation::msg::Base* p_msg) const {
    return m_pOperations->applyOperationMsg(p_msg);
}


int Debugger::waitConsoleCmd(char* out, int maxlen) {
    eastl::string cmd;
    if (!qd::imp::console_queue.waitConsoleCmd(cmd))
        return -1;

    const int len = (int)cmd.size();
    if (len < maxlen)
        strcpy(out, cmd.data());
    else
        EASTL_ASSERT(0);
    return len;
}


qd::Debugger* Debugger::get() {
    return g_pInstance;
}


 Debugger::Debugger()
{
    Debugger::g_pInstance = this;

    setPartActive(true);
    setPartVisisble(true);
}


void qd::Debugger::execConsoleCmd(eastl::string&& cmd) {
    imp::console_queue.addCmdToQueue(eastl::move(cmd));
}


void Debugger::destroy() {
    if (gui)
        gui->destroy();
    imp::console_queue.destroy();
    if (m_pOperations)
        m_pOperations->destroy();
    m_pOperations = nullptr;

    // Cleanup
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    delete gui;
    delete m_pCapstone;
    m_pCapstone = nullptr;
    gui = nullptr;
    vm = nullptr;
    VM::destrotVmInst();

    SDL_DestroyRenderer(m_pRenderer);
    m_pRenderer = nullptr;
    SDL_DestroyWindow(m_pWindow);
    m_pWindow = nullptr;

    mbInit = false;
}


void Debugger::update(float dt, float time)
{
    m_pImGui->newFrame();

    gui->drawImGuiMainFrame();

    m_pImGui->endFrame();
}


void Debugger::render() {
    m_pImGui->render(qd::Color(128,128,128));

}


bool Debugger::isVisible() const {
    uint32_t window_flags = SDL_GetWindowFlags(m_pWindow);
    if (window_flags & (SDL_WINDOW_HIDDEN | SDL_WINDOW_MINIMIZED)) {
        return false;
    } else {
        return true;
    }
}


void Debugger::toggleWndVisible(DebuggerMode mode) {
    if (!isVisible()) {
        SDL_ShowWindow(m_pWindow);
    } else {
        SDL_HideWindow(m_pWindow);
    }
}


void Debugger::onSdlEventProc(SDL_Event& event) {
    m_pImGui->onSdlEventProc(event);
}




const qd::Breakpoint* BreakpointsSortedList::getBpByAddr(AddrRef addr, EReg reg) const {
    OneAddrBp lh;
    lh.addr = addr;
    auto it = mOneAddrBps.find(lh);
    if (it == mOneAddrBps.end())
        return nullptr;
    return &mBreakpoints[it->bpIdx];
}


};  // namespace qd
