#include "dbgGuiDesktop.h"
#include "amDebugger/commonOperations.h"
#include "EASTL/optional.h"
#include "EASTL/span.h"
#include "qd/imGui/imGuiHelperClass.h"
#include "qd/log/log.h"
#include "qd/qimGui/controls/qimMenu.h"
#include "qd/qui/comps/uiOperationMgrComp.h"
#include "qd/qui/comps/uiShortcutMgrComp.h"
#include "qd/qui/controls/lambda.h"
#include "qd/qui/controls/menuItemOperation.h"
#include "qd/qui/shortcutMgr.h"
#include "qd/qui/uiOperationMgr.h"
#include "qd/typeSystem/typeRegistry.h"
#include <amDebugger/dbgOperation.h>
#include <amDebugger/shortcut_list.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>



namespace amD {

using namespace operation;
inline static const char* dma_options = "off\0"
                                        "mode 2\0"
                                        "mode 3\0"
                                        "mode 4\0"
                                        "\0";


DbgGuiDesktop::DbgGuiDesktop(Debugger* in_dbg)
    : m_pDbg(in_dbg)
{}


void DbgGuiDesktop::_drawMainMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (auto pm = qIm::LockMenu("File"))
        {
        }

        if (auto pm = qIm::LockMenu("Emulator"))
        {
            qIm::menuItemOperation(STRINGIFY(amD::operation::UaeWndAlwaysOnTop));
            qIm::menuItemOperation(STRINGIFY(amD::operation::UaeResetAmiga));
        }

        if (auto pm = qIm::LockMenu("Debug"))
        {
            qIm::menuItemOperation(STRINGIFY(amD::operation::DebugTraceStart));
            ImGui::Separator();
            qIm::menuItemOperation(STRINGIFY(amD::operation::DisasmTraceStep));
            qIm::menuItemOperation(STRINGIFY(amD::operation::DisasmTraceStepOut));
            qIm::menuItemOperation(STRINGIFY(amD::operation::DebugTraceContinue));
            qIm::menuItemOperation(STRINGIFY(amD::operation::DisasmToggleBreakpoint));
            ImGui::Separator();
            qIm::menuItemOperation(STRINGIFY(amD::operation::CopperTraceStep));
            qIm::menuItemOperation(STRINGIFY(amD::operation::CopperToggleBreakpoint));
            ImGui::Separator();

            amD::operation::DebugDmaOption* pDebugDmaOp;
            pDebugDmaOp = getOperationMgr()->getOperation_<amD::operation::DebugDmaOption>();
            if (pDebugDmaOp)
            {
                int dmaMode = pDebugDmaOp->getCurDebugDmaMode();
                int n = dmaMode > 0 ? dmaMode - 1 : 0;
                if (ImGui::Combo(pDebugDmaOp->getName().c_str(), &n, dma_options))
                    pDebugDmaOp->changeDebugDmaMode(n);
            }
        }

        if (auto pEm = qIm::LockMenu("Window"))
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


void DbgGuiDesktop::onNodeCreated(qd::UiNodeCreator* mk)
{
    TSuper::onNodeCreated(mk);

    m_pWindows.resize((size_t)WndId::MostCommonCount);
    m_pOperationMgr = qd::UiOperationMgr::get(); // createComp_<qd::UiOperationMgrComp>()->m_pOpMgr;
    m_pShortcutMgr = qd::ShortcutsMgr::get(); // createComp_<qd::UiShortcutsMgrComp>();
    m_pShortcutMgr->init(eastl::span(&shortcut::g_shortcuts_list[0], (size_t)shortcut::EId::MAX_COUNT));

    // create all m_pWindows
    createAllUiWndows();

    amD::operation::AmDebuggerOperationCreator operationCreate;
    operationCreate.gui = this;
    operationCreate.dbg = m_pDbg;
    m_pOperationMgr->createOperations(&operationCreate);

    qim::createContext();
}


void DbgGuiDesktop::createAllUiWndows()
{
    qd::TypeInfoSpan windowTypes = qd::TypeRegistry::get()->findAllDerivedFromTypesCached_<amD::AmDbgWindow>(false);
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
        amD::AmDbgWindow* pCurWnd = pCreateAttr->makeInstance_<AmDbgWindow>(cv);
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
    getShortcuts()->update(m_pOperationMgr);

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGuiWindowFlags wndFlags = 0;
    wndFlags |= ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    wndFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    // qim::beginFrame();

    bool open = true;
    if (ImGui::Begin("Quaesar debugger", &open, wndFlags))
    {
        _drawMainMenuBar();
        _drawToolBar();
        ImGui::DockSpace(ImGui::GetID("DockSpace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

        // draw static nodes
        this->drawContentImp();
    }
    ImGui::End();

    // qim::endFrame();
}


void DbgGuiDesktop::_drawToolBar()
{
    ImGuiWindowFlags wndFlags = 0;
    wndFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar;
    ImVec2 rgn = ImGui::GetContentRegionAvail();
    if (auto bg = qIm::LockChild("ToolBar", ImVec2(rgn.x, 20.f), ImGuiChildFlags_None, wndFlags))
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

        pCurShortcut = shMgr->getShortcut(shortcut::EId::DebugTraceStepInto);
        if (pCurShortcut)
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
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
}



void DbgGuiDesktop::destroy()
{
    TSuper::destroy();
}



}; // namespace amD
