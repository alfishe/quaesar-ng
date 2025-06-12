#include "gui_manager.h"
#include "EASTL/span.h"
#include "qd/UI/uiControls/lambda.h"
#include "qd/UI/uiControls/mainMenu.h"
#include <amDebugger/dbgOperation.h>
#include <amDebugger/shortcut/shortcut_list.h>
#include <imgui/imgui_internal.h>
#include "qd/Log/log.h"
#include "qd/TypeSystem/typeRegistry.h"
#include "qd/UI/uiOperationManager.h"
#include "qd/UI/shortcutMgr.h"
#include "qd/UImApi/uiImApi.h"
#include "qd/UImApi/uiControls/uiMenu.h"
#include "amDebugger/commonOperations.h"
#include "imgui/imgui.h"



namespace qd {

using namespace operation;

GuiManager::GuiManager(Debugger* in_dbg)
    : m_pDbg(in_dbg)
{}


void GuiManager::onNodeCreated(NodeCreator* mk)
{
    TSuper::onNodeCreated(mk);

    m_pWindows.resize((size_t)WndId::MostCommonCount);
    m_pOperationMgr = createComp_<UiOperationMgr>();
    m_pShortcutMgr = createComp_<ShortcutsMgr>();

    m_pShortcutMgr->init(eastl::span(&shortcut::g_shortcuts_list[0], (size_t)shortcut::EId::MAX_COUNT));

    // create all m_pWindows
    createAllUiWndows();

    operation::AmDebuggerOperationCreator operationCreate;
    operationCreate.gui = this;
    operationCreate.dbg = m_pDbg;
    m_pOperationMgr->createOperations(&operationCreate);

    qim::getContext()->init();

#if 0
    qd::UiMainMenu* pMenu = this->addChild_<qd::UiMainMenu>();

    auto* pFile = pMenu->addChild_<qd::UiMenu>("File");


    auto* pWindow = pMenu->addChild_<qd::UiMenu>("Window");
    {
        pWindow->addChild_<qd::UiLambda>([this]() {
            for (UiView* pCurWnd : m_pWindows)
            {
                if (!pCurWnd)
                    continue;
                bool bVis = pCurWnd->isVisible();
                if (ImGui::MenuItem(pCurWnd->m_title.c_str(), 0, &bVis))
                    pCurWnd->setVisible(bVis);
            }
        });
    }
#endif //


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
    assert(m_pWindows.empty());
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

    qim::beginFrame();

    bool open = true;
    if (ImGui::Begin("Quaesar debugger", &open, wndFlags))
    {

        _drawMainMenuBar();

        _drawToolBar();
        ImGui::DockSpace(ImGui::GetID("DockSpace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);





        //////////////////////////////////////////////////////////////////////////
//         qim::MainMenuBar* pMenu = qim::beginCtrl_<qim::MainMenuBar>();
//         qim::endCtrl(pMenu);


//        ctrl<qd::UiMenu> pMenu = pDebug->get_<qd::UiMenu>();
//        pMenu->operationName = "qd::operation::DebugDmaOption";


        // draw static nodes
        drawContentImp();
    }
    ImGui::End();

    qim::endFrame();

}


void GuiManager::_drawToolBar()
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



void GuiManager::destroy()
{
    TSuper::destroy();

    while (!m_pWindows.empty())
    {
        UiView* curWnd = m_pWindows.back();
        m_pWindows.pop_back();
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
        assert(!m_pWindows[idx] && "already set");
        m_pWindows[idx] = view;
    }
    else
        m_pWindows.push_back(view);
}


void GuiManager::_drawMainMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (auto pMenu = qim::beginCtrl_<qim::UiMenu>("File"))
        {
            if (auto pItem = pMenu->beginChild_<qim::UiMenuItem>("Item 1"))
            {
            }
        }

        if (auto pEmulator = qim::beginCtrl_<qim::UiMenu>("Emulator"))
        {
            pEmulator->beginChild_<qim::UiMenuOperation>(STRINGIFY(qd::operation::ToggleTurboEmulation));
            pEmulator->beginChild_<qim::UiMenuOperation>(STRINGIFY(qd::operation::UaeWndAlwaysOnTop));
            pEmulator->beginChild_<qim::UiMenuOperation>(STRINGIFY(qd::operation::UaeResetAmiga));
        }

        if (auto pDebug = qim::beginCtrl_<qim::UiMenu>("Debug"))
        {
            pDebug->beginChild_<qim::UiMenuOperation>(STRINGIFY(qd::operation::DebugTraceStart));
            ImGui::Separator();
            pDebug->beginChild_<qim::UiMenuOperation>(STRINGIFY(qd::operation::DisasmTraceStep));
            pDebug->beginChild_<qim::UiMenuOperation>(STRINGIFY(qd::operation::DisasmTraceStepOut));
            pDebug->beginChild_<qim::UiMenuOperation>(STRINGIFY(qd::operation::DebugTraceContinue));
            pDebug->beginChild_<qim::UiMenuOperation>(STRINGIFY(qd::operation::DisasmToggleBreakpoint));
            ImGui::Separator();
            pDebug->beginChild_<qim::UiMenuOperation>(STRINGIFY(qd::operation::CopperTraceStep));
            pDebug->beginChild_<qim::UiMenuOperation>(STRINGIFY(qd::operation::CopperToggleBreakpoint));
            ImGui::Separator();

            if (pDebug->isOpen())
            {
                operation::DebugDmaOption* pDebugDmaOp =
                    getOperationMgr()->getOperation_<qd::operation::DebugDmaOption>();
                if (pDebugDmaOp)
                {
                    const char* options = "off\0"
                                          "mode 2\0"
                                          "mode 3\0"
                                          "mode 4\0"
                                          "\0";
                    int dmaMode = pDebugDmaOp->getCurDebugDmaMode();
                    int n = dmaMode > 0 ? dmaMode - 1 : 0;
                    if (ImGui::Combo(CC(pDebugDmaOp->getName()), &n, options))
                        pDebugDmaOp->changeDebugDmaMode(n);
                }
            }
        }
        ImGui::EndMainMenuBar();
    }
}


}; // namespace qd
