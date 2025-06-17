#include "uae_options_wnd.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "qd/stl/algorithm.h"


void UaeOptionsDlg::drawContentImp() {
    ImVec2 rgn = ImGui::GetContentRegionAvail();
    ImVec2 wndL = ImVec2(rgn.x * 0.5f, rgn.y);

    // left column
    uint32_t cldFlg = ImGuiChildFlags_None | ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX;
    if (ImGui::BeginChild("##LEFT_COL", wndL, cldFlg, ImGuiWindowFlags_None)) {
        int items_count = (int)m_pCategories.size();

        const ImGuiContext& g = *ImGui::GetCurrentContext();

        // Calculate size from "height_in_items"
        int height_in_items = ImMin(items_count, 7);
        float height_in_items_f = height_in_items + 0.25f;
        ImVec2 size(0.0f,
                    ImTrunc(ImGui::GetTextLineHeightWithSpacing() * height_in_items_f + g.Style.FramePadding.y * 2.0f));

        if (ImGui::BeginListBox("##OPTIONS CAT", size)) {
            // Assume all items have even height (= 1 line of text). If you need items of different height,
            // you can create a custom version of ListBox() in your code without using the clipper.
            bool value_changed = false;
            ImGuiListClipper clipper;
            clipper.Begin(
                items_count,
                ImGui::GetTextLineHeightWithSpacing());  // We know exactly our line height here so we pass it as a
                                                         // minor optimization, but generally you don't need to.

            int nSelectedCat =
                qd::find_index(m_pCategories, [&](const auto& cat) { return cat.get() == m_pSelectedCat; });

            clipper.IncludeItemByIndex(nSelectedCat);
            while (clipper.Step())
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                    UCategory* pCurrCat = m_pCategories[i].get();
                    const char* item_text = pCurrCat->m_title.c_str();
                    ImGui::PushID(i);
                    const bool bItemSelected = (pCurrCat == m_pSelectedCat);
                    if (ImGui::Selectable(item_text, bItemSelected)) {
                        m_pSelectedCat = pCurrCat;
                        value_changed = true;
                    }
                    if (bItemSelected)
                        ImGui::SetItemDefaultFocus();
                    ImGui::PopID();
                }
            ImGui::EndListBox();
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // right column
    rgn = ImGui::GetContentRegionAvail();
    ImVec2 wndR = ImVec2(rgn.x * 0.0f, rgn.y);
    if (ImGui::BeginChild("##RIGHT_COL", wndR, ImGuiChildFlags_None | ImGuiChildFlags_Borders, ImGuiWindowFlags_None)) {
        ImGui::TextUnformatted("BLTCON1");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-1.f);
        //         ImGui::InputScalar("##BLTCON1", ImGuiDataType_U16, &bltCon1, nullptr, nullptr, "%04X",
        //                            ImGuiInputTextFlags_CharsHexadecimal);
        //
    }
    ImGui::EndChild();
}


UaeOptionsDlg::~UaeOptionsDlg() {
}


void UaeOptionsDlg::onNodeCreated(qd::NodeCreator* mk) {
    TSuper::onNodeCreated(mk);


    UCategory* pCatQuick = createCategory(nullptr, "Quickstart");
    UCategory* pCatHW = createCategory(nullptr, "Hardware");
    UCategory* pCatHost = createCategory(nullptr, "Host");

    UCategory* pCatCpu = createCategory(pCatHW, "CPU");
    UCategory* pCatFloppy = createCategory(pCatHW, "Floppy drives");

    UOption* pFloppy0 = createOption(pCatFloppy, "Floppy 0");
    UOption* pFloppy1 = createOption(pCatFloppy, "Floppy 1");
    UOption* pFloppy2 = createOption(pCatFloppy, "Floppy 2");
}
