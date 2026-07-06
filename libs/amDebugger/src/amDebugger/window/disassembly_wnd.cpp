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
    IVm::VM* vm = dbg->getVm();
    ImGuiContext& g = *ImGui::GetCurrentContext();

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
            m_addrViewExtraStart = m_mustViewAddr;
            m_nMustViewAddrDesiredLine = g_extraScrollLines;
            m_bSnapViewPc = false;
            m_bViewNeedsAdjust = true;
            m_nAdjustAttempts = 0;
        }
        else
            m_viewBaseAddr.reset();
    }
    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();

    AddrRef regPc = vm->cpu->getPC();
    bool bPcChanged = (m_prevRegPc != regPc);
    m_prevRegPc = regPc;

    if (ImGui::Button("PC") || bPcChanged)
    {
        m_viewBaseAddr.reset();
        m_bSnapViewPc = true;

        if (!qd::is_in_10(regPc, m_addrViewExtraStart, m_addrViewEnd))
        {
            m_mustViewAddr = regPc;
            m_nMustViewAddrDesiredLine = (int)m_nPrevLineCount / 2; // center of disasm
            m_addrViewExtraStart = qd::clamp_max(m_mustViewAddr - m_nMustViewAddrDesiredLine * 8u, m_mustViewAddr);
        }
        m_bViewNeedsAdjust = true;
        m_nAdjustAttempts = 0;
    }

    float disWndSizeY = ImGui::GetWindowHeight() - 64.f;
    float lineSizeY = ImGui::GetFrameHeightWithSpacing();
    if (disWndSizeY <= 0 || lineSizeY <= 0)
        return;

    int nLinesReq = (int)ceilf(disWndSizeY / lineSizeY) + g_extraScrollLines * 2;

    // Window resize changes the number of visible lines
    if (nLinesReq != m_nPrevLinesReq)
    {
        m_nPrevLinesReq = nLinesReq;
        m_bViewNeedsAdjust = true;
        m_nAdjustAttempts = 0;
    }

    // request cached disasm lines — ONLY re-fetch when inputs changed.
    // This is the key fix for the pause-stability issue: when UAE is paused,
    // regPc, m_addrViewExtraStart, and nLinesReq don't change between frames,
    // so the cache stays valid and the widget renders identical content
    // without rebuilding capstone output or the ImGui table rows.
    cda::M68CodeDisassembler* pCodeServer = &cda::M68CodeDisassembler::get();
    AddrRef topViewAddr = m_viewBaseAddr ? *m_viewBaseAddr : regPc;
    // Anchor on the real CPU PC (always a genuine instruction boundary), not
    // topViewAddr - that can be a user-typed "go to address" target which
    // isn't necessarily aligned to a real instruction at all.
    {
        bool bCacheHit = m_bDisasmValid
            && m_lastDisasmStart == m_addrViewExtraStart
            && m_lastDisasmLines == nLinesReq
            && m_lastDisasmAnchor == regPc;
        if (!bCacheHit) {
            pCodeServer->requestM68DisasmLines(vm, m_addrViewExtraStart, nLinesReq, &m_vDisasmLines, &regPc);
            m_lastDisasmStart  = m_addrViewExtraStart;
            m_lastDisasmLines  = nLinesReq;
            m_lastDisasmAnchor = regPc;
            m_bDisasmValid = true;
        }
    }

    static float row_min_height = 0.0f; // for auto height
    int flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
        ImGuiTableFlags_SizingFixedFit; // | ImGuiTableFlags_ScrollY;

    int nReqLine = find_disasm_addr_line_idx(m_vDisasmLines, m_mustViewAddr);
    int nDrawStartLine = (nReqLine >= 0) ? nReqLine : 0;

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

        for (size_t i = (size_t)nDrawStartLine; i < m_vDisasmLines.size(); ++i)
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
            }
            else
            { // SCROLL UP (MWHEEL: BACKWARD)
                if (nReqLine >= 0 && (size_t)(nReqLine + 1) < m_vDisasmLines.size())
                    m_mustViewAddr = m_vDisasmLines[nReqLine + 1]->m_addr;
            }
            m_bViewNeedsAdjust = true;
            m_nAdjustAttempts = 0;
        }
        //ImGui::SetKeyOwner(wheel_key, ImGui::GetItemID());
    }

    // View position adjustment — ONLY when something changed.
    // This gate prevents the per-frame feedback loop that caused address drift:
    // the adjustment would fire every frame even when paused, modifying
    // m_addrViewExtraStart, which changed the disasm output next frame,
    // which triggered another adjustment, etc.
    if (m_bViewNeedsAdjust && !m_vDisasmLines.empty())
    {
        bool bAdjusted = false;

        if (nReqLine < 0)
        {
            // Target address not in current view — recenter on it.
            m_addrViewExtraStart = qd::clamp_max(m_mustViewAddr - cda::g_maxOpSize, m_mustViewAddr);
            bAdjusted = true;
        }
        else if (m_nMustViewAddrDesiredLine < g_extraScrollLines)
        {
            // Need more context above the target — shift view up.
            m_addrViewExtraStart = qd::clamp_max(m_addrViewExtraStart - cda::g_maxOpSize, m_addrViewExtraStart);
            m_nMustViewAddrDesiredLine = g_extraScrollLines + 1;
            bAdjusted = true;
        }
        else if (nReqLine > g_extraScrollLines + 1)
        {
            // Target is too far down — shift view so it sits g_extraScrollLines
            // from the top. The +1 deadzone prevents oscillation between two
            // adjacent positions due to variable instruction sizes.
            m_addrViewExtraStart = m_vDisasmLines[nReqLine - g_extraScrollLines]->m_addr;
            bAdjusted = true;
        }

        if (!bAdjusted || ++m_nAdjustAttempts >= m_nMaxAdjustAttempts)
            m_bViewNeedsAdjust = false;

        // If the view start changed, invalidate the disasm cache so the
        // next frame re-fetches with the new address range.
        if (bAdjusted && m_addrViewExtraStart != m_lastDisasmStart)
            m_bDisasmValid = false;
    }

    m_nPrevLineCount = (int)m_vDisasmLines.size();
    // NOTE: do NOT clear m_vDisasmLines here. The pointers are owned by
    // M68CodeDisassembler::m_curItems, which stays alive until the next
    // requestM68DisasmLines() call. Keeping the vector allows the cache
    // check to skip re-fetching when nothing changed (paused state).
}


qd::EFlow DisassemblyView::applyOperationMsgProcImp(qd::operation::BaseOpArgs* args)
{
    if (auto p = args->cast_<amD::operation::DisasmToggleBreakpoint>())
    {
        p->address = getCursorAddr();
        p->reg = EReg::PC;
        getDbg()->applyOperationMsgProcImp(p);
    }
    return EFlow::NO_RESULT;
}


}; // namespace window
}; // namespace amD
