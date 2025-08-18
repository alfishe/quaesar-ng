#include "debuggerDesktop.h"
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
#include <amDebugger/shortcutsList.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include "amDebugger/debuggerOps.h"
#include "amDebugger/window/disassembly_wnd.h"



namespace amD {

using namespace operation;


qd::IOperationEnvironment* DebuggerDesktop::getOpEnvParent() const
{
    return m_pDbg;
}


qd::EFlow DebuggerDesktop::applyOperationMsgProc(qd::operation::args::Base* args)
{
    if (args->cast_<amD::operation::args::DisasmToggleBreakpoint>())
    {
        if (auto pWnd = findChildByType_<amD::window::DisassemblyView>())
            return pWnd->applyOperationMsgProc(args);
    }
    if (m_pDbg)
        return m_pDbg->applyOperationMsgProc(args);
    return EFlow::NO_RESULT;
}


DebuggerDesktop::DebuggerDesktop(Debugger* in_dbg)
    : m_pDbg(in_dbg)
{}


void DebuggerDesktop::_drawMainMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        qd::IOperationEnvironment* env = this;
        Debugger* pDbg = m_pDbg;
        IVm::VM* vm = pDbg->getVm();

        if (auto pm = qIm::LockMenu("File"))
        {
        }

        if (auto pm = qIm::LockMenu("Emulator"))
        {
            qIm::menuItemOperationArgs_< amD::operation::args::UaeWndAlwaysOnTop>(pDbg);
            qIm::menuItemOperationArgs_ < amD::operation::args::UaeResetAmiga>(pDbg);
        }

        if (auto pm = qIm::LockMenu("Debug"))
        {
            qIm::menuItemOperationArgs_<amD::operation::args::DebugTraceStart>(pDbg);
            ImGui::Separator();
            qIm::menuItemOperationArgs_<amD::operation::args::DisasmTraceStep>(pDbg);
            qIm::menuItemOperationArgs_<amD::operation::args::DisasmTraceStepOut>(pDbg);
            qIm::menuItemOperationArgs_<amD::operation::args::DebugTraceContinue>(pDbg);
            qIm::menuItemOperationArgs_<amD::operation::args::DisasmToggleBreakpoint>(this);
            ImGui::Separator();
            qIm::menuItemOperationArgs_<amD::operation::args::CopperTraceStep>(pDbg);
            qIm::menuItemOperationArgs_<amD::operation::args::CopperToggleBreakpoint>(this);
            ImGui::Separator();

            amD::operation::args::DebugDmaOption debugDmaOp;
            {
                qd::UiOperationMgr& opMgr = qd::UiOperationMgr::get();
                const qd::operation::args::OpDesc& opDesc = opMgr.getOpDesc_(&debugDmaOp);
                int dmaMode = vm->emu->getDebugDmaMode();
                int n = dmaMode > 0 ? dmaMode - 1 : 0;
                if (ImGui::Combo(opDesc.m_name.c_str(), &n, debugDmaOp.dma_options))
                    vm->emu->setDebugDmaMode(n);
            }
        }

        // Windows
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


void DebuggerDesktop::onNodeCreated(qd::UiNodeCreator* mk)
{
    TSuper::onNodeCreated(mk);

    m_pWindows.resize((size_t)WndId::MostCommonCount);
    m_pOperationMgr = &qd::UiOperationMgr::get(); // createComp_<qd::UiOperationMgrComp>()->m_pOpMgr;
    m_pShortcutMgr = qd::ShortcutsMgr::get(); // createComp_<qd::UiShortcutsMgrComp>();
    m_pShortcutMgr->createPredefinedShortcuts(eastl::span(&amD::shortcut::g_shortcuts_list[0], (size_t)amD::shortcut::EId::MAX_COUNT));

    // create all m_pWindows
    createAllUiWndows();

    amD::operation::AmDebuggerOperationCreator operationCreate;
    operationCreate.gui = this;
    operationCreate.dbg = m_pDbg;
    m_pOperationMgr->createOperations(&operationCreate);

    qim::createContext();
}


void DebuggerDesktop::createAllUiWndows()
{
    qd::TypeRegistry* tpr = qd::TypeRegistry::get();
    qd::TypeInfoSpan windowTypes = tpr->findAllDerivedFromTypesCached_<amD::AmDbgWindow>(false);
    for (int i = 0; i < windowTypes.size(); ++i)
    {
        const qd::TypeInfo* pCurWindowType = windowTypes[i];
        auto* pCreateAttr = pCurWindowType->getAttribute_<qd::tsAttr::CreateClassCb>();
        if (!pCreateAttr)
        {
            log_error("Creator not defined in class:'%s'", pCurWindowType->getFullName().c_str());
            continue;
        }
        UiViewCreateCtx cv(this);
        amD::AmDbgWindow* pCurWnd = pCreateAttr->makeInstance_<amD::AmDbgWindow>(cv);
        assert(pCurWnd);
        addWindowNode(pCurWnd);
    }
}


DebuggerDesktop::~DebuggerDesktop()
{
    assert(m_pWindows.empty());
}


void DebuggerDesktop::drawImGuiMainFrame()
{
    getShortcuts()->update(this, m_pOperationMgr);

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


void DebuggerDesktop::_drawToolBar()
{
    qd::IOperationEnvironment* env = this;
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

        pCurShortcut = &shMgr->getShortcut(shortcut::EId::DebugTraceStepInto);
        if (!pCurShortcut->empty())
        {
            if (ImGui::ImageButton("##StepInto", my_tex_id, size, uv0, uv1, ImVec4(0, 0, 0, 1)))
            {
                shMgr->triggerShortcut(env, pCurShortcut);
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
        pCurShortcut = &shMgr->getShortcut(shortcut::EId::DebugWaitScanLines);
        if (ImGui::Button("Wait Scanlines"))
        {
            shMgr->triggerShortcut(env, pCurShortcut);
        }
    }
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);
}



void DebuggerDesktop::destroy()
{
    TSuper::destroy();
}



void* DebuggerDesktop::getOpEnvPtr(const qd::TypeInfo& classType) const
{
    if (classType == m_pDbg->getStaticTypeInfo())
        return m_pDbg;
    assert(0);
    return nullptr;
}


}; // namespace amD
