#include "dbgGuiDesktop.h"
#include "EASTL/span.h"
#include "qd/ui/controls/lambda.h"
#include "qd/ui/controls/mainMenu.h"
#include <amDebugger/dbgOperation.h>
#include <amDebugger/shortcut/shortcut_list.h>
#include <imgui/imgui_internal.h>
#include "qd/log/log.h"
#include "qd/typeSystem/typeRegistry.h"
#include "qd/ui/uiOperationManager.h"
#include "qd/ui/shortcutMgr.h"
#include "qd/qimGui/controls/qimMenu.h"
#include "amDebugger/commonOperations.h"
#include "imgui/imgui.h"
#include "EASTL/optional.h"



namespace amD {

using namespace operation;

DbgGuiDesktop::DbgGuiDesktop(Debugger* in_dbg)
    : m_pDbg(in_dbg)
{}


void DbgGuiDesktop::onNodeCreated(qd::NodeCreator* mk)
{
    TSuper::onNodeCreated(mk);

    m_pWindows.resize((size_t)WndId::MostCommonCount);
    m_pOperationMgr = createComp_<qd::UiOperationMgr>();
    m_pShortcutMgr = createComp_<qd::ShortcutsMgr>();

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

#endif //


}


void DbgGuiDesktop::createAllUiWndows()
{
    qd::TypeInfoSpan windowTypes = qd::TypeRegistry::get()->findAllDerivedFromTypesCached_<amD::UiWindow>(false);
    for (int i = 0; i < windowTypes.size(); ++i)
    {
        const qd::TypeInfo* pCurWindowType = windowTypes[i];
        auto* pCreateAttr = pCurWindowType->getAttribute_<qd::tsAttr::CreateClassCb>();
        if (!pCreateAttr)
        {
            SDL_Log("Creator not defined in class:'%s'", pCurWindowType->getFullName().c_str());
            continue;
        }
        UiViewCreateCtx cv(this);
        amD::UiWindow* pCurWnd = pCreateAttr->makeInstance_<UiWindow>(cv);
        assert(pCurWnd);
        addView(pCurWnd);
    }
}


DbgGuiDesktop::~DbgGuiDesktop()
{
    assert(m_pWindows.empty());
}


void DbgGuiDesktop::drawImGuiMainFrame()
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

        // draw static nodes
        drawContentImp();
    }
    ImGui::End();

    qim::endFrame();

}


void DbgGuiDesktop::_drawToolBar()
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
        qd::ShortcutsMgr* shMgr = getShortcuts();
        const qd::Shortcut* pCurShortcut;
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



void DbgGuiDesktop::destroy()
{
    TSuper::destroy();
}


void DbgGuiDesktop::_drawMainMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (auto pMenu = qim::beginChild_<qim::UiMenu>("File"))
        {
            //if (auto pItem = pMenu->beginChild_<qim::UiMenuItem>("Item 1")) {}
        }

        if (auto pEmulator = qim::beginChild_<qim::UiMenu>("Emulator"))
        {
            pEmulator->beginChild_<qim::UiMenuOperation>(STRINGIFY(amD::operation::ToggleTurboEmulation));
            pEmulator->beginChild_<qim::UiMenuOperation>(STRINGIFY(amD::operation::UaeWndAlwaysOnTop));
            pEmulator->beginChild_<qim::UiMenuOperation>(STRINGIFY(amD::operation::UaeResetAmiga));
        }

        if (auto pDebug = qim::beginChild_<qim::UiMenu>("Debug"); pDebug->isOpen())
        {
            pDebug->beginChild_<qim::UiMenuOperation>(STRINGIFY(amD::operation::DebugTraceStart));
            ImGui::Separator();
            pDebug->beginChild_<qim::UiMenuOperation>(STRINGIFY(amD::operation::DisasmTraceStep));
            pDebug->beginChild_<qim::UiMenuOperation>(STRINGIFY(amD::operation::DisasmTraceStepOut));
            pDebug->beginChild_<qim::UiMenuOperation>(STRINGIFY(amD::operation::DebugTraceContinue));
            pDebug->beginChild_<qim::UiMenuOperation>(STRINGIFY(amD::operation::DisasmToggleBreakpoint));
            ImGui::Separator();
            pDebug->beginChild_<qim::UiMenuOperation>(STRINGIFY(amD::operation::CopperTraceStep));
            pDebug->beginChild_<qim::UiMenuOperation>(STRINGIFY(amD::operation::CopperToggleBreakpoint));
            ImGui::Separator();

            if (pDebug->isOpen())
            {
                static eastl::optional<operation::DebugDmaOption *> pDebugDmaOp = nullptr;
                if (!pDebugDmaOp.has_value())
                    pDebugDmaOp = getOperationMgr()->getOperation_<amD::operation::DebugDmaOption>();
                if (*pDebugDmaOp)
                {
                    const char* options = "off\0"
                                          "mode 2\0"
                                          "mode 3\0"
                                          "mode 4\0"
                                          "\0";
                    int dmaMode = (*pDebugDmaOp)->getCurDebugDmaMode();
                    int n = dmaMode > 0 ? dmaMode - 1 : 0;
                    if (ImGui::Combo(CC((*pDebugDmaOp)->getName()), &n, options))
                        (*pDebugDmaOp)->changeDebugDmaMode(n);
                }
            }
        }

        if (auto pWindow = qim::beginChild_<qim::UiMenu>("Window"); pWindow->isOpen())
        {
            for (qd::UiNode* pCurWnd : m_pWindows)
            {
                if (!pCurWnd)
                    continue;
                bool bVis = pCurWnd->isVisible();
                if (ImGui::MenuItem(pCurWnd->getText().c_str(), 0, &bVis))
                    pCurWnd->setVisible(bVis);
            }
        }

        ImGui::EndMainMenuBar();
    }
}


}; // namespace amD
