// clang-format off
// #include "sysconfig.h"
// #include "sysdeps.h"
// #include "uae/time.h"
// #include "options.h"
// #include "adf.h"
// #include "uae.h"
// clang-format on

#include "uae_wnd_desktop.h"
#include "amDebugger/commonOperations.h"
#include "amDebugger/debuggerOps.h"
#include "amDebugger/vm/absVM.h"
#include "qd/app/application.h"
#include "qd/imGui/imGuiContextManager.h"
#include "qd/imGui/imGuiHelperClass.h"
#include "qd/qui/controls/menuItemOperation.h"
#include "qd/qui/uiOperationMgr.h"
#include "quaesar_operations.h"
#include "quasar_app/quaesar_app.h"
#include "quasar_app/uae_app_imp/uae_client_app_part.h"
#include "uae_options_wnd.h"

namespace qsr {

void UaeGuiDesktop::init() {
    auto pDlg = this->addChild_<qsr::UaeOptionsDlg>("options");
    pDlg->setVisible(false);
}


void UaeGuiDesktop::drawContentImp() {
    // Main TOOLBAR
    AbsVM::VM* vm = getUaeClientApp()->getVm();
    if (ImGui::BeginMainMenuBar()) {
        if (auto p1 = qIm::LockMenu("File")) {
            if (ImGui::MenuItem("Open DF0:")) {
                AbsVM::Floppy* cfgFloppy = vm->floppySlots[0];
                assert(cfgFloppy);
                qsr::open_file_dlg_select_adf(*cfgFloppy);
                doOperation_<amD::operation::args::UaeResetAmiga>();
            }

            if (ImGui::MenuItem("Settings")) {
                qsr::UaeOptionsDlg* pOptionsDlg = this->findChildByIdName_<qsr::UaeOptionsDlg>("options");
                pOptionsDlg->setVm(vm);
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

        if (auto p2 = qIm::LockMenu("Window", true)) {
            qsr::operation::args::ShowDebuggerWnd s1;
            s1.dbgSource = EQuaServerId::S_UAE;
            qIm::menuItemOperationArgs(this, &s1, "Activate debugger");
        }

        ImGui::EndMainMenuBar();
    }


    return TSuper::drawContentImp();
}


qd::IOperationEnvironment* UaeGuiDesktop::getOpEnvParent() const {
    assert(m_pUaeClientApp);
    return m_pUaeClientApp;
}


void* UaeGuiDesktop::getOpEnvPtr(const qd::TypeInfo& classType) const {
    assert(0);
    return nullptr;
}


};  // namespace qsr
