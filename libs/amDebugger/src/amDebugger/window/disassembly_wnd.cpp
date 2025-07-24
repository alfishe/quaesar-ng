#include "disassembly_wnd.h"
#include "qd/imGui/imGuiHelperClass.h"
#include <amDebugger/debugger.h>
#include <amDebugger/msg_list.h>
#include <amDebugger/ui/ui_style.h>
#include <amDebugger/vm/vm.h>
#include <capstone/capstone.h>
#include <EASTL/fixed_string.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include "qd/ImGui/imgui_eastl.h"
#include "qd/base/variant16.h"
#include "amDebugger/codeAnalyzer/cdaServer.h"
#include "amDebugger/codeAnalyzer/cdaTypes.h"

namespace amD {
namespace window {

void DisassemblyView::drawContentImp()
{
    Debugger* dbg = getDbg();
    VM* vm = dbg->getVm();

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
            m_addrViewStart = m_addrViewEnd = *m_viewBaseAddr;
            m_snapViewPc = false;
        }
        else
            m_viewBaseAddr.reset();
    }

    AddrRef regPc = vm->cpu->getPC();
    AddrRef pcAddr = regPc;
    if (m_viewBaseAddr)
        pcAddr = *m_viewBaseAddr;

//     int64_t viewAddr64 = (int64_t)pcAddr + (int64_t)m_viewOffsetAddr;
//     pcAddr = (AddrRef)viewAddr64;

    if (m_snapViewPc)
    {
        if (!qd::is_in_10(regPc, m_addrViewStart, m_addrViewEnd))
            m_addrViewStart = m_addrViewEnd = pcAddr;
    }

    uint8_t* pcAddrDat = vm->mem->getRealAddr(pcAddr);

    float disWndSizeY = ImGui::GetWindowHeight() - 64.f;
    float lineSizeY = ImGui::GetTextLineHeight();
    if (disWndSizeY <= 0 || lineSizeY <= 0)
        return;
    int nLinesReq = (int)ceilf(disWndSizeY / lineSizeY);

    cda::CodeAnalyzerServer* pCodeServer = &cda::CodeAnalyzerServer::get();
    pCodeServer->requestAnalyzedBlock(vm, m_addrViewStart, nLinesReq, &m_disasmLines, &pcAddr);

    if (!m_disasmLines.empty())
    {
        m_addrViewStart = m_disasmLines.front()->m_addr;
        m_addrViewEnd = m_disasmLines.back()->m_addr;
    }

    const BreakpointsSortedList& bpList = dbg->getBreakpointsSorted();

    uint32_t offset = 0;
    uint32_t start_disasm = pcAddr - offset;

    size_t instructionCount = 0; //TO
//     cs_insn* instructions = nullptr;
//     uint32_t count_bytes = 80;
//     cs_option(*dbg->m_pCapstone, CS_OPT_SKIPDATA, CS_OPT_ON);
//     size_t instructionCount =
//         cs_disasm(*dbg->m_pCapstone, pcAddrDat - offset, count_bytes, start_disasm, 100, &instructions);

    static float row_min_height = 0.0f; // for auto height
    int flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
                ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY;

    if (ImGui::BeginTable("##disassembly", 4, flags, ImVec2(0, disWndSizeY)))
    {
        // ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("##breakpoint",
            ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_NoReorder, 8.f);
        ImGui::TableSetupColumn("##address");
        ImGui::TableSetupColumn("##bytes");
        ImGui::TableSetupColumn("##OpCodes");
        ImGui::TableHeadersRow();

        eastl::fixed_string<char, 255, false> strAddr, strTmp;

        for (size_t i = 0; i < m_disasmLines.size(); ++i)
        {
            const cda::Item& entry = *m_disasmLines[i];
            ImGui::TableNextRow(ImGuiTableRowFlags_None, row_min_height);
            AddrRef curAddr = (uint32_t)entry.m_addr;
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
                operation::msg::DisasmToggleBreakpoint p;
                p.address = curAddr;
                p.reg = EReg::PC;
                dbg->applyOperationMsg(&p);
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
            ImGui::TableNextColumn();

            ImGui::PopID();
        }
        ImGui::EndTable();
    }
}

}; // namespace window
}; // namespace amD
