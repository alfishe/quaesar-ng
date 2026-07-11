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
#include <filesystem>


namespace amD {

constexpr uint32_t g_nDebuggerWndSizeX = 1368;
constexpr uint32_t g_nDebuggerWndSizeY = 800;

static int resizeEventWatcher(void* data, SDL_Event* event)
{
    if (event->type == SDL_WINDOWEVENT && event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
    {
        DebuggerApp* app = static_cast<DebuggerApp*>(data);
        if (app && SDL_GetWindowID(app->getWindow()) == event->window.windowID)
        {
            app->updateAppPart(0, 0);
            app->renderAppPart();
        }
    }
    return 0;
}


// Persistent storage for the ini filename (must outlive the ImGui context).
static char g_iniFilename[2048] = "";


// If the user has no saved imgui.ini yet, copy the bundled default layout next to the executable
// so that ImGui's own NewFrame() loader picks it up.  This avoids calling LoadIniSettingsFromMemory()
// before the first frame, which can't properly rebuild dock nodes because no windows exist yet.
static void ensureDebuggerLayoutFile()
{
    char* base = SDL_GetBasePath();
    if (!base)
    {
        SDL_Log("Debugger layout: SDL_GetBasePath() returned NULL");
        return;
    }

    char iniPath[2048];
    char defaultPath[2048];
    SDL_snprintf(iniPath, sizeof(iniPath), "%simgui.ini", base);
    SDL_snprintf(defaultPath, sizeof(defaultPath), "%sdefault_layout.ini", base);
    SDL_strlcpy(g_iniFilename, iniPath, sizeof(g_iniFilename));
    SDL_free(base);

    if (std::filesystem::exists(iniPath))
    {
        SDL_Log("Debugger layout: user ini exists at \"%s\"", iniPath);
        return;
    }

    if (!std::filesystem::exists(defaultPath))
    {
        SDL_Log("Debugger layout: no default layout at \"%s\" — first launch will show empty dockspace", defaultPath);
        return;
    }

    std::error_code ec;
    std::filesystem::copy_file(defaultPath, iniPath,
        std::filesystem::copy_options::overwrite_existing, ec);
    if (ec)
        SDL_Log("Debugger layout: failed to copy \"%s\" -> \"%s\" (%s)", defaultPath, iniPath, ec.message().c_str());
    else
        SDL_Log("Debugger layout: copied default layout to \"%s\"", iniPath);
}


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

    SDL_AddEventWatch(resizeEventWatcher, this);

    m_pDebugger->setDbgServiceBridge(create_dummy_connection());

    assert(m_pDebugger);
    qd::UiNodeCreator mk;
    m_pGui = mk.make_<amD::DebuggerDesktop>(this, m_pDebugger);
    m_pOperationMgr = m_pGui->getOperationMgr();
    assert(m_pOperationMgr);

    loadLayoutSettings();
    m_bFullyInitialized = true;
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
    // NO vsync on the debugger renderer.
    //
    // The main window renderer (QsrMainClientWnd) is the SOLE vsync authority —
    // its SDL_RENDERER_PRESENTVSYNC call paces the entire main loop. The main
    // loop renders BOTH windows sequentially in one iteration:
    //
    //   iteration:  updateAppPart(main)  → renderAppPart(main)  → [vsync block]
    //              updateAppPart(debugger) → renderAppPart(debugger) → [no block]
    //
    // If this debugger renderer also had vsync, each iteration would block on
    // two independent vsync waits (~33ms total at 60Hz), halving the effective
    // frame rate. The emulator produces 50fps; a 30fps loop would drop half.
    m_pWndRenderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!m_pWndRenderer)
    {
        SDL_DestroyWindow(window);
        SDL_LogCritical(0, "Error creating SDL_Renderer!");
        return;
    }
    m_pWindow = window;

    // Clear framebuffer to gray immediately to avoid red/garbage flash
    SDL_SetRenderDrawColor(m_pWndRenderer, 128, 128, 128, 255);
    SDL_RenderClear(m_pWndRenderer);
    SDL_RenderPresent(m_pWndRenderer);
}


