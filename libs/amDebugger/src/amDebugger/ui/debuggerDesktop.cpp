#include "debuggerDesktop.h"
#include "qd/imGui/imGuiHelperClass.h"
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
    if (m_pDbg)
        return m_pDbg->applyOperationMsgProcImp(args);
    return EFlow::NO_RESULT;
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
            qIm::menuItemFromOperationArgs_<amD::operation::VmPlayerWndAlwaysOnTop>(pDbg);
            qIm::menuItemFromOperationArgs_<amD::operation::VmEmuReset>(pDbg);
        }

        if (auto pm = qIm::LockMenu("Debug"))
        {
            IVm::VM* vm = pDbg->getVm();
            IVm::EVmDebugMode debugMode = vm ? vm->getVmDebugMode().get() : IVm::EVmDebugMode::Live;
            qIm::menuItemFromOperationArgs_<amD::operation::DebugTraceContinue>(pDbg, "", false,
                debugMode.isBreak());
            qIm::menuItemFromOperationArgs_<amD::operation::DebugTraceStart>(pDbg, "", false, debugMode.isLive());
            ImGui::Separator();
            qIm::menuItemFromOperationArgs_<amD::operation::DisasmTraceStepInto>(pDbg);
            qIm::menuItemFromOperationArgs_<amD::operation::DisasmTraceStepOut>(pDbg);
            qIm::menuItemFromOperationArgs_<amD::operation::DisasmToggleBreakpoint>(this);
            ImGui::Separator();
            qIm::menuItemFromOperationArgs_<amD::operation::CopperTraceStep>(pDbg);
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
    if (auto bg = qIm::LockChild("ToolBar", ImVec2(rgn.x, 20.f), ImGuiChildFlags_None, wndFlags))
    {
        ImGuiIO& io = ImGui::GetIO();
        ImGuiWindow* window = ImGui::GetCurrentWindow();

        window->DC.LayoutType = ImGuiLayoutType_Horizontal;

        Debugger* dbg = getDbg();
        qtd::string hint;

        bool isDbgMode = dbg->isDebugActivated();
        if (ImGui::Checkbox("Trace", &isDbgMode))
        {
            dbg->setDebugMode(isDbgMode ? EVmDebugMode::Break : EVmDebugMode::Live);
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

        qd::OperationsRegistry* pOpMgr = &qd::OperationsRegistry::get();
        const qd::operation::OpDesc* pOpDesc;
        pOpDesc = pOpMgr->findOpDesc(amD::operation::DisasmTraceStepInto::CID);
        {
            if (ImGui::ImageButton("##StepInto", my_tex_id, size, uv0, uv1, ImVec4(0, 0, 0, 1)))
            {
                doOperation_<amD::operation::DisasmTraceStepInto>();
            }
            ImGui::SetItemTooltipV(CC(pOpDesc->getShortcutGuiStr()), nullptr);

            //
            ImGui::Separator();
            //

            // wait scanlines
            pOpDesc = pOpMgr->findOpDesc(amD::operation::DebugWaitScanLines::CID);
            int nScanLines = dbg->getWaitScanLines();
            ImGui::SetNextItemWidth(30);
            if (ImGui::InputInt("##skipScanLines", &nScanLines, -1, -1))
            {
                qd::clamp_min_inplace(nScanLines, 1);
                dbg->setWaitScanLines(nScanLines);
            }
            hint = qd::string_format("Scanlines number(%s)", pOpDesc->getShortcutGuiStr());
            ImGui::SetItemTooltipV(hint.c_str(), nullptr);
        }
        // wait button
        ImGui::SameLine();
        if (ImGui::Button("Wait Scanlines"))
        {
            doOperation_<amD::operation::DebugWaitScanLines>();
        }
    }
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
}




}; // namespace amD
