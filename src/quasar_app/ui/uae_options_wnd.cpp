// clang-format off
#include "sysconfig.h"
#include "sysdeps.h"
#include "uae/time.h"
#include "options.h"
#include "adf.h"
#include "uae.h"
// clang-format on
#include "uae_options_wnd.h"
#include <SDL_video.h>
#include <nfd.h>
#include <nfd_sdl2.h>
#include "amDebugger/vm/vmInterface.h"
#include "qd/imGui/imGui.h"
#include "qd/imGui/imGuiHelperClass.h"
#include "qd/log/log.h"
#include "qd/qui/uiMessages.h"
#include "qd/stl/algorithm.h"
#include "qd/stl/string.h"
#include "qsr_config.h"
#include "qsr_main_wnd_client_app.h"


namespace qsr {

void set_native_window(SDL_Window* sdlWindow, ::nfdwindowhandle_t* nativeWindow) {
    if (!NFD_GetNativeWindowFromSDLWindow(sdlWindow, nativeWindow)) {
        printf("NFD_GetNativeWindowFromSDLWindow failed: %s\n", SDL_GetError());
    }
}


/*extern*/ void open_file_dlg_select_adf(IVm::Floppy& cfgFloppy, SDL_Window* pParentWnd) {
    ::nfdu8filteritem_t filters[1] = {{"Amiga images", "adf,exe,dms,zip"}};
    ::nfdopendialogu8args_t args = {};
    args.filterList = filters;
    args.filterCount = EAArrayCount(filters);
    if (pParentWnd)
        set_native_window(pParentWnd, &args.parentWindow);
    ::nfdu8char_t* outPath = nullptr;
    ::nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);
    if (result == NFD_OKAY) {
        cfgFloppy.setAdfPath(outPath);
        NFD_FreePathU8(outPath);
    }
}


static void draw_opt_floppy_cfg(OptionDrawCtx* ctx, int nFloppy) {
    qd::string strDF = qd::string_format("DF%i:", nFloppy);

    IVm::Floppy* pFloppyCfg = (&ctx->vm->floppy0)[nFloppy];
    bool bEnabled = pFloppyCfg->getEnabled();
    if (ImGui::Checkbox(strDF.c_str(), &bEnabled))
        pFloppyCfg->setEnabled(bEnabled);  // ? 0 : -1;

    if (bEnabled) {
        ImGui::SameLine();
        if (ImGui::Button("Select image file")) {
            open_file_dlg_select_adf(*pFloppyCfg);
        }
        ImGui::SameLine();
        bool wp = pFloppyCfg->getWriteProtect();
        if (ImGui::Checkbox("Write-protected", &wp))
            pFloppyCfg->setWriteProtect(wp);

        ImGui::SameLine();
        if (ImGui::Button("Eject")) {
            pFloppyCfg->setEnabled(false);
            pFloppyCfg->setAdfPath("");
        }
        qd::string adfPath = pFloppyCfg->getAdfPath();
        if (ImGui::InputText("##Image file", &adfPath, ImGuiInputTextFlags_EnterReturnsTrue))
            pFloppyCfg->setAdfPath(adfPath);
    }
}