void DebuggerApp::initImGui()
{
    auto pImGuiMgr = qd::ModuleManager::get()->getModuleInstOrCreate_<qd::ImGuiContextManager>();
    m_pQimGuiCtx = pImGuiMgr->createContextImGui(m_pWindow, m_pWndRenderer);

    // Setup Dear ImGui context
    ImGuiIO& io = m_pQimGuiCtx->getIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = "debugger_layout.ini";

    // Setup Dear ImGui style
    qd::imGuiApplyStyleDark();

    // First-launch bootstrap: if the user has no saved layout yet, copy the bundled default
    // layout next to the executable.  ImGui's own NewFrame() will load it on the first frame.
    // Point IniFilename to a full path (next to exe) instead of the default CWD-relative "imgui.ini".
    ensureDebuggerLayoutFile();
    if (g_iniFilename[0] != '\0')
        io.IniFilename = g_iniFilename;
}


void DebuggerApp::loadLayoutSettings()
{
    m_pQimGuiCtx->useCurrent();
    ImGui::LoadIniSettingsFromDisk(ImGui::GetIO().IniFilename);
}


DebuggerApp::~DebuggerApp()
{
    assert(!m_init);
}


qd::EFlow DebuggerApp::applyOperationMsgProcImp(qd::operation::BaseOpArgs* p_msg)
{
    qd::EFlow r = m_pDebugger->applyOperationMsgProcImp(p_msg);

    // Also forward to the real emulator thread (if callback registered)
    // so that emulator control operations (pause, continue, step, etc.)
    // reach the actual running emulator, not just the dummy VM.
    if (m_forwardOpToEmulatorCb)
        m_forwardOpToEmulatorCb(p_msg);

    return r;
}


void DebuggerApp::destroy()
{
    SDL_DelEventWatch(resizeEventWatcher, this);

    if (m_pOperationMgr)
        m_pOperationMgr->destroy();
    m_pOperationMgr = nullptr;

    if (m_pGui)
        m_pGui->destroy();
    m_pGui = nullptr;

    // Save layout before destroying context
    if (m_pQimGuiCtx)
    {
        m_pQimGuiCtx->useCurrent();
        ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
    }
    SAFE_DESTROY(m_pQimGuiCtx);

    SDL_DestroyRenderer(m_pWndRenderer);
    m_pWndRenderer = nullptr;
    SDL_DestroyWindow(m_pWindow);
    m_pWindow = nullptr;

    m_init = false;
}


void DebuggerApp::updateAppPart(float /*dt*/, float /*time*/)
{
    if (!isWndVisible())
    {
        m_pQimGuiCtx->skipFrame();
        return;
    }

    // ── Centralized refresh trigger ──────────────────────────────────────
    // fetchVmState() snapshots ALL VM data (registers, memory, custom regs)
    // into module caches at a fixed ~15fps. Between ticks, every widget
    // reads the same stale cached values — fully synchronized, no per-widget
    // flags. The Cpu module's fetch() override snapshots registers too, so
    // even getters like getPC()/getRegD() return stable values between ticks.
    //
    // The ImGui frame always renders (never skipFrame) because skipping
    // SDL_RenderPresent on macOS Metal causes an implicit vsync stall that
    // halves the effective frame rate. Redrawing stale cached text at 60fps
    // is negligible CPU — it's just vertex generation for cached glyphs.
    uint64_t now = SDL_GetTicks64();
    if (now - m_lastStateFetchMs >= kStateFetchIntervalMs)
    {
        m_lastStateFetchMs = now;
        getDbg()->fetchVmState();
    }

    m_pQimGuiCtx->newFrame();
    if (m_bFullyInitialized && m_pGui && m_pDebugger)
    {
        m_pGui->drawImGuiMainFrame();
    }
    m_pQimGuiCtx->endFrame();
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
        // Clear to gray before showing to avoid uninitialized framebuffer flash
        SDL_SetRenderDrawColor(m_pWndRenderer, 128, 128, 128, 255);
        SDL_RenderClear(m_pWndRenderer);
        SDL_RenderPresent(m_pWndRenderer);

        SDL_ShowWindow(m_pWindow);
        setPartRenderable(true);
    }
    else
    {
        // Save layout before hiding
        m_pQimGuiCtx->useCurrent();
        ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);

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
