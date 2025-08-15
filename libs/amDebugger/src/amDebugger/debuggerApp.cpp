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
#include "amDebugger/vm/absVM.h"
#include "qd/thread/thread.h"
#include "amDebugger/ui/debuggerDesktop.h"
#include "amDebugger/ui/uiStyle.h"
#include "qd/qui/uiOperationMgr.h"
#include "qd/imGui/imGuiContextManager.h"
#include "qd/app/moduleManager.h"
#include "dbgConnection.h"
#include "qd/app/application.h"


namespace amD {


void DebuggerApp::setCurDbgClientIdx(uint32_t curDbgClientIdx)
{
    if (curDbgClientIdx >= m_pClients.size())
        ASSERT_AND_DO(0, curDbgClientIdx = 0, "Bad client index");
    m_nCurDbgClientIdx = curDbgClientIdx;
    m_pCurDbgClient = m_pClients[m_nCurDbgClientIdx];
}


DebuggerApp::DebuggerApp()
{
    DebuggerApp::g_pInstance = this;

    m_pClients.push_back(new Debugger(this, amD::create_dummy_connection())); // DummyClient

    setPartActive(true);
    setPartVisible(true);
}


void DebuggerApp::onPartCreate(AppPart::OnCreate_t& prm)
{
    TSuper::onPartCreate(prm);
}

void DebuggerApp::init() {
    mbInit = true;
    createRenderWindow();
    initImGui();

    IDbgConnectionManager* pConnMgr = m_pApp->getInterface_<IDbgConnectionManager>();
    ASSERT_AND_DO(pConnMgr, return);
    for (uint32_t i = 0; i < pConnMgr->getNumConnections(); ++ i)
    {
        if (ref_ptr<IDbgConnection> pCon = pConnMgr->getConnectionByNo(i))
            m_pClients.push_back(new Debugger(this, pCon));
    }

    qd::UiNodeCreator mk;
    m_pGui = mk.make_<amD::DebuggerDesktop>(m_pCurDbgClient);
    m_pOperationMgr = m_pGui->getOperationMgr();
    assert(m_pOperationMgr);
    assert(m_pOperationMgr->getNumOps());
}


void DebuggerApp::createRenderWindow() {
    uint32_t window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_HIDDEN;
    SDL_Window* window =
        SDL_CreateWindow("Quaesar: DebuggerApp", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);

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
        SDL_Log("Error creating SDL_Renderer!");
        return;
    }
    m_pWindow = window;

}


void DebuggerApp::initImGui() {

    auto pImGuiMgr = qd::ModuleManager::get()->getModuleInstOrCreate_<qd::ImGuiContextManager>();
    m_pQimGui = pImGuiMgr->createContextImGui(m_pWindow, m_pWndRenderer);

    // Setup Dear ImGui context
    ImGuiIO& io = m_pQimGui->getIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Setup Dear ImGui style
    qd::imGuiApplyStyleDark();
}


DebuggerApp::~DebuggerApp() {
    assert(!mbInit);
}


qd::EFlow DebuggerApp::applyOperationMsg(qd::operation::args::Base* p_msg)
{
    return m_pCurDbgClient->applyOperationMsg(p_msg);
}


void* DebuggerApp::getOpEnvPtr(const qd::TypeInfo& classType) const
{
    if (classType == this->getStaticTypeInfo())
        return const_cast<void*>((const void*)this);
    return nullptr;
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
    AbsVM::VM::destrotVmInst();

    SDL_DestroyRenderer(m_pWndRenderer);
    m_pWndRenderer = nullptr;
    SDL_DestroyWindow(m_pWindow);
    m_pWindow = nullptr;

    mbInit = false;
}


void DebuggerApp::update(float dt, float time)
{
    if (isWndVisible())
    {
        m_pQimGui->newFrame();
        m_pGui->drawImGuiMainFrame();
        m_pQimGui->endFrame();
    }
}


void DebuggerApp::render() {
    if (isWndVisible())
        m_pQimGui->render(qd::Color(128,128,128));

}


bool DebuggerApp::isWndVisible() const {
    uint32_t window_flags = SDL_GetWindowFlags(m_pWindow);
    if (window_flags & (SDL_WINDOW_HIDDEN | SDL_WINDOW_MINIMIZED)) {
        return false;
    } else {
        return true;
    }
}


void DebuggerApp::toggleWndVisible(EDebuggerMode mode) {
    if (!isWndVisible()) {
        SDL_ShowWindow(m_pWindow);
    } else {
        SDL_HideWindow(m_pWindow);
    }
}


qd::EFlow DebuggerApp::onSdlEventProc(SDL_Event& event) {
    return m_pQimGui->onSdlEventProc(event);
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
