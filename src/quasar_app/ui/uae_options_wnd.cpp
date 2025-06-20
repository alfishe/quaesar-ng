#include "uae_options_wnd.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "qd/log/log.h"
#include "qd/qimGui/controls/qimInputBox.h"
#include "qd/qimGui/qimGui.h"
#include "qd/stl/algorithm.h"
#include "qd/stl/string.h"


void opt_floppy_draw(int nFloppy) {
    ImGui::Text("Floppy %i: Draw callback", nFloppy);

    static int vi = 0;
    QCTRL(qim::InputInt, pCtrl, "test2", &vi) {
        pCtrl->propAdd_<qim::Props::Size>();
        pCtrl->propAdd_<qim::InputInt::Color>().set(qd::Color(qd::Color::YELLOW));
        if (pCtrl->isTextChanged())
            qdlog("Text changed");
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
    ImVec2 wndL = ImVec2(rgn.x * 0.5f, rgn.y);

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
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // right column
    rgn = ImGui::GetContentRegionAvail();
    ImVec2 wndR = ImVec2(rgn.x * 0.0f, rgn.y);
    if (ImGui::BeginChild("##RIGHT_COL", wndR, ImGuiChildFlags_None | ImGuiChildFlags_Borders, ImGuiWindowFlags_None)) {
        if (m_pSelectedCat) {
            switch (m_pSelectedCat->m_id) {
                case EOptionCat::QUICK_START: {
                    ImGui::TextUnformatted("Quick Start Options");
                } break;
                case EOptionCat::FLOPPY: {
                    ImGui::TextUnformatted("Floppy Drives Options");
                    if (m_pSelectedCat->m_pOptions.empty()) {
                        ImGui::TextUnformatted("No floppy drives configured.");
                    } else {
                        for (UOption* pOpt : m_pSelectedCat->m_pOptions) {
                            if (pOpt) {
                                ImGui::Text("Option: %s", pOpt->m_title.c_str());
                                pOpt->m_drawCb();
                                // Here you can add controls for each option
                                // For example, using qim::InputInt or similar controls
                            }
                        }
                    }
                } break;
                default:
                    break;
            }
        }

#if 0
        static int vi = 0;
        QCTRL(qim::InputInt, pCtrl, "test1", &vi) {
            pCtrl->propAdd_<qim::InputInt::StepInt>().step(2).stepFast(200);
            if (pCtrl->isTextChanged())
                qdlog("Text changed");
        }

        QCTRL(qim::InputInt, pCtrl, "test2", &vi) {
            pCtrl->propAdd_<qim::InputInt::Color>().set(qd::Color(qd::Color::YELLOW));
            if (pCtrl->isTextChanged())
                qdlog("Text changed");
        }
        QCTRL(qim::InputInt, pCtrl, "test3", &vi);
        QCTRL(qim::InputInt, pCtrl, "test4", &vi);
#endif  //
    }
    ImGui::EndChild();
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
