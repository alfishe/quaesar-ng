#include "disassembly_wnd.h"
#include "qd/imGui/imGuiHelperClass.h"
#include <amDebugger/debuggerApp.h>
#include <amDebugger/debuggerOps.h>
#include <amDebugger/ui/uiStyle.h>
#include <amDebugger/vm/vmInterface.h>
#include <capstone/capstone.h>
#include <EASTL/fixed_string.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include "qd/ImGui/imgui_eastl.h"
#include "qd/base/variant16.h"
#include "amDebugger/codeAnalyzer/cdaServer.h"
#include "amDebugger/codeAnalyzer/cdaTypes.h"
#include "qd/log/log.h"

namespace amD {
namespace window {


int find_disasm_addr_line_idx(const qd::vector<amD::cda::Item*> &disasm_lines, AddrRef addr)
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
            m_viewBaseAddr = static_cast<AddrRef>(val.getUInt());
            m_reqViewAddr = *m_viewBaseAddr;
            m_snapViewPc = false;
        }
        else
            m_viewBaseAddr.reset();
    }
    ImGui::SameLine();

    AddrRef regPc = vm->cpu->getPC();

    if (ImGui::Button("PC") || (m_prevRegPc != regPc))
    {
        m_viewBaseAddr.reset();
        m_snapViewPc = true;

        if (!qd::is_in_10(regPc, m_addrExtraViewStart, m_addrViewEnd))
        {
            m_reqViewAddrDesiredLine = (int)m_disasmLines.size() / 2; // center of disasm
            m_addrExtraViewStart = regPc - m_reqViewAddrDesiredLine * 8;
            m_reqViewAddr = regPc;
        }
    }
    m_prevRegPc = regPc;

    AddrRef pcAddr = regPc;
    if (m_viewBaseAddr)
        pcAddr = *m_viewBaseAddr;

    float disWndSizeY = ImGui::GetWindowHeight() - 64.f;
    float lineSizeY = ImGui::GetFrameHeightWithSpacing();
    if (disWndSizeY <= 0 || lineSizeY <= 0)
        return;

    int nLinesReq = (int)ceilf(disWndSizeY / lineSizeY) + g_extraScrollLines * 2;

    cda::CodeAnalyzerServer* pCodeServer = &cda::CodeAnalyzerServer::get();
    pCodeServer->requestAnalyzedBlock(vm, m_addrExtraViewStart, nLinesReq, &m_disasmLines, &pcAddr);

    int nReqLine = find_disasm_addr_line_idx(m_disasmLines, m_reqViewAddr);
    //m_reqViewAddrDesiredLine = nReqLine;

    const BreakpointsSortedList& bpList = dbg->getBreakpointsSorted();

    uint32_t offset = 0;
    uint32_t start_disasm = pcAddr - offset;

    static float row_min_height = 0.0f; // for auto height
    int flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
        ImGuiTableFlags_SizingFixedFit; // | ImGuiTableFlags_ScrollY;

    // Disasm Ctrl
    if (ImGui::BeginTable("##disassembly", 4, flags, ImVec2(0, disWndSizeY)))
    {
        ImGui::TableSetupColumn(nullptr/*"##breakpoint"*/,
            ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_NoReorder, 8.f);
        ImGui::TableSetupColumn(nullptr/*"##address"*/);
        ImGui::TableSetupColumn(nullptr/*"##bytes"*/);
        ImGui::TableSetupColumn(nullptr/*"##OpCodes"*/);
        ImGui::TableHeadersRow();

        eastl::fixed_string<char, 255, false> strAddr, strTmp;

        for (size_t i = (size_t)nReqLine; i < m_disasmLines.size(); ++i)
        {
            const cda::Item& entry = *m_disasmLines[i];
            ImGui::TableNextRow(ImGuiTableRowFlags_None, row_min_height);

            AddrRef curAddr = (uint32_t)entry.m_addr;
            if (!ImGui::IsItemVisible())
                continue;
            m_addrViewEnd = curAddr;

            ImGui::PushID(curAddr);
            ImGui::TableSetColumnIndex(0);

            if (curAddr == pcAddr)
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
                operation::args::DisasmToggleBreakpoint p;
                p.address = curAddr;
                p.reg = EReg::PC;
                dbg->applyOperationMsgProc(&p);
            }
            ImGui::TableNextColumn();

            // col:addr
            bool isRowSelected = false;
            strAddr.sprintf("%08X", (uint32_t)curAddr);
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
        const ImGuiKey wheel_key = ImGuiKey_MouseWheelY;
        if (wheel != 0.0f /*&& ImGui::TestKeyOwner(wheel_key, ImGui::GetItemID()) &&*/ )
        {
            if (m_snapViewPc)
            {
                if (qd::is_in_10(regPc, m_addrExtraViewStart, m_addrViewEnd))
                    m_snapViewPc = false;
            }

            if (wheel > 0)
            { // SCROLL DOWN (MWHEEL:FORWARD )
                if (nReqLine > 1)
                {
                    m_addrExtraViewStart -= cda::g_m68MaxOpSize;
                    m_reqViewAddrDesiredLine = nReqLine - 1;
                    m_reqViewAddr = m_disasmLines[nReqLine - 1]->m_addr;
                }
                else
                    assert(0);
            }
            else
            { // SCROLL UP (MWHEEL: BACKWARD)
                m_reqViewAddr = m_disasmLines[nReqLine + 1]->m_addr;
            }
            //qd::logDebug("Wheel:%f", wheel);
        }
        //ImGui::SetKeyOwner(wheel_key, ImGui::GetItemID());
    }

    if (!m_disasmLines.empty())
    {
        if (nReqLine < 0)
            m_addrExtraViewStart = m_reqViewAddr - cda::g_m68MaxOpSize;

        else if (m_reqViewAddrDesiredLine < g_extraScrollLines)
        {
            m_addrExtraViewStart -= cda::g_m68MaxOpSize; // request little more next time
            m_reqViewAddrDesiredLine = g_extraScrollLines + 1;
        }
        else if (nReqLine > g_extraScrollLines)
            m_addrExtraViewStart = m_disasmLines[nReqLine - g_extraScrollLines]->m_addr;
    }
}


qd::EFlow DisassemblyView::applyOperationMsgProc(qd::operation::args::Base* args)
{
    if (auto p = args->cast_<amD::operation::args::DisasmToggleBreakpoint>())
    {
        p->address = getCursorAddr();
        p->reg = EReg::PC;
        getDbg()->applyOperationMsgProc(p);
    }
    return EFlow::NO_RESULT;
}


}; // namespace window
}; // namespace amD
