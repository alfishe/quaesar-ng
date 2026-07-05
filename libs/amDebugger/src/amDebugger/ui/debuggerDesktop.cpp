#include "debuggerDesktop.h"
#include "qd/imGui/imGuiHelperClass.h"
#include "qd/imGui/faIcons.h"
#include "qd/log/log.h"
#include "qd/qui/comps/uiOperationMgrComp.h"
#include "qd/qui/comps/uiShortcutMgrComp.h"
#include "qd/qui/controls/menuItemOperation.h"
#include "qd/qui/operationsRegistry.h"
#include "qd/qui/shortcutMgr.h"
#include "qd/typeSystem/typeRegistry.h"
#include "qd/stl/optional.h"
#include "qd/stl/span.h"
#include <amDebugger/debuggerOps.h>
#include <amDebugger/shortcutsList.h>
#include <amDebugger/window/disassembly_wnd.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>



namespace amD {

using namespace operation;


qd::IOperationEnvironment* DebuggerDesktop::getOpEnvParent() const
{
    return m_pDbg;
}


qd::EFlow DebuggerDesktop::applyOperationMsgProcImp(qd::operation::BaseOpArgs* args)
{
    if (args->cast_<amD::operation::DisasmToggleBreakpoint>())
    {
        if (auto pWnd = findChildByType_<amD::window::DisassemblyView>())
            return pWnd->applyOperationMsgProcImp(args);
    }
    // Forward all operations to the real emulator thread.
    // Do NOT fall through to the dummy VM — it has no real UAE backend
    // and will crash on emulator-control ops (step, pause, continue, etc.).
    if (m_pDbgApp)
        m_pDbgApp->forwardOpToEmulator(args);

    return EFlow::STOP;
}


DebuggerDesktop::DebuggerDesktop(amD::DebuggerApp* pDbgApp, Debugger* in_dbg)
    : m_pDbgApp(pDbgApp)
    , m_pDbg(in_dbg)
{}


void DebuggerDesktop::_drawMainMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        Debugger* pDbg = m_pDbg;

        if (auto pm = qIm::LockMenu("File"))
        {
        }

        if (auto pm = qIm::LockMenu("Emulator"))
        {
            qIm::menuItemFromOperationArgs_<amD::operation::VmPlayerWndAlwaysOnTop>(this);
            qIm::menuItemFromOperationArgs_<amD::operation::VmEmuReset>(this);
        }

        if (auto pm = qIm::LockMenu("Debug"))
        {
            IVm::VM* vm = pDbg->getVm();
            IVm::EVmDebugMode debugMode = vm ? vm->getVmDebugMode().get() : IVm::EVmDebugMode::Live;
            qIm::menuItemFromOperationArgs_<amD::operation::PauseEmulation>(this, "", false,
                debugMode.isLive());
            qIm::menuItemFromOperationArgs_<amD::operation::DebugTraceContinue>(this, "", false,
                debugMode.isBreak());
            qIm::menuItemFromOperationArgs_<amD::operation::DebugTraceStart>(this, "", false, debugMode.isLive());
            ImGui::Separator();
            // Step/trace commands only make sense when paused (Break mode)
            qIm::menuItemFromOperationArgs_<amD::operation::DisasmTraceStepInto>(this, "", false,
                debugMode.isBreak());
            qIm::menuItemFromOperationArgs_<amD::operation::DisasmTraceStepOut>(this, "", false,
                debugMode.isBreak());
            qIm::menuItemFromOperationArgs_<amD::operation::DisasmToggleBreakpoint>(this);
            ImGui::Separator();
            qIm::menuItemFromOperationArgs_<amD::operation::CopperTraceStep>(this, "", false,
                debugMode.isBreak());
            qIm::menuItemFromOperationArgs_<amD::operation::CopperToggleBreakpoint>(this);
            ImGui::Separator();

            amD::operation::DebugDmaOption debugDmaOp;
            if (vm)
            {
                qd::OperationsRegistry& opMgr = qd::OperationsRegistry::get();
                const qd::operation::OpDesc& opDesc = opMgr.getOpDesc_(&debugDmaOp);
                int dmaMode = vm->emu->getDebugDmaMode();
                int n = dmaMode > 0 ? dmaMode - 1 : 0;
                if (ImGui::Combo(opDesc.m_name.c_str(), &n, debugDmaOp.dma_options))
                    vm->emu->setDebugDmaMode(n);
            }
        }