void BaseOptionsDlg::drawContentImp() {
    ImVec2 rgn = ImGui::GetContentRegionAvail();
    rgn.y -= 30;
    ImVec2 wndL = ImVec2(rgn.x * 0.25f, rgn.y);

    // left column
    uint32_t cldFlg = ImGuiChildFlags_None | ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX;
    if (auto bc = qIm::LockChild("##LEFT_COL", wndL, cldFlg, ImGuiWindowFlags_None)) {
        int items_count = (int)m_pCategories.size();

        ImVec2 size(-FLT_MIN, -FLT_MIN);

        if (ImGui::BeginListBox("##OPTIONS CAT", size)) {
            // Assume all items have even height (= 1 line of text). If you need items of different height,
            // you can create a custom version of ListBox() in your code without using the clipper.
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
                    for (int j = 0; j < pCurrCat->ident; ++j)
                        itemName.append("  ");  // indent
                    const char* catName = EOptionCat::to_string(pCurrCat->id);
                    itemName.append(catName);
                    ImGui::PushID(i);
                    const bool bItemSelected = (pCurrCat == m_pSelectedCat);
                    if (ImGui::Selectable(itemName.c_str(), bItemSelected)) {
                        m_pSelectedCat = pCurrCat;
                    }
                    if (bItemSelected)
                        ImGui::SetItemDefaultFocus();
                    ImGui::PopID();
                }
            ImGui::EndListBox();
        }
    }

    ImGui::SameLine();

    // right column, category's options content
    ImVec2 wndR = ImVec2(0, rgn.y);
    if (auto bc = qIm::LockChild("##RIGHT_COL", wndR, ImGuiChildFlags_None | ImGuiChildFlags_Borders,
                                 ImGuiWindowFlags_None)) {
        OptionDrawCtx ctx;
        ctx.vm = m_pVm;
        ctx.m_pDlg = this;
        UCategory* pSelCat = m_pSelectedCat;
        if (pSelCat) {
            if (!pSelCat->title.empty())
                ImGui::TextUnformatted(pSelCat->title.c_str());

            for (UOption* pOpt : pSelCat->options) {
                if (!pOpt)
                    continue;
                drawOptionContent(&ctx, pOpt);
            }
        }
    }

    // buttons
    ImGui::SetCursorPosX(rgn.x - 200);
    if (ImGui::Button("Ok", ImVec2(100, 0)))
        setVisible(false);
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(100, 0)))
        setVisible(false);
}


EFlow BaseOptionsDlg::onUiNodeMessageProc(qd::UiMessage* in_msg) {
    if (auto p = in_msg->cast_<qd::uiMsg::OnVisibleChanged>()) {
        if (p->m_bVisible)
            BPT();
    }
    return TSuper::onUiNodeMessageProc(in_msg);
}


UCategory* BaseOptionsDlg::createCategory(EOptionCat nOpt, EOptionCat nParentCat) {
    assert(!getCategoryById(nOpt));
    qd::unique_ptr<UCategory> pCategory(new UCategory(nOpt, nParentCat));
    int nIdent = 0;
    UCategory* pParent = getCategoryById(nParentCat);
    if (pParent) {
        nIdent = pParent->ident + 1;
        pParent->childCats.push_back(pCategory.get());
    }
    pCategory->ident = nIdent;
    m_pCategories[(size_t)nOpt] = std::move(pCategory);
    return m_pCategories[(size_t)nOpt].get();
}


void BaseOptionsDlg::drawOptionContent(OptionDrawCtx* ctx, UOption* pOption) {
    if (pOption->drawOptionCb) {
        ImGui::PushID(pOption);
        pOption->drawOptionCb(ctx);
        ImGui::PopID();
    }
}


void UaeOptionsDlg::onUiNodeCreated(qd::UiNodeCreator* mk) {
    TSuper::onUiNodeCreated(mk);

    setSize({600, 400});

    createCategory(EOptionCat::UNDEF, EOptionCat::ROOT);

    if (UCategory* pCat = createCategory(EOptionCat::QUICK_START, EOptionCat::ROOT)) {
        pCat->title = "Quick Start Options";
    }

    //     /*UCategory* pCatHW =*/createCategory(EOptionCat::HARDWARE, EOptionCat::ROOT);

    if (UCategory* pCat = createCategory(EOptionCat::HOST, EOptionCat::ROOT)) {
        pCat->title = "Main options";
        createOption(pCat)->setDrawCb(
            [](OptionDrawCtx*) { ImGui::Checkbox("Use ESC key to Quit", &g_cfg_vm_wnd.quitByEsc); });
    }

    //     /*UCategory* pCatCpu =*/createCategory(EOptionCat::CPU, EOptionCat::HARDWARE);

    if (UCategory* pCat = createCategory(EOptionCat::FLOPPY, EOptionCat::HARDWARE)) {
        pCat->title = "Floppy Drives Options";
        createOption(pCat)->setDrawCb([](OptionDrawCtx* ctx) { draw_opt_floppy_cfg(ctx, 0); });
        createOption(pCat)->setDrawCb([](OptionDrawCtx* ctx) { draw_opt_floppy_cfg(ctx, 1); });
        createOption(pCat)->setDrawCb([](OptionDrawCtx* ctx) { draw_opt_floppy_cfg(ctx, 2); });
        createOption(pCat)->setDrawCb([](OptionDrawCtx* ctx) { draw_opt_floppy_cfg(ctx, 3); });
    }
}


};  // namespace qsr
