#include "os_modules_wnd.h"
#include "amDebugger/debugger.h"
#include "amDebugger/debuggerWndApp.h"
#include <imgui/imgui.h>
#include <algorithm>

namespace amD::window {

void OsModulesWnd::onCreate(UiViewCreateCtx* cp) {
    AmDbgWindow::onCreate(cp);
    m_title = "OS Modules";
}

void OsModulesWnd::drawContentImp() {
    Debugger* dbg = getDbg();
    if (!dbg) return;
    
    os::OsIntrospector* intro = dbg->getOsIntro();
    if (!intro || !intro->isOsBooted()) {
        ImGui::TextColored(ImVec4(1,0,0,1), "Non-AmigaOS environment detected, or OS not booted yet.");
        if (intro) {
            uint32_t execBase = intro->readU32(0x00000004);
            uint8_t lnType = intro->readU8(execBase + 8);
            uint32_t namePtr = intro->readU32(execBase + 10);
            std::string name = intro->readCString(namePtr, 64);
            ImGui::Separator();
            ImGui::Text("ExecBase pointer: $%08X", execBase);
            ImGui::Text("Node Type: %d (expected 3)", lnType);
            ImGui::Text("Name Ptr: $%08X", namePtr);
            ImGui::Text("Name: '%s' (expected 'exec.library')", name.c_str());
        }
        return;
    }

    os::KickstartInfo ks = intro->getKickstartInfo();
    ImGui::Text("Kickstart: %d.%d  ExecBase: $%08X", ks.version, ks.revision, ks.execBase);
    ImGui::Text("ROM: $%08X (512KB)  ID: %s", ks.romBase, ks.idString.c_str());
    ImGui::Separator();

    double currentTime = ImGui::GetTime();
    double scanInterval = (m_hasScanned && (!m_cachedLibs.empty() || !m_cachedTags.empty())) ? (1.0 / 15.0) : 1.0;

    if (currentTime - m_lastScanTime >= scanInterval) {
        m_cachedLibs = intro->scanLibraries();
        m_cachedTags = intro->scanRomTags();
        m_hasScanned = true;
        m_lastScanTime = currentTime;
    }

    if (m_hasScanned && ImGui::BeginTabBar("OsModulesTabs")) {
        if (ImGui::BeginTabItem("Libraries")) {
            float libsTableHeight = m_selectedLibName.empty() ? -ImGui::GetFrameHeightWithSpacing() : (ImGui::GetContentRegionAvail().y * 0.6f);
            if (ImGui::BeginTable("LibsTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImVec2(0, libsTableHeight))) {
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Load Base");
                ImGui::TableSetupColumn("LVO Base");
                ImGui::TableSetupColumn("Version");
                ImGui::TableSetupColumn("OpenCnt");
                ImGui::TableSetupColumn("Size");
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableHeadersRow();

                for (const auto& lib : m_cachedLibs) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    bool isSelected = (m_selectedLibName == lib.name);
                    
                    // Use a unique ID for Selectable combining name and address to prevent ImGui ID collisions
                    std::string selectableLabel = lib.name + "##" + std::to_string(lib.baseAddress);
                    
                    if (ImGui::Selectable(selectableLabel.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns)) {
                        m_selectedLibName = lib.name;
                    }
                    ImGui::TableNextColumn();
                    ImGui::Text("$%08X", lib.baseAddress - lib.negSize);
                    ImGui::TableNextColumn();
                    ImGui::Text("$%08X", lib.baseAddress);
                    ImGui::TableNextColumn();
                    ImGui::Text("%d.%d", lib.version, lib.revision);
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", lib.openCount);
                    ImGui::TableNextColumn();
                    ImGui::Text("%u", lib.posSize + lib.negSize);
                }
                ImGui::EndTable();
            }

            // Draw selected library LVOs
            if (!m_selectedLibName.empty()) {
                ImGui::Separator();
                ImGui::Text("Selected: %s", m_selectedLibName.c_str());
                
                auto it = std::find_if(m_cachedLibs.begin(), m_cachedLibs.end(), [&](const os::LibraryInfo& l) { return l.name == m_selectedLibName; });
                if (it != m_cachedLibs.end()) {
                    ImGui::Text("Negative area: $%08X - $%08X (0x%X)", it->baseAddress, it->baseAddress - it->negSize, it->negSize);
                    ImGui::Text("Positive area: $%08X - $%08X (0x%X)", it->baseAddress, it->baseAddress + it->posSize, it->posSize);
                    
                    if (ImGui::BeginTable("LvoTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY, ImVec2(0, -ImGui::GetFrameHeightWithSpacing()))) {
                        ImGui::TableSetupColumn("Offset");
                        ImGui::TableSetupColumn("Function");
                        ImGui::TableSetupColumn("Target");
                        ImGui::TableSetupScrollFreeze(0, 1);
                        ImGui::TableHeadersRow();
                        for (const auto& lvo : it->lvoEntries) {
                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();
                            ImGui::Text("%d", lvo.offset);
                            ImGui::TableNextColumn();
                            ImGui::Text("%s", lvo.funcName.c_str());
                            ImGui::TableNextColumn();
                            ImGui::Text("$%08X", lvo.targetAddress);
                        }
                        ImGui::EndTable();
                    }
                }
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("ROM Tags")) {
            if (ImGui::BeginTable("RomTagsTable", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImVec2(0, -ImGui::GetFrameHeightWithSpacing()))) {
                ImGui::TableSetupColumn("Name");
                ImGui::TableSetupColumn("Type");
                ImGui::TableSetupColumn("Pri");
                ImGui::TableSetupColumn("Ver");
                ImGui::TableSetupColumn("Init Addr");
                ImGui::TableSetupColumn("Size");
                ImGui::TableSetupColumn("ID String");
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableHeadersRow();

                for (const auto& tag : m_cachedTags) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", tag.name.c_str());
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", tag.type);
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", tag.priority);
                    ImGui::TableNextColumn();
                    ImGui::Text("%d", tag.version);
                    ImGui::TableNextColumn();
                    ImGui::Text("$%08X", tag.initFunc);
                    ImGui::TableNextColumn();
                    ImGui::Text("%u", tag.size);
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", tag.idString.c_str());
                }
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
    
    // Bottom status line
    ImGui::Separator();
    if (!m_hasScanned || (m_cachedLibs.empty() && m_cachedTags.empty())) {
        ImGui::TextDisabled("Scanning every 1 second...");
    } else {
        ImGui::TextDisabled("Found %zu libraries and %zu ROM tags", m_cachedLibs.size(), m_cachedTags.size());
    }
}

} // namespace amD::window