        // Windows
        if (auto pEm = qIm::LockMenu("Window"))
        {
            for (int i = 0; i < getNumChild(); ++i)
            {
                qd::UiNode* pCurWnd = getChild(i);
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


void DebuggerDesktop::onUiNodeCreated(qd::UiNodeCreator* mk)
{
    TSuper::onUiNodeCreated(mk);

    m_pOperationMgr = &qd::OperationsRegistry::get(); // createComp_<qd::UiOperationMgrComp>()->m_pOpMgr;
    m_pShortcutMgr = qd::ShortcutsMgr::get(); // createComp_<qd::UiShortcutsMgrComp>();
    m_pShortcutMgr->createPredefinedShortcuts(
        qtd::span(amD::shortcut::g_shortcuts_list));

    // create all m_pChilds
    createAllUiWndows();

    amD::operation::AmDebuggerOperationCreator operationCreate;
    operationCreate.gui = this;
    operationCreate.dbg = m_pDbg;
    m_pOperationMgr->createOperations(&operationCreate);
}


void DebuggerDesktop::createAllUiWndows()
{
    qd::TypeRegistry* tpr = qd::TypeRegistry::get();
    qd::TypeInfoSpan windowTypes = tpr->findAllDerivedFromTypesCached_<amD::AmDbgWindow>(false);
    for (size_t i = 0; i < windowTypes.size(); ++i)
    {
        const qd::TypeInfo* pCurWindowType = windowTypes[i];
        auto* pCreateAttr = pCurWindowType->findAttribute_<qd::tsAttr::CreateClassCb>();
        if (!pCreateAttr)
        {
            logErr("Creator not defined in class:'%s'", pCurWindowType->getFullName().c_str());
            continue;
        }
        UiViewCreateCtx cv(this);
        amD::AmDbgWindow* pCurWnd = pCreateAttr->makeInstance_<amD::AmDbgWindow>(&cv);
        assert(pCurWnd);
        addChild(pCurWnd);
    }
}

void DebuggerDesktop::destroy()
{
    TSuper::destroy();
}


DebuggerDesktop::~DebuggerDesktop()
{
}


void DebuggerDesktop::_buildDefaultDockLayout(ImGuiID dockspaceId)
{
    ImGuiID idLeft, idRight;
    ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.48f, &idLeft, &idRight);

    ImGui::DockBuilderDockWindow("Disassembly", idLeft);
    ImGui::DockBuilderDockWindow("Copper debug", idLeft);

    ImGuiID idRightTop, idRightBottom;
    ImGui::DockBuilderSplitNode(idRight, ImGuiDir_Up, 0.66f, &idRightTop, &idRightBottom);

    ImGui::DockBuilderDockWindow("Screen", idRightTop);
    ImGui::DockBuilderDockWindow("Memory graph", idRightTop);
    ImGui::DockBuilderDockWindow("Console", idRightBottom);
    ImGui::DockBuilderDockWindow("Memory", idRightBottom);

    ImGuiID idLeftMain, idLeftMid;
    ImGui::DockBuilderSplitNode(idLeft, ImGuiDir_Left, 0.59f, &idLeftMain, &idLeftMid);

    ImGui::DockBuilderDockWindow("Disassembly", idLeftMain);
    ImGui::DockBuilderDockWindow("Copper debug", idLeftMain);

    ImGuiID idMidTop, idMidBottom;
    ImGui::DockBuilderSplitNode(idLeftMid, ImGuiDir_Up, 0.5f, &idMidTop, &idMidBottom);

    ImGui::DockBuilderDockWindow("Registers", idMidTop);
    ImGui::DockBuilderDockWindow("Palette", idMidTop);
    ImGui::DockBuilderDockWindow("Blitter", idMidTop);
    ImGui::DockBuilderDockWindow("Custom regs", idMidBottom);

    ImGui::DockBuilderFinish(dockspaceId);
}


void DebuggerDesktop::drawImGuiMainFrame()
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGuiWindowFlags wndFlags = 0;
    wndFlags |= ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    wndFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollbar;

    bool open = true;
    if (ImGui::Begin("Quaesar debugger", &open, wndFlags))
    {
        // Only draw content when VM is fully bound - avoids layout jumps during init
        if (m_pDbgApp && m_pDbgApp->m_bFullyInitialized)
        {
            _drawMainMenuBar();
            _drawToolBar();
        }

        ImGuiID dockspaceId = ImGui::GetID("DockSpace");
        ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

        // Build default layout if ini was missing/empty/invalid
        static int s_layoutCheckFrame = 0;
        if (s_layoutCheckFrame < 2)
        {
            s_layoutCheckFrame++;
            if (s_layoutCheckFrame == 2)
            {
                ImGuiDockNode* rootNode = ImGui::DockBuilderGetNode(dockspaceId);
                bool hasChildren = rootNode && (rootNode->ChildNodes[0] || rootNode->ChildNodes[1]);
                if (!hasChildren)
                    _buildDefaultDockLayout(dockspaceId);
            }
        }

        if (m_pDbgApp && m_pDbgApp->m_bFullyInitialized)
        {
            this->drawContentImp();

            qd::OperationsRegistry* pOpMgr = &qd::OperationsRegistry::get();
            pOpMgr->testOperationsShortcuts_<
                // clang-format off
                  amD::operation::DisasmTraceStepInto
                , amD::operation::DebugWaitScanLines
                , amD::operation::VmPlayerWndAlwaysOnTop
                , amD::operation::DebugTraceContinue
                , amD::operation::DebugTraceStart
                , amD::operation::PauseEmulation
                , amD::operation::DisasmToggleBreakpoint
                , amD::operation::CopperTraceStep
                , amD::operation::CopperToggleBreakpoint
                // clang-format on
                >(this);
        }
    }
    ImGui::End();
}


void DebuggerDesktop::_drawToolBar()
{
    ImGuiWindowFlags wndFlags = 0;
    wndFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar;
    ImVec2 rgn = ImGui::GetContentRegionAvail();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.f, 2.f));
    if (auto bg = qIm::LockChild("ToolBar", ImVec2(rgn.x, 26.f), ImGuiChildFlags_None, wndFlags))
    {
        // Vertically center: offset cursor by half remaining space
        float barH = 26.f;
        float btnH = ImGui::GetFrameHeight();
        float padY = (barH - btnH) * 0.5f;
        if (padY > 0.f)
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + padY);
        Debugger* dbg = getDbg();
        IVm::VM* vm = dbg->getVm();
        IVm::EVmDebugMode debugMode = vm ? vm->getVmDebugMode().get() : IVm::EVmDebugMode::Live;
        bool isPaused = debugMode.isBreak();

        qd::OperationsRegistry* pOpMgr = &qd::OperationsRegistry::get();
        const qd::operation::OpDesc* pOpDesc;

        // --- Pause / Continue ---
        if (!isPaused) {
            pOpDesc = pOpMgr->findOpDesc(amD::operation::PauseEmulation::CID);
            if (ImGui::Button(ICON_FA_PAUSE "##Pause"))
                doOperation_<amD::operation::PauseEmulation>();
        } else {
            pOpDesc = pOpMgr->findOpDesc(amD::operation::DebugTraceContinue::CID);
            if (ImGui::Button(ICON_FA_PLAY "##Continue"))
                doOperation_<amD::operation::DebugTraceContinue>();
        }
        ImGui::SetItemTooltipV(CC(pOpDesc ? pOpDesc->getShortcutGuiStr() : ""), nullptr);

        ImGui::SameLine();

        // --- Step Into (enabled only when paused) ---
        pOpDesc = pOpMgr->findOpDesc(amD::operation::DisasmTraceStepInto::CID);
        if (!isPaused) { ImGui::BeginDisabled(); }
        if (ImGui::Button(ICON_FA_FORWARD_STEP "##StepInto"))
            doOperation_<amD::operation::DisasmTraceStepInto>();
        if (!isPaused) { ImGui::EndDisabled(); }
        ImGui::SetItemTooltipV(CC(pOpDesc ? pOpDesc->getShortcutGuiStr() : ""), nullptr);

        ImGui::SameLine();

        // --- Step Out (enabled only when paused) ---
        pOpDesc = pOpMgr->findOpDesc(amD::operation::DisasmTraceStepOut::CID);
        if (!isPaused) { ImGui::BeginDisabled(); }
        if (ImGui::Button(ICON_FA_ARROW_RIGHT_FROM_BRACKET "##StepOut"))
            doOperation_<amD::operation::DisasmTraceStepOut>();
        if (!isPaused) { ImGui::EndDisabled(); }
        ImGui::SetItemTooltipV(CC(pOpDesc ? pOpDesc->getShortcutGuiStr() : ""), nullptr);

        ImGui::SameLine();
        ImGui::Separator();
        ImGui::SameLine();

        // --- Copper Trace Step (enabled only when paused) ---
        pOpDesc = pOpMgr->findOpDesc(amD::operation::CopperTraceStep::CID);
        if (!isPaused) { ImGui::BeginDisabled(); }
        if (ImGui::Button(ICON_FA_FLAG_CHECKERED "##CopperStep"))
            doOperation_<amD::operation::CopperTraceStep>();
        if (!isPaused) { ImGui::EndDisabled(); }
        ImGui::SetItemTooltipV(CC(pOpDesc ? pOpDesc->getShortcutGuiStr() : ""), nullptr);

        ImGui::SameLine();
        ImGui::Separator();
        ImGui::SameLine();

        // --- Reset ---
        pOpDesc = pOpMgr->findOpDesc(amD::operation::VmEmuReset::CID);
        if (ImGui::Button(ICON_FA_ROTATE_RIGHT "##Reset"))
            doOperation_<amD::operation::VmEmuReset>();
        ImGui::SetItemTooltipV(CC(pOpDesc ? pOpDesc->getShortcutGuiStr() : ""), nullptr);

        ImGui::SameLine();

        // --- Turbo ---
        pOpDesc = pOpMgr->findOpDesc(amD::operation::ToggleTurboEmulation::CID);
        if (ImGui::Button(ICON_FA_BOLT "##Turbo"))
            doOperation_<amD::operation::ToggleTurboEmulation>();
        ImGui::SetItemTooltipV(CC(pOpDesc ? pOpDesc->getShortcutGuiStr() : ""), nullptr);

        ImGui::SameLine();
        ImGui::Separator();
        ImGui::SameLine();

        // --- Trace checkbox ---
        bool isDbgMode = dbg->isDebugActivated();
        if (ImGui::Checkbox("Trace", &isDbgMode))
            dbg->setDebugMode(isDbgMode ? EVmDebugMode::Break : EVmDebugMode::Live);

        ImGui::SameLine();
        ImGui::Separator();
        ImGui::SameLine();

        // --- Scanlines ---
        pOpDesc = pOpMgr->findOpDesc(amD::operation::DebugWaitScanLines::CID);
        int nScanLines = dbg->getWaitScanLines();
        ImGui::SetNextItemWidth(30);
        if (ImGui::InputInt("##skipScanLines", &nScanLines, -1, -1))
        {
            qd::clamp_min_inplace(nScanLines, 1);
            dbg->setWaitScanLines(nScanLines);
        }
        qtd::string hint = qd::string_format("Scanlines (%s)", pOpDesc ? pOpDesc->getShortcutGuiStr() : "");
        ImGui::SetItemTooltipV(hint.c_str(), nullptr);
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_CLOCK " Wait"))
            doOperation_<amD::operation::DebugWaitScanLines>();
    }
    ImGui::PopStyleVar();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
}




}; // namespace amD
