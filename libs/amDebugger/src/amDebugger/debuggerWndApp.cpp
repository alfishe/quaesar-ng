#include "debuggerWndApp.h"
#include "amDebugger/debuggerOps.h"
#include "amDebugger/ui/debuggerDesktop.h"
#include "amDebugger/ui/uiStyle.h"
#include "amDebugger/vm/vmInterface.h"
#include "dbgConnection.h"
#include "qd/app/application.h"
#include "qd/app/moduleManager.h"
#include "qd/imGui/backends/sdl2/imgui_impl_sdl2.h"
#include "qd/imGui/backends/sdl2/imgui_impl_sdlrenderer2.h"
#include "qd/imGui/imGuiContextManager.h"
#include "qd/qui/operationsRegistry.h"
#include "qd/thread/thread.h"
#include <EASTL/queue.h>
#include "qd/stl/algorithm.h"
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <SDL.h>


namespace amD {

constexpr uint32_t g_nDebuggerWndSizeX = 1368;
constexpr uint32_t g_nDebuggerWndSizeY = 800;


DebuggerApp::DebuggerApp()
{
    setPartActive(true);
    setPartRenderable(true);

    m_pDebugger = new amD::Debugger(this); // Debugger client
}


void DebuggerApp::onPartCreate(ApplicationPart::OnCreate_t& prm)
{
    TSuper::onPartCreate(prm);
}


void DebuggerApp::init()
{
    m_init = true;
    createRenderWindow();
    initImGui();

    m_pDebugger->setDbgServiceBridge(create_dummy_connection());

    assert(m_pDebugger);
    qd::UiNodeCreator mk;
    m_pGui = mk.make_<amD::DebuggerDesktop>(this, m_pDebugger);
    m_pOperationMgr = m_pGui->getOperationMgr();
    assert(m_pOperationMgr);
}


void DebuggerApp::createRenderWindow()
{
    uint32_t window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_HIDDEN;
    SDL_Window* window = SDL_CreateWindow("Quaesar: DebuggerApp", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        g_nDebuggerWndSizeX, g_nDebuggerWndSizeY, window_flags);

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif
    if (!window)
    {
        fprintf(stderr, "Error creating window.\n");
        return;
    }
    m_pWndRenderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (!m_pWndRenderer)
    {
        SDL_DestroyWindow(window);
        SDL_LogCritical(0, "Error creating SDL_Renderer!");
        return;
    }
    m_pWindow = window;
}


void DebuggerApp::initImGui()
{
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


DebuggerApp::~DebuggerApp()
{
    assert(!m_init);
}


qd::EFlow DebuggerApp::applyOperationMsgProcImp(qd::operation::BaseOpArgs* p_msg)
{
    return m_pDebugger->applyOperationMsgProcImp(p_msg);
}


void DebuggerApp::destroy()
{
    if (m_pOperationMgr)
        m_pOperationMgr->destroy();
    m_pOperationMgr = nullptr;

    if (m_pGui)
        m_pGui->destroy();
    m_pGui = nullptr;

    SAFE_DESTROY(m_pQimGuiCtx);
    SDL_DestroyRenderer(m_pWndRenderer);
    m_pWndRenderer = nullptr;
    SDL_DestroyWindow(m_pWindow);
    m_pWindow = nullptr;

    m_init = false;
}


void DebuggerApp::updateAppPart(float /*dt*/, float /*time*/)
{
    if (isWndVisible())
    {
        m_pQimGuiCtx->newFrame();
        getDbg()->fetchVmState();
        m_pGui->drawImGuiMainFrame();
        m_pQimGuiCtx->endFrame();
    }
    else
        m_pQimGuiCtx->skipFrame();
}


void DebuggerApp::renderAppPart()
{
    if (isWndVisible() && m_pQimGuiCtx->m_frameStarted)
        m_pQimGuiCtx->render(qd::Color(128, 128, 128));
}


bool DebuggerApp::isWndVisible() const
{
    uint32_t window_flags = SDL_GetWindowFlags(m_pWindow);
    if (window_flags & (SDL_WINDOW_HIDDEN | SDL_WINDOW_MINIMIZED))
        return false;
    return true;
}


void DebuggerApp::setWndVisible(bool v)
{
    if (v)
    {
        SDL_ShowWindow(m_pWindow);
        setPartRenderable(true);
    }
    else
    {
        SDL_HideWindow(m_pWindow);
        setPartRenderable(false);
    }
}


qd::EFlow DebuggerApp::onSdlEventProc(SDL_Event& event)
{
    uint32_t uaeWndId = SDL_GetWindowID(m_pWindow);
    switch (event.type)
    {
    case SDL_WINDOWEVENT:
    {
        if (event.window.windowID != uaeWndId)
            return qd::EFlow::CONTINUE;
        uint8_t wndEvent = event.window.event;
        if (wndEvent == SDL_WINDOWEVENT_CLOSE)
        {
            setWndVisible(false);
            return qd::EFlow::STOP;
        }
        break;
    }
    default:
        break;
    };

    // send system events to current ImGui context
    return m_pQimGuiCtx->onSdlEventProc(event);
}


IVm::VM* DebuggerApp::getVm() const
{
    return m_pDebugger->getVm();
}


const amD::Breakpoint* BreakpointsSortedList::getBpByAddr(AddrRef addr, EReg /*reg*/) const
{
    OneAddrBp lh;
    lh.addr = addr;
    auto it = mOneAddrBps.find(lh);
    if (it == mOneAddrBps.end())
        return nullptr;
    return &mBreakpoints[it->bpIdx];
}


}; // namespace amD
