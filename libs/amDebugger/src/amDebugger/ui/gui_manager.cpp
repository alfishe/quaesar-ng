#include "gui_manager.h"
#include "EASTL/span.h"
#include "qdIce/qdUI/uiControls/lambda.h"
#include "qdIce/qdUI/uiControls/mainMenu.h"
#include <amDebugger/dbgOperation.h>
#include <amDebugger/shortcut/shortcut_list.h>
#include <imgui/imgui_internal.h>
#include "qdIce/qdLog/log.h"
#include "qdIce/qdTypeSystem/typeRegistry.h"
#include "qdIce/qdUI/uiOperationManager.h"
#include "qdIce/qdUI/shortcutMgr.h"



namespace qd {

using namespace operation;

GuiManager::GuiManager(Debugger* in_dbg)
    : m_pDbg(in_dbg)
{}


void GuiManager::onNodeCreated(NodeCreator* mk)
{
    TSuper::onNodeCreated(mk);

    windows.resize((size_t)WndId::MostCommonCount);
    m_pOperationMgr = createComp_<UiOperationMgr>();
    m_pShortcutMgr = createComp_<ShortcutsMgr>();

    m_pShortcutMgr->init(eastl::span(&shortcut::g_shortcuts_list[0], (size_t)shortcut::EId::MAX_COUNT));

    // create all windows
    createAllUiWndows();

    operation::AmDebuggerOperationCreator operationCreate;
    operationCreate.gui = this;
    operationCreate.dbg = m_pDbg;
    m_pOperationMgr->createOperations(&operationCreate);

    qd::UiMainMenu* pMenu = this->addChild_<qd::UiMainMenu>();

    auto* pFile = pMenu->addChild_<qd::UiMenu>("File");
    auto* pEmulator = pMenu->addChild_<qd::UiMenu>("Emulator");
    {
        pEmulator->addChild_<qd::UiMenuOperation>(STRINGIFY(qd::operation::ToggleTurboEmulation));
        pEmulator->addChild_<qd::UiMenuOperation>(STRINGIFY(qd::operation::UaeWndAlwaysOnTop));
        pEmulator->addChild_<qd::UiMenuOperation>(STRINGIFY(qd::operation::UaeResetAmiga));
    }

    auto* pDebug = pMenu->addChild_<qd::UiMenu>("Debug");
    {
        pDebug->addChild_<qd::UiMenuOperation>(STRINGIFY(qd::operation::DebugTraceStart));
        pDebug->addChild_<qd::UiSeparator>();
        pDebug->addChild_<qd::UiMenuOperation>(STRINGIFY(qd::operation::DisasmTraceStep));
        pDebug->addChild_<qd::UiMenuOperation>(STRINGIFY(qd::operation::DisasmTraceStepOut));
        pDebug->addChild_<qd::UiMenuOperation>(STRINGIFY(qd::operation::DebugTraceContinue));
        pDebug->addChild_<qd::UiMenuOperation>(STRINGIFY(qd::operation::DisasmToggleBreakpoint));
        pDebug->addChild_<qd::UiSeparator>();
        pDebug->addChild_<qd::UiMenuOperation>(STRINGIFY(qd::operation::CopperTraceStep));
        pDebug->addChild_<qd::UiMenuOperation>(STRINGIFY(qd::operation::CopperToggleBreakpoint));
        pDebug->addChild_<qd::UiSeparator>();
        pDebug->addChild_<qd::UiMenuOperation>(STRINGIFY(qd::operation::DebugDmaOption));

        //typeid();
//         const TypeInfo& act = typeof_<>();
//         assert(act.isDefined());
    }

    auto* pWindow = pMenu->addChild_<qd::UiMenu>("Window");
    {
        pWindow->addChild_<qd::UiLambda>([this]() {
            for (UiView* pCurWnd : windows)
            {
                if (!pCurWnd)
                    continue;
                bool bVis = pCurWnd->isVisible();
                if (ImGui::MenuItem(pCurWnd->m_title.c_str(), 0, &bVis))
                    pCurWnd->setVisible(bVis);
            }
        });
    }
}


void GuiManager::createAllUiWndows()
{
    qd::TypeInfoSpan windowTypes = qd::TypeRegistry::get()->findAllDerivedFromTypesCached_<qd::UiWindow>(false);
    for (int i = 0; i < windowTypes.size(); ++i)
    {
        const qd::TypeInfo* pCurWindowType = windowTypes[i];
        auto* pCreateAttr = pCurWindowType->getAttribute_<qd::CreateClassCbAttr>();
        if (!pCreateAttr)
        {
            SDL_Log("Creator not defined in class:'%s'", pCurWindowType->getFullName().c_str());
            continue;
        }
        UiViewCreateCtx cv(this);
        qd::UiWindow* pCurWnd = pCreateAttr->makeInstance_<UiWindow>(cv);
        assert(pCurWnd);
        addView(pCurWnd);
    }
}


GuiManager::~GuiManager()
{
    assert(windows.empty());
}


void GuiManager::drawImGuiMainFrame()
{

    getShortcuts()->update();

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGuiWindowFlags wndFlags = 0;
    wndFlags |= ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    wndFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    bool open = true;
    if (ImGui::Begin("Quaesar debugger", &open, wndFlags))
    {

        drawContentImp();

        _drawMainMenuBar();
        _drawMainToolBar();
        _drawDebuggerWindows();
    }
    ImGui::End();
}


void GuiManager::_drawMainToolBar()
{
    ImGuiWindowFlags wndFlags = 0;
    wndFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar;
    ImVec2 rgn = ImGui::GetContentRegionAvail();
    if (ImGui::BeginChild("ToolBar", ImVec2(rgn.x, 20.f), ImGuiChildFlags_None, wndFlags))
    {
        ImGuiIO& io = ImGui::GetIO();
        ImGuiWindow* window = ImGui::GetCurrentWindow();

        window->DC.LayoutType = ImGuiLayoutType_Horizontal;

        Debugger* dbg = getDbg();
        ShortcutsMgr* shMgr = getShortcuts();
        const Shortcut* pCurShortcut;
        eastl::string hint;

        bool isDbgMode = dbg->isDebugActivated();
        if (ImGui::Checkbox("Trace", &isDbgMode))
        {
            dbg->setDebugMode(isDbgMode ? DebuggerMode_Break : DebuggerMode_Live);
        }
        //
        ImGui::Separator();
        //
        ImTextureID my_tex_id = io.Fonts->TexID;
        float my_tex_w = (float)io.Fonts->TexWidth;
        float my_tex_h = (float)io.Fonts->TexHeight;
        ImVec2 size, uv0, uv1;
        size = ImVec2(16.0f, 16.0f);
        uv0 = ImVec2(0.0f, 0.0f); // TODO ICONS
        uv1 = ImVec2(uv0.x + size.x / my_tex_w, uv1.x + size.y / my_tex_h);

        if (pCurShortcut = shMgr->getShortcut(shortcut::EId::DebugTraceStepInto))
        {
            if (ImGui::ImageButton("##StepInto", my_tex_id, size, uv0, uv1, ImVec4(0, 0, 0, 1)))
            {
                shMgr->triggerShortcut(pCurShortcut);
            }
            ImGui::SetItemTooltipV(pCurShortcut->toString().c_str(), nullptr);

            //
            ImGui::Separator();
            //

            // wait scanlines
            int nScanLines = dbg->getWaitScanLines();
            ImGui::SetNextItemWidth(30);
            if (ImGui::InputInt("##skipScanLines", &nScanLines, -1, -1))
            {
                if (nScanLines < 0)
                    nScanLines = 1;
                dbg->setWaitScanLines(nScanLines);
            }
            if (pCurShortcut)
                hint.sprintf("Scanlines number(%s)", pCurShortcut->toString().c_str());
            ImGui::SetItemTooltipV(hint.c_str(), nullptr);
        }
        // wait button
        ImGui::SameLine();
        pCurShortcut = shMgr->getShortcut(shortcut::EId::DebugWaitScanLines);
        if (ImGui::Button("Wait Scanlines"))
        {
            shMgr->triggerShortcut(pCurShortcut);
        }
    }
    ImGui::EndChild();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
}


void GuiManager::_drawDebuggerWindows()
{
    ImGui::DockSpace(ImGui::GetID("DockSpace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
    // Debugger windows
    for (size_t i = 0; i < windows.size(); ++i)
    {
        UiView* curWnd = windows[i];
        if (!curWnd || !curWnd->isVisible())
            continue;
        curWnd->draw();
    }
}


void GuiManager::_drawMainMenuBar()
{

#if 0
    auto pOperationMgr = getOperationMgr();
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Emulator")) {
//             dr.drawElement = UiDrawEvent::MainMenu_Emul;
//             pOperationMgr->sendOperationMsgT(dr);

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Debug")) {
//             qd::operation::msg::OnDrawMainMenuItem dr;
//             dr.drawElement = UiDrawEvent::MainMenu_Debug;
//             pOperationMgr->sendOperationMsgT(dr
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Window")) {
            for (UiView* curWnd : windows) {
                if (!curWnd)
                    continue;
                ImGui::MenuItem(curWnd->m_title.c_str(), 0, &curWnd->isVisible());
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
#endif //
}

void GuiManager::destroy()
{
    TSuper::destroy();

    while (!windows.empty())
    {
        UiView* curWnd = windows.back();
        windows.pop_back();
        curWnd->destroy();
        delete curWnd;
    }
}

void GuiManager::addView(UiView* view)
{
    addChild(view);

    uint32_t idx = view->mClassId;
    if (idx < (size_t)WndId::MostCommonCount)
    {
        assert(!windows[idx] && "already set");
        windows[idx] = view;
    }
    else
        windows.push_back(view);
}


}; // namespace qd
