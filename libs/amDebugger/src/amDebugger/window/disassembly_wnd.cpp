#include "disassembly_wnd.h"
#include "amDebugger/debuggerWndApp.h"
#include <amDebugger/debuggerOps.h>
#include <amDebugger/ui/uiStyle.h>
#include <amDebugger/vm/vmInterface.h>
#include <capstone/capstone.h>
#include "qd/stl/string.h"
#include "qd/stl/vector.h"
#include <qd/imGui/imGui.h>
#include "qd/base/variant16.h"
#include "amDebugger/codeAnalyzer/cdaServer.h"
#include "amDebugger/codeAnalyzer/cdaTypes.h"
#include "qd/log/log.h"


namespace amD {
namespace window {


int find_disasm_addr_line_idx(const qtd::vector<amD::cda::Item*> &disasm_lines, AddrRef addr)
{
    int idx = -1;
    for (const cda::Item* pCurItem : disasm_lines)
    {
        ++ idx;
        if (!pCurItem || pCurItem->m_addr != addr)
            continue;
        return idx;
    }
    return -1;
}



AddrRef DisassemblyView::getCursorAddr() const
{
    return m_prevRegPc; // FIXME
}


void DisassemblyView::drawContentImp()
{
    Debugger* dbg = getDbg();
    if (!dbg)
        return;
    IVm::VM* vm = dbg->getVm();
    if (!vm || !vm->isReady())
        return;
    ImGuiContext& g = *ImGui::GetCurrentContext();

    // Read CPU state before using vm. Note: InlineString (255-byte buffer) on
    // the stack can overlap vm pointer in the compiler's stack layout; using a
    // smaller fixed_string for the address input avoids the corruption.
    const AddrRef regPc = vm->cpu->getPC();

    // btn: goto addr
    qd::InlineString_<32> addrStr(m_addrInputStr.getStrVal().begin(), m_addrInputStr.getStrVal().end());
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
            m_addrViewExtraStart = m_mustViewAddr;
            m_nMustViewAddrDesiredLine = g_extraScrollLines;
            m_bSnapViewPc = false;
        }
        else
            m_viewBaseAddr.reset();
    }

    // Re-derive vm pointer after ImGui::InputText, as the InlineString's
    // inline buffer may have corrupted the stack-local vm pointer.
    vm = dbg->getVm();
    if (!vm)
        return;

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();
    if (ImGui::Button("PC") || (m_prevRegPc != regPc))
    {
        m_viewBaseAddr.reset();
        m_bSnapViewPc = true;

        if (!qd::is_in_10(regPc, m_addrViewExtraStart, m_addrViewEnd))
        {
            m_mustViewAddr = regPc;
            m_nMustViewAddrDesiredLine = (int)m_nPrevLineCount / 2; // center of disasm
            m_addrViewExtraStart = qd::clamp_max(m_mustViewAddr - m_nMustViewAddrDesiredLine * 8u, m_mustViewAddr);
        }
    }
    m_prevRegPc = regPc;

    float disWndSizeY = ImGui::GetWindowHeight() - 64.f;
    float lineSizeY = ImGui::GetFrameHeightWithSpacing();
    if (disWndSizeY <= 0 || lineSizeY <= 0)
        return;

    int nLinesReq = (int)ceilf(disWndSizeY / lineSizeY) + g_extraScrollLines * 2;

    // request cached disasm lines
    cda::M68CodeDisassembler* pCodeServer = &cda::M68CodeDisassembler::get();
    AddrRef topViewAddr = m_viewBaseAddr ? *m_viewBaseAddr : regPc;
    pCodeServer->requestM68DisasmLines(vm, m_addrViewExtraStart, nLinesReq, &m_vDisasmLines, &topViewAddr);

    static float row_min_height = 0.0f; // for auto height
    int flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
        ImGuiTableFlags_SizingFixedFit; // | ImGuiTableFlags_ScrollY;

    int nReqLine = find_disasm_addr_line_idx(m_vDisasmLines, m_mustViewAddr);

