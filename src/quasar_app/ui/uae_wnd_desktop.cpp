// clang-format off
#include "sysconfig.h"
#include "sysdeps.h"
#include "uae/time.h"
#include "options.h"
#include "adf.h"
#include "uae.h"
// clang-format on

#include "uae_wnd_desktop.h"
#include "amDebugger/commonOperations.h"
#include "amDebugger/debuggerOps.h"
#include "qd/app/appliction.h"
#include "qd/imGui/imGuiContextManager.h"
#include "qd/imGui/imGuiHelperClass.h"
#include "qd/qui/controls/menuItemOperation.h"
#include "qd/qui/uiOperationMgr.h"
#include "quasar_app/quaesar_app.h"
#include "uae_options_wnd.h"


void UaeWndDesktop::setup() {
    auto pDlg = this->addChild_<UaeOptionsDlg>("options");
    pDlg->setVisible(false);
}


void UaeWndDesktop::drawContentImp() {
    // Main TOOLBAR
    if (ImGui::BeginMainMenuBar()) {
        if (auto p1 = qIm::LockMenu("File")) {
            if (ImGui::MenuItem("Open DF0:")) {
                floppyslot& cfgFloppy = ::changed_prefs.floppyslots[0];
                show_image_file_open_dlg(cfgFloppy);
                doOperation_<amD::operation::args::UaeResetAmiga>();
            }

            if (ImGui::MenuItem("Settings")) {
                UaeOptionsDlg* pOptionsDlg = this->findChildByIdName_<UaeOptionsDlg>("options");
                this->showModal(pOptionsDlg);
            }
            if (ImGui::MenuItem("Exit")) {
                g_pApp->requestAppToQuit();
            }
        }

        if (auto p2 = qIm::LockMenu("Emulator", true)) {
            qIm::menuItemOperation(this, STRINGIFY(amD::operation::ToggleTurboEmulation));
            qIm::menuItemOperation(this, STRINGIFY(amD::operation::UaeWndAlwaysOnTop));
            qIm::menuItemOperation(this, STRINGIFY(amD::operation::UaeResetAmiga));
        }
        ImGui::EndMainMenuBar();
    }


    return TSuper::drawContentImp();
}


qd::IOperationEnvironment* UaeWndDesktop::getOpEnvParent() const {
    assert(0);
    return nullptr;
}


void* UaeWndDesktop::getOpEnvPtr(const qd::TypeInfo& classType) const {
    assert(0);
    return nullptr;
}
