#include "amDebugger/debuggerWndApp.h"
#include "qd/stl/string.h"
#include "qd/stl/fixed_vector.h"
#include "qd/stl/optional.h"
#include "amDebugger/debuggerOps.h"
#include "amDebugger/vm/memory.h"
#include "amDebugger/vm/vmInterface.h"
#include "qd/base/Color.h"
#include <qd/imGui/imGui.h>
#include "amDebugger/ui/uiStyle.h"
#include "amDebugger/ui/uiView.h"
#include "amDebugger/ui/debuggerDesktop.h"
#include "amDebugger/shortcutsList.h"
#include "qd/qui/shortcutMgr.h"
#include "amDebugger/exprValue.h"
#include "amDebugger/codeAnalyzer/copperDisasm.h"


namespace amD {
namespace window {

class CopperDbgWnd : public amD::AmDbgWindow {
    QDB_WINDOW_REGISTER(WndId::CopperDbgWnd, amD::window::CopperDbgWnd, amD::AmDbgWindow);

    amD::ExprValStr m_addrInputStr;
    qd::optional<AddrRef> m_viewBaseAddr;
    AddrRef m_mustViewAddr = 0;
    int m_nMustViewAddrDesiredLine = 0;
    bool m_bSnapViewPc = true;
    AddrRef m_addrViewExtraStart = 0;
    AddrRef m_addrViewEnd = 0;
    AddrRef m_prevRegPc = 0;
    int m_nPrevLineCount = 0;


public:
    virtual void onCreate(UiViewCreateCtx* cp) override {
        AmDbgWindow::onCreate(cp);
        m_title = "Copper debug";
    }

    virtual void drawContentImp() override;

}; // CopperDbgWnd
//////////////////////////////////////////////////////////////////////////




void CopperDbgWnd::drawContentImp() {
    Debugger* dbg = getDbg();
    if (!dbg)
        return;
    IVm::VM* vm = dbg->getVm();
    if (!vm || !vm->isReady())
        return;

    IVm::CustomRegs* custRegs = vm->custom;
    custRegs->fetch();

    // btn: goto addr
    qd::InlineString addrStr(m_addrInputStr.getStrVal().begin(), m_addrInputStr.getStrVal().end());
    if (ImGui::InputText("##disAddr", &addrStr,
            ImGuiInputTextFlags_EscapeClearsAll | ImGuiInputTextFlags_EnterReturnsTrue |
                ImGuiInputTextFlags_AutoSelectAll))
    {
        m_addrInputStr.setStrVal(addrStr);
        qd::Var16 val;
        if (m_addrInputStr.evaluate(vm, val))
        {
            m_viewBaseAddr = static_cast<AddrRef>(val.getU32());
            m_mustViewAddr = *m_viewBaseAddr;
            m_bSnapViewPc = false;
        }
        else
            m_viewBaseAddr.reset();
    }
    ImGui::SameLine();

    AddrRef regPc = vm->copper->getCopperAddr(IVm::CopperAddr_ip);
    if (ImGui::Button("PC") || (m_prevRegPc != regPc))
    {
        m_viewBaseAddr.reset();
        m_bSnapViewPc = true;

        if (!qd::is_in_10(regPc, m_addrViewExtraStart, m_addrViewEnd))
        {
            m_mustViewAddr = regPc;
            m_nMustViewAddrDesiredLine = (int)m_nPrevLineCount / 2; // center of disasm
            m_addrViewExtraStart = m_mustViewAddr - m_nMustViewAddrDesiredLine * 8;
        }
    }
    m_prevRegPc = regPc;


    QImPushFloatLock st;
    st.pushFloat(&ImGui::GetStyle().CellPadding.y, 2);
    static float row_min_height = 0.0f;  // for auto height

    qd::InlineString strAddr;
    qd::InlineString strTmp;

    uint16_t rDmaCon = custRegs->getRegVal(CustReg::DMACONR);

    bool bCopEn = rDmaCon & DMAC::COPEN;
    ImGui::Checkbox("COPEN", &bCopEn);
    ImGui::SameLine();

    if (ImGui::Button("Trace"))
    {
        getUi()->getShortcuts()->triggerShortcut(this, (int)amD::shortcut::EId::CopperToggleBreakpoint);
    }

    AddrRef lc1 = vm->copper->getCopperAddr(IVm::CopperAddr_cop1lc);
    AddrRef lc2 = vm->copper->getCopperAddr(IVm::CopperAddr_cop2lc);
    AddrRef startAddr = m_viewBaseAddr ? *m_viewBaseAddr : (regPc - lc1) < (regPc - lc2) ? lc1 : lc2;
    DecodedCopperList copDec;
    copDec.decodeLines(vm, startAddr, 1024);

    ImVec2 rgn = ImGui::GetContentRegionAvail();
    int flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit |
                ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY;
    if (ImGui::BeginTable("##copperInst", 5, flags, ImVec2(rgn.x, rgn.y))) {
        // ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn(
            "##breakpoint",
            ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_NoReorder, 8.f);
        ImGui::TableSetupColumn("##address");
        ImGui::TableSetupColumn("##bytes");
        ImGui::TableSetupColumn("##copCmd");
        ImGui::TableSetupColumn("##data");
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < copDec.decoded.size(); ++i) {
            const DecodedCopperList::Entry& curEntry = copDec.decoded[i];
            AddrRef curAddr = curEntry.addr;
            ImGui::TableNextRow(ImGuiTableRowFlags_None, row_min_height);
            ImGui::PushID(curAddr);
            ImGui::TableSetColumnIndex(0);

            if (curAddr == regPc)
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, uiGetColorU(UiStyle::DisasmWnd_PcCursor));

            // col:breakpoint
            strTmp = " ";
            if (ImGui::Selectable(strTmp.c_str(), false, 0, ImVec2(0, row_min_height))) {
            }
            ImGui::TableNextColumn();

            // col:addr
            ImGui::TextColored(uiGetColorF(UiStyle::DisasmWnd_Addr), "%06X", curAddr);
            ImGui::TableNextColumn();

            // col:code bytes
            ImGui::TextColored(uiGetColorF(UiStyle::DisasmWnd_OpCodeBytes), "%04X %04X", curEntry.w1, curEntry.w2);
            ImGui::TableNextColumn();

            // col:instr
            ImGui::TextUnformatted(curEntry.strInsn.c_str());
            ImGui::TableNextColumn();

            // col: comment
            ImGui::TextUnformatted(curEntry.comment.c_str());
            // ImGui::TableNextColumn();

            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}

};  // namespace window
};  // namespace amD
