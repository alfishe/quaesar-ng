#include "debuggerApp.h"
#include <EASTL/queue.h>
#include <EASTL/sort.h>
#include <SDL.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include "qd/imGui/backends/imgui_impl_sdl2.h"
#include "qd/imGui/backends/imgui_impl_sdlrenderer2.h"
#include "amDebugger/dbgOperation.h"
#include "amDebugger/debuggerOps.h"
#include "amDebugger/vm/vmInterface.h"
#include "qd/thread/thread.h"
#include "amDebugger/ui/debuggerDesktop.h"
#include "amDebugger/ui/uiStyle.h"
#include "qd/qui/operationsRegistry.h"
#include "qd/imGui/imGuiContextManager.h"
#include "qd/app/moduleManager.h"
#include "dbgConnection.h"
#include "qd/app/application.h"


namespace amD {

constexpr uint32_t g_nDebuggerWndSizeX = 1368;
constexpr uint32_t g_nDebuggerWndSizeY = 800;


DebuggerApp::DebuggerApp()
{
    DebuggerApp::g_pInstance = this;

    setPartActive(true);
    setPartVisible(true);
}


void DebuggerApp::onPartCreate(ApplicationPart::OnCreate_t& prm)
{
    TSuper::onPartCreate(prm);
}

void DebuggerApp::init() {
    m_init = true;
    createRenderWindow();
    initImGui();

    amD::IDbgConnectionManager* pConnMgr = m_pApp->getInterface_<IDbgConnectionManager>();
    ASSERT_AND_DO(pConnMgr, return);
    ref_ptr<IDbgConnection> pCurConnect = pConnMgr->createConnectionByInd(0);
    assert(pCurConnect);

    m_pDebugger = new Debugger(this, pCurConnect); // Debugger client
    m_pDebugger->init();


    assert(m_pDebugger);
    qd::UiNodeCreator mk;
    m_pGui = mk.make_<amD::DebuggerDesktop>(this, m_pDebugger);
    m_pOperationMgr = m_pGui->getOperationMgr();
    assert(m_pOperationMgr);
}


void DebuggerApp::createRenderWindow() {
    uint32_t window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_HIDDEN;
    SDL_Window* window =
        SDL_CreateWindow("Quaesar: DebuggerApp", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, g_nDebuggerWndSizeX, g_nDebuggerWndSizeY, window_flags);

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif
    if (!window) {
        fprintf(stderr, "Error creating window.\n");
        return;
    }
    m_pWndRenderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (!m_pWndRenderer) {
        SDL_DestroyWindow(window);
        SDL_LogCritical(0, "Error creating SDL_Renderer!");
        return;
    }
    m_pWindow = window;
}


void DebuggerApp::initImGui() {

    auto pImGuiMgr = qd::ModuleManager::get()->getModuleInstOrCreate_<qd::ImGuiContextManager>();
    m_pQimGuiCtx = pImGuiMgr->createContextImGui(m_pWindow, m_pWndRenderer);

    // Setup Dear ImGui context
    ImGuiIO& io = m_pQimGuiCtx->getIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Setup Dear ImGui style
    qd::imGuiApplyStyleDark();
}


DebuggerApp::~DebuggerApp() {
    assert(!m_init);
}


qd::EFlow DebuggerApp::applyOperationMsgProcImp(qd::operation::args::Base* p_msg)
{
    return m_pDebugger->applyOperationMsgProcImp(p_msg);
}


amD::DebuggerApp* DebuggerApp::get() {
    return g_pInstance;
}




void DebuggerApp::destroy() {
    if (m_pGui)
        m_pGui->destroy();
    if (m_pOperationMgr)
        m_pOperationMgr->destroy();
    m_pOperationMgr = nullptr;

    // Cleanup
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    delete m_pGui;
    m_pGui = nullptr;
    IVm::VM::destrotVmInst();

    SDL_DestroyRenderer(m_pWndRenderer);
    m_pWndRenderer = nullptr;
    SDL_DestroyWindow(m_pWindow);
    m_pWindow = nullptr;

    m_init = false;
}


void DebuggerApp::update(float dt, float time)
{
    if (isWndVisible())
    {
        m_pQimGuiCtx->newFrame();
        m_pGui->drawImGuiMainFrame();
        m_pQimGuiCtx->endFrame();
    }
    else
        m_pQimGuiCtx->skipFrame();
}


void DebuggerApp::render() {
    if (isWndVisible() && m_pQimGuiCtx->m_frameStarted)
        m_pQimGuiCtx->render(qd::Color(128,128,128));

}


bool DebuggerApp::isWndVisible() const {
    uint32_t window_flags = SDL_GetWindowFlags(m_pWindow);
    if (window_flags & (SDL_WINDOW_HIDDEN | SDL_WINDOW_MINIMIZED)) {
        return false;
    } else {
        return true;
    }
}


void DebuggerApp::setWndVisible(bool v) {
    if (v) {
        SDL_ShowWindow(m_pWindow);
        setPartVisible(true);
    } else {
        SDL_HideWindow(m_pWindow);
        setPartVisible(false);
    }
}



qd::EFlow DebuggerApp::onSdlEventProc(SDL_Event& event) {
    // send system events to current ImGui context
    return m_pQimGuiCtx->onSdlEventProc(event);
}




const amD::Breakpoint* BreakpointsSortedList::getBpByAddr(AddrRef addr, EReg reg) const
{
    OneAddrBp lh;
    lh.addr = addr;
    auto it = mOneAddrBps.find(lh);
    if (it == mOneAddrBps.end())
        return nullptr;
    return &mBreakpoints[it->bpIdx];
}


};  // namespace amD
