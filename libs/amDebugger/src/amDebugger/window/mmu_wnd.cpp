#include "mmu_wnd.h"
#include "amDebugger/debugger.h"
#include "amDebugger/vm/vmInterface.h"
#include "qd/imGui/imGuiHelperClass.h"
#include <imgui/imgui.h>

namespace amD::window {

void MmuWnd::onCreate(UiViewCreateCtx* cp) {
    AmDbgWindow::onCreate(cp);
    m_title = "MMU";
}

void MmuWnd::drawContentImp() {
    Debugger* pDbg = getDbg();
    if (!pDbg) return;
    
    IVm::VM* pVm = pDbg->getVm();
    if (!pVm || !pVm->cpu) return;

    double t = ImGui::GetTime();
    if (m_lastFetchTime < 0 || (t - m_lastFetchTime) > (1.0 / 15.0)) {
        m_lastFetchTime = t;
        m_cachedCpuModel = pVm->cpu->getCpuModel();
        m_cachedMmuEnabled = pVm->cpu->isMmuEnabled();
        m_cachedPages.clear();
        m_cachedStats = {};
        if (m_cachedMmuEnabled) {
            pVm->cpu->getMmuPages(m_cachedPages, &m_cachedStats);
        }

        m_filteredPages.clear();
        for (const auto& page : m_cachedPages) {
            if (!m_showIdentityPages && page.logical == page.physical && !page.writeProtected && !page.superOnly) {
                continue; // Skip identity mapped pages without special protection
            }
            m_filteredPages.push_back(page);
        }
    }

    ImGui::Text("CPU Model: %d", m_cachedCpuModel);
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 120);
    if (m_cachedMmuEnabled) {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "MMU: ENABLED");
    } else {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "MMU: DISABLED");
    }

    ImGui::Separator();

    if (!m_cachedMmuEnabled) {
        ImGui::TextDisabled("MMU is not enabled or not supported by this CPU model.");
        return;
    }

    if (m_cachedStats.totalMemoryBytes > 0) {
        ImGui::TextDisabled("MMU Structure Size: %u bytes (%u KB)", m_cachedStats.totalMemoryBytes, m_cachedStats.totalMemoryBytes / 1024);
        ImGui::TextDisabled("Allocated Tables: %u Root-Level, %u Pointer-Level, %u Page-Level", m_cachedStats.numRootTables, m_cachedStats.numPtrTables, m_cachedStats.numPageTables);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("In the 68040 MMU architecture:\n"
                              "1 Root-Level table contains up to 128 pointers to Pointer-Level tables.\n"
                              "1 Pointer-Level table contains up to 128 pointers to Page-Level tables.\n"
                              "1 Page-Level table contains up to 64 Page Descriptors.\n"
                              "Thus, %u Page-Level tables can map up to %u individual pages.", 
                              m_cachedStats.numPageTables, m_cachedStats.numPageTables * 64);
        }
        ImGui::TextDisabled("Pages: %zu mapped, %zu shown (filter applied)", m_cachedPages.size(), m_filteredPages.size());
        ImGui::Separator();
    }

    if (ImGui::Checkbox("Show Identity Mapped Pages", &m_showIdentityPages)) {
        // Force refresh immediately on toggle
        m_lastFetchTime = -1.0;
    }

    if (m_filteredPages.empty()) {
        ImGui::Text("No MMU pages match the current filter.");
        return;
    }

    ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
    
    if (ImGui::BeginTable("MmuPageTable", 7, flags)) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Logical", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Physical", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        ImGui::TableSetupColumn("S/U", ImGuiTableColumnFlags_WidthFixed, 30.0f);
        ImGui::TableSetupColumn("Cache", ImGuiTableColumnFlags_WidthFixed, 40.0f);
        ImGui::TableSetupColumn("WP", ImGuiTableColumnFlags_WidthFixed, 30.0f);
        ImGui::TableSetupColumn("M", ImGuiTableColumnFlags_WidthFixed, 30.0f);
        ImGui::TableHeadersRow();

        ImGuiListClipper clipper;
        clipper.Begin(m_filteredPages.size());
        while (clipper.Step()) {
            for (int row_n = clipper.DisplayStart; row_n < clipper.DisplayEnd; row_n++) {
                const auto& page = m_filteredPages[row_n];
                ImGui::TableNextRow();
                
                ImGui::TableNextColumn();
                ImGui::Text("%08X", (uint32_t)page.logical);
                
                ImGui::TableNextColumn();
                ImGui::Text("%08X", (uint32_t)page.physical);
                
                ImGui::TableNextColumn();
                if (page.size == 8192) {
                    ImGui::Text("8K");
                } else if (page.size == 4096) {
                    ImGui::Text("4K");
                } else {
                    ImGui::Text("%u", page.size);
                }
                
                ImGui::TableNextColumn();
                ImGui::Text(page.superOnly ? "S" : "U");
                
                ImGui::TableNextColumn();
                ImGui::Text(page.cacheable ? "C" : "-");
                
                ImGui::TableNextColumn();
                ImGui::Text(page.writeProtected ? "WP" : "-");

                ImGui::TableNextColumn();
                ImGui::Text(page.modified ? "M" : "-");
            }
        }
        ImGui::EndTable();
    }
}

} // namespace amD::window
