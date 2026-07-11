#include "disassembly_wnd.h"
#include "amDebugger/debuggerWndApp.h"
#include <amDebugger/debuggerOps.h>
#include <amDebugger/ui/uiStyle.h>
#include <amDebugger/vm/vmInterface.h>
#include "capstone/capstone.h"
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
            m_bViewNeedsAdjust = true;
            m_nAdjustAttempts = 0;
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

    bool bPcChanged = (m_prevRegPc != regPc);
    m_prevRegPc = regPc;

    if (ImGui::Button("PC") || (m_bSnapViewPc && bPcChanged))
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
    // Anchor on the real CPU PC (always a genuine instruction boundary) if we are snapped to PC.
    // If not snapped, don't anchor, so we decode exactly from the requested address.
    const AddrRef* pAnchor = m_bSnapViewPc ? &regPc : nullptr;
    AddrRef anchorVal = pAnchor ? *pAnchor : (AddrRef)-1;
    {
        bool bCacheHit = m_bDisasmValid
            && m_lastDisasmStart == m_addrViewExtraStart
            && m_lastDisasmLines == nLinesReq
            && m_lastDisasmAnchor == anchorVal;
        if (!bCacheHit) {
            pCodeServer->requestM68DisasmLines(vm, m_addrViewExtraStart, nLinesReq, &m_vDisasmLines, pAnchor);

#ifndef NDEBUG
            // Sanity: verify addresses are monotonically increasing.
            // If they're not, the disassembler or memory layer has a bug.
            for (size_t k = 1; k < m_vDisasmLines.size(); ++k)
            {
                if (m_vDisasmLines[k]->m_addr < m_vDisasmLines[k - 1]->m_addr)
                {
                    qd::logErr("disasm: non-monotonic address at idx %zu: %08X < %08X",
                        k, (uint32_t)m_vDisasmLines[k]->m_addr, (uint32_t)m_vDisasmLines[k-1]->m_addr);
                    break;
                }
            }
#endif

            m_lastDisasmStart  = m_addrViewExtraStart;
            m_lastDisasmLines  = nLinesReq;
            m_lastDisasmAnchor = anchorVal;
            m_bDisasmValid = true;
        }
    }

    int nReqLine = find_disasm_addr_line_idx(m_vDisasmLines, m_mustViewAddr);
    int nDrawStartLine = (nReqLine >= 0) ? nReqLine : 0;

    // Disassembly table with proper scrolling and clipper-based rendering.
    // The old code lacked ScrollY and used a crude nDrawStartLine hack to
    // skip rows, which left empty TableNextRow entries that confused ImGui's
    // row layout and caused text overlap / visual garbage.
    int flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY;

    if (ImGui::BeginTable("##disassembly", 4, flags, ImVec2(0, disWndSizeY)))
    {
        ImGui::TableSetupColumn("##bp",
            ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 16.f);
        ImGui::TableSetupColumn("##addr", ImGuiTableColumnFlags_WidthFixed, 76.f);
        ImGui::TableSetupColumn("##bytes", ImGuiTableColumnFlags_WidthFixed, 96.f);
        ImGui::TableSetupColumn("##asm", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        qd::InlineString strAddr, strTmp;

        const BreakpointsSortedList& bpList = dbg->getBreakpointsSorted();
        const int nTotalLines = (int)m_vDisasmLines.size();

        ImGuiListClipper clipper;
        clipper.Begin(nTotalLines);
        while (clipper.Step())
        {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
            {
                const cda::Item& entry = *m_vDisasmLines[i];
                ImGui::TableNextRow();

                AddrRef curAddr = (uint32_t)entry.m_addr;

                // Track view end from the last visible row.
                if (i == clipper.DisplayEnd - 1)
                    m_addrViewEnd = curAddr;

                // Use row index for unique PushID — avoids ImGui ID conflicts
                // that occurred when using addresses (e.g. duplicate data bytes
                // decoded via SKIPDATA could produce same-address pseudo-insns).
                ImGui::PushID(i);

                // col 0: breakpoint
                ImGui::TableSetColumnIndex(0);
                if (curAddr == topViewAddr)
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0,
                        uiGetColorU(UiStyle::DisasmWnd_PcCursor));

                strTmp = " ";
                if (const amD::Breakpoint* curBp = bpList.getBpByAddr(curAddr, EReg::PC))
                    strTmp = curBp->enabled ? "0" : "O";
                if (ImGui::Selectable(strTmp.c_str(), false))
                {
                    operation::DisasmToggleBreakpoint p;
                    p.address = curAddr;
                    p.reg = EReg::PC;
                    dbg->applyOperationMsgProcImp(&p);
                }

                // col 1: address
                ImGui::TableNextColumn();
                qd::string_format_inplace(strAddr, "%08X", (uint32_t)curAddr);
                ImGui::PushStyleColor(ImGuiCol_Text, uiGetColorU(UiStyle::DisasmWnd_Addr));
                ImGui::TextUnformatted(strAddr.c_str());
                ImGui::PopStyleColor();

                // col 2: code bytes
                ImGui::TableNextColumn();
                ImGui::TextColored(uiGetColorF(UiStyle::DisasmWnd_OpCodeBytes),
                    "%s", entry.m_bytesString.c_str());

                // col 3: instruction text
                ImGui::TableNextColumn();
                if (cda::CodeItem* pCodeItem = entry.cast_<cda::CodeItem>())
                    ImGui::TextUnformatted(pCodeItem->m_text.c_str());

                ImGui::PopID();
            }
        }
        clipper.End();

        // Scroll so the target line is visible (g_extraScrollLines from top).
        if (nReqLine >= 0 && m_bViewNeedsAdjust)
        {
            float lineH = ImGui::GetTextLineHeightWithSpacing();
            float targetY = qd::max(0.f, (nReqLine - g_extraScrollLines) * lineH);
            float curScrollY = ImGui::GetScrollY();
            if (targetY < curScrollY || targetY > curScrollY + ImGui::GetWindowHeight() * 0.5f)
                ImGui::SetScrollY(targetY);
        }

        ImGui::EndTable();
    }

    // scroll disasm wnd via mouse wheel — adjusts m_addrViewExtraStart
    // to fetch a new address range, rather than scrolling within the table.
    if (ImGui::IsItemHovered(0))
    {
        const float wheel = g.IO.MouseWheel;
        if (wheel != 0.0f)
        {
            if (m_bSnapViewPc && qd::is_in_10(regPc, m_addrViewExtraStart, m_addrViewEnd))
                m_bSnapViewPc = false;

            if (wheel > 0)
            { // SCROLL DOWN (MWHEEL: FORWARD)
                m_addrViewExtraStart = qd::clamp_max(m_addrViewExtraStart - cda::g_maxOpSize, m_addrViewExtraStart);
            }
            else
            { // SCROLL UP (MWHEEL: BACKWARD)
                m_addrViewExtraStart = m_addrViewExtraStart + cda::g_maxOpSize;
            }
            m_bViewNeedsAdjust = true;
            m_nAdjustAttempts = 0;
            m_bDisasmValid = false; // force re-fetch with new start address
        }
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
        else if (nReqLine > g_extraScrollLines + 1)
        {
            // Target is too far down in the list — shift the fetch start
            // address up so the target lands near the top of the next fetch.
            m_addrViewExtraStart = m_vDisasmLines[nReqLine - g_extraScrollLines]->m_addr;
            bAdjusted = true;
        }

        if (!bAdjusted || ++m_nAdjustAttempts >= m_nMaxAdjustAttempts)
            m_bViewNeedsAdjust = false;

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
        Debugger* dbg = getDbg();
        if (dbg)
            dbg->applyOperationMsgProcImp(p);
    }
    return EFlow::NO_RESULT;
}


}; // namespace window
}; // namespace amD