    // Disasm Ctrl
    if (ImGui::BeginTable("##disassembly", 4, flags, ImVec2(0, disWndSizeY)))
    {
        ImGui::TableSetupColumn(nullptr/*"##breakpoint"*/,
            ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_NoReorder, 8.f);
        ImGui::TableSetupColumn(nullptr/*"##address"*/);
        ImGui::TableSetupColumn(nullptr/*"##bytes"*/);
        ImGui::TableSetupColumn(nullptr/*"##OpCodes"*/);
        ImGui::TableHeadersRow();

        qd::InlineString strAddr, strTmp;

        const BreakpointsSortedList& bpList = dbg->getBreakpointsSorted();

        for (size_t i = (size_t)nReqLine; i < m_vDisasmLines.size(); ++i)
        {
            const cda::Item& entry = *m_vDisasmLines[i];
            ImGui::TableNextRow(ImGuiTableRowFlags_None, row_min_height);

            AddrRef curAddr = (uint32_t)entry.m_addr;
            if (!ImGui::IsItemVisible())
                continue;
            m_addrViewEnd = curAddr;

            ImGui::PushID(curAddr);
            ImGui::TableSetColumnIndex(0);

            if (curAddr == topViewAddr)
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, uiGetColorU(UiStyle::DisasmWnd_PcCursor));

            // col:breakpoint
            strTmp = " ";
            const amD::Breakpoint* curBp;
            curBp = bpList.getBpByAddr(curAddr, EReg::PC);
            if (curBp)
            {
                strTmp = curBp->enabled ? "0" : "O";
            }
            if (ImGui::Selectable(strTmp.c_str(), false, 0, ImVec2(0, row_min_height)))
            {
                operation::DisasmToggleBreakpoint p;
                p.address = curAddr;
                p.reg = EReg::PC;
                dbg->applyOperationMsgProcImp(&p);
            }
            ImGui::TableNextColumn();

            // col:addr
            bool isRowSelected = false;
            qd::string_format_inplace(strAddr, "%08X", (uint32_t)curAddr);
            ImGuiSelectableFlags selectableFlags =
                ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap;
            ImGui::PushStyleColor(ImGuiCol_Text, uiGetColorU(UiStyle::DisasmWnd_Addr));
            if (ImGui::Selectable(strAddr.c_str(), isRowSelected, selectableFlags, ImVec2(0, row_min_height)))
            {
            }
            ImGui::PopStyleColor();
            ImGui::TableNextColumn();

            // col:code bytes
            ImGui::TextColored(uiGetColorF(UiStyle::DisasmWnd_OpCodeBytes), "%s", entry.m_bytesString.c_str());
            ImGui::TableNextColumn();
            // col:instr
            if (cda::CodeItem *pCodeItem = entry.cast_<cda::CodeItem>())
            {
                ImGui::TextUnformatted(pCodeItem->m_text.c_str());
            }
            //ImGui::TableNextColumn();
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    // scroll disasm wnd
    if (ImGui::IsItemHovered(0))
    {
        const float wheel = g.IO.MouseWheel;
        if (wheel != 0.0f /*&& ImGui::TestKeyOwner(wheel_key, ImGui::GetItemID()) &&*/ )
        {
            if (m_bSnapViewPc)
            {
                if (qd::is_in_10(regPc, m_addrViewExtraStart, m_addrViewEnd))
                    m_bSnapViewPc = false;
            }

            if (wheel > 0)
            { // SCROLL DOWN (MWHEEL:FORWARD )
                if (nReqLine > 1)
                {
                    m_addrViewExtraStart = qd::clamp_max(m_addrViewExtraStart - cda::g_maxOpSize, m_addrViewExtraStart);
                    m_nMustViewAddrDesiredLine = nReqLine - 1;
                    m_mustViewAddr = m_vDisasmLines[nReqLine - 1]->m_addr;
                }
                //else //assert(0);
            }
            else
            { // SCROLL UP (MWHEEL: BACKWARD)
                m_mustViewAddr = m_vDisasmLines[nReqLine + 1]->m_addr;
            }
            //qd::logDebug("Wheel:%f", wheel);
        }
        //ImGui::SetKeyOwner(wheel_key, ImGui::GetItemID());
    }

    if (!m_vDisasmLines.empty())
    {
        if (nReqLine < 0)
            m_addrViewExtraStart = qd::clamp_max(m_mustViewAddr - cda::g_maxOpSize, m_mustViewAddr);

        else if (m_nMustViewAddrDesiredLine < g_extraScrollLines)
        {
            // request little more next time
            m_addrViewExtraStart = qd::clamp_max(m_addrViewExtraStart - cda::g_maxOpSize, m_addrViewExtraStart);
            m_nMustViewAddrDesiredLine = g_extraScrollLines + 1;
        }
        else if (nReqLine > g_extraScrollLines)
            m_addrViewExtraStart = m_vDisasmLines[nReqLine - g_extraScrollLines]->m_addr;
    }
    m_nPrevLineCount = (int)m_vDisasmLines.size();
    m_vDisasmLines.clear();
}


qd::EFlow DisassemblyView::applyOperationMsgProcImp(qd::operation::BaseOpArgs* args)
{
    if (auto p = args->cast_<amD::operation::DisasmToggleBreakpoint>())
    {
        p->address = getCursorAddr();
        p->reg = EReg::PC;
        Debugger* dbg = getDbg();
        if (dbg)
            dbg->applyOperationMsgProcImp(p);
    }
    return EFlow::NO_RESULT;
}


}; // namespace window
}; // namespace amD
