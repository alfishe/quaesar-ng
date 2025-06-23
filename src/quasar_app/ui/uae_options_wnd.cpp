#include "uae_options_wnd.h"
#include <nfd.h>
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "qd/log/log.h"
#include "qd/qimGui/controls/qimInputBox.h"
#include "qd/qimGui/qimGui.h"
#include "qd/stl/algorithm.h"
#include "qd/stl/string.h"
// clang-format off
#include "sysconfig.h"
#include "sysdeps.h"
#include "uae/time.h"
#include "options.h"
#include "uae_imp/adf.h"
#include "uae.h"
// clang-format on


void opt_floppy_draw(int nFloppy) {
    qd::string strDF = qd::string_format("DF%i:", nFloppy);

    floppyslot& cfgFloppy = ::changed_prefs.floppyslots[nFloppy];
    bool bEnabled = cfgFloppy.dfxtype >= 0;
    if (ImGui::Checkbox(strDF.c_str(), &bEnabled))
        cfgFloppy.dfxtype = bEnabled ? 0 : -1;

    if (bEnabled) {
        ImGui::SameLine();
        if (ImGui::Button("Select image file")) {
            nfdu8char_t* outPath;
            nfdu8filteritem_t filters[2] = {{"Source code", "c,cpp,cc"}, {"Headers", "h,hpp"}};
            nfdopendialogu8args_t args = {0};
            args.filterList = filters;
            args.filterCount = 2;
            nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);
            if (result == NFD_OKAY) {
                strcpy(cfgFloppy.df, outPath);
                NFD_FreePathU8(outPath);
            }
        }
        ImGui::SameLine();
        ImGui::Checkbox("Write-protected", &cfgFloppy.forcedwriteprotect);

        ImGui::SameLine();
        if (ImGui::Button("Eject")) {
            cfgFloppy.dfxtype = -1;
            cfgFloppy.df[0] = 0;
        }
        ImGui::InputText("##Image file", cfgFloppy.df, sizeof(cfgFloppy.df), ImGuiInputTextFlags_EnterReturnsTrue);
    }
}


void draw_option(UOption* pOption) {
    if (pOption->m_drawCb) {
        ImGui::PushID(pOption);
        pOption->m_drawCb();
        ImGui::PopID();
    }
}


void UaeOptionsDlg::onNodeCreated(qd::NodeCreator* mk) {
    TSuper::onNodeCreated(mk);

    setSize({600, 400});

    createCategory(EOptionCat::ROOT, EOptionCat::UNDEF);
    UCategory* pCatQuick = createCategory(EOptionCat::ROOT, EOptionCat::QUICK_START);
    UCategory* pCatHW = createCategory(EOptionCat::ROOT, EOptionCat::HARDWARE);
    UCategory* pCatHost = createCategory(EOptionCat::ROOT, EOptionCat::HOST);

    UCategory* pCatCpu = createCategory(EOptionCat::HARDWARE, EOptionCat::CPU);
    UCategory* pCatFloppy = createCategory(EOptionCat::HARDWARE, EOptionCat::FLOPPY);

    createOption(pCatFloppy, "Floppy 0")->setDrawCallback([]() { opt_floppy_draw(0); });
    createOption(pCatFloppy, "Floppy 1")->setDrawCallback([]() { opt_floppy_draw(1); });
    createOption(pCatFloppy, "Floppy 2")->setDrawCallback([]() { opt_floppy_draw(2); });
    createOption(pCatFloppy, "Floppy 3")->setDrawCallback([]() { opt_floppy_draw(3); });
}


void UaeOptionsDlg::drawContentImp() {
    ImVec2 rgn = ImGui::GetContentRegionAvail();
    rgn.y -= 30;
    ImVec2 wndL = ImVec2(rgn.x * 0.25f, rgn.y);

    // left column
    uint32_t cldFlg = ImGuiChildFlags_None | ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX;
    if (ImGui::BeginChild("##LEFT_COL", wndL, cldFlg, ImGuiWindowFlags_None)) {
        int items_count = (int)m_pCategories.size();

        const ImGuiContext& g = *ImGui::GetCurrentContext();

        // Calculate size from "height_in_items"
        int height_in_items = ImMin(items_count, 7);
        float height_in_items_f = height_in_items + 0.25f;
        ImVec2 size(-FLT_MIN, -FLT_MIN);

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

            qd::InlineString itemName;
            while (clipper.Step())
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
                    if (i + 1 >= EOptionCat::MAX_COUNT)
                        break;
                    UCategory* pCurrCat = getCategoryById(i + 1);
                    if (!pCurrCat)
                        continue;
                    itemName.clear();
                    for (int j = 0; j < pCurrCat->m_ident; ++j)
                        itemName.append("  ");  // indent
                    const char* catName = EOptionCat::to_string(pCurrCat->m_id);
                    itemName.append(catName);
                    ImGui::PushID(i);
                    const bool bItemSelected = (pCurrCat == m_pSelectedCat);
                    if (ImGui::Selectable(itemName.c_str(), bItemSelected)) {
                        m_pSelectedCat = pCurrCat;
                        value_changed = true;
                    }
                    if (bItemSelected)
                        ImGui::SetItemDefaultFocus();
                    ImGui::PopID();
                }
            ImGui::EndListBox();
        }

        ImGui::EndChild();
    }

    ImGui::SameLine();

    // right column
    ImVec2 wndR = ImVec2(0, rgn.y);
    if (ImGui::BeginChild("##RIGHT_COL", wndR, ImGuiChildFlags_None | ImGuiChildFlags_Borders, ImGuiWindowFlags_None)) {
        UCategory* pSelCat = m_pSelectedCat;
        if (pSelCat) {
            switch (pSelCat->m_id) {
                case EOptionCat::QUICK_START: {
                    ImGui::TextUnformatted("Quick Start Options");
                } break;
                case EOptionCat::FLOPPY: {
                    ImGui::TextUnformatted("Floppy Drives Options");
                    if (pSelCat->m_pOptions.empty()) {
                        ImGui::TextUnformatted("No floppy drives configured.");
                    } else {
                        for (UOption* pOpt : pSelCat->m_pOptions) {
                            if (!pOpt)
                                continue;
                            draw_option(pOpt);
                        }
                    }
                } break;
                default:
                    break;
            }
        }
        ImGui::EndChild();
    }

    ImGui::SetCursorPosX(rgn.x - 200);
    if (ImGui::Button("Ok", ImVec2(100, 0)))
        setVisible(false);
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(100, 0)))
        setVisible(false);
}


UCategory* UaeOptionsDlg::createCategory(EOptionCat nParentCat, EOptionCat nOpt) {
    assert(!getCategoryById(nOpt));
    qd::unique_ptr<UCategory> pCategory(new UCategory(nOpt, nParentCat));
    int nIdent = 0;
    UCategory* pParent = getCategoryById(nParentCat);
    if (pParent) {
        nIdent = pParent->m_ident + 1;
        pParent->m_pChildCat.push_back(pCategory.get());
    }
    pCategory->m_ident = nIdent;
    m_pCategories[(size_t)nOpt] = std::move(pCategory);
    return m_pCategories[(size_t)nOpt].get();
}


UaeOptionsDlg::~UaeOptionsDlg() {
}
