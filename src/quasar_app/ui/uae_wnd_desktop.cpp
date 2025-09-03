#include "uae_wnd_desktop.h"
#include "amDebugger/commonOperations.h"
#include "amDebugger/debuggerOps.h"
#include "amDebugger/vm/vmInterface.h"
#include "qd/app/application.h"
#include "qd/imGui/imGuiContextManager.h"
#include "qd/imGui/imGuiHelperClass.h"
#include "qd/imGui/style/style.h"
#include "qd/qui/controls/menuItemOperation.h"
#include "qd/qui/operationsRegistry.h"
#include "quaesar_operations.h"
#include "quasar_app/quaesar_app.h"
#include "quasar_app/uae_app_imp/uae_client_app_part.h"
#include "uae_options_wnd.h"

#define DLG_TITLE_OPTIONS "Options"


namespace qsr {

void UaeGuiDesktop::init() {
    qd::imGuiApplyStyleDark();
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);  // empty background

    auto pDlg = this->addChild_<qsr::UaeOptionsDlg>(DLG_TITLE_OPTIONS);
    pDlg->setVisible(false);
}


void UaeGuiDesktop::drawContentImp() {
    // Main TOOLBAR
    IVm::VM* vm = getUaeClientApp()->getVm();

    qd::OperationsRegistry& pOpsReg = qd::OperationsRegistry::get();
    pOpsReg.testOperationsShortcuts_<
        // clang-format off
        amD::operation::args::UaeResetAmiga
        , amD::operation::args::ToggleTurboEmulation
        , amD::operation::args::UaeWndAlwaysOnTop
        , qsr::operations::ShowDebuggerWnd
        , qsr::operations::ShowUaeOptionsWnd
        // clang-format on
        >(this);

    if (ImGui::BeginMainMenuBar()) {
        if (auto p1 = qIm::LockMenu("File")) {
            if (ImGui::MenuItem("Open DF0:")) {
                IVm::Floppy* cfgFloppy = vm->floppies[0];
                assert(cfgFloppy);
                qsr::open_file_dlg_select_adf(*cfgFloppy);
                vm->setVmDebugMode(amD::EVmDebugMode::Live);
                doOperationDefault_<amD::operation::args::UaeResetAmiga>();
            }
            qIm::menuItemFromOperationArgs_<qsr::operations::ShowUaeOptionsWnd>(this);
            if (ImGui::MenuItem("Exit")) {
                g_pApp->requestAppToQuit();
            }
        }

        if (auto p2 = qIm::LockMenu("Emulator", true)) {
            qIm::menuItemFromOperationArgs_<amD::operation::args::ToggleTurboEmulation>(this);
            qIm::menuItemFromOperationArgs_<amD::operation::args::UaeWndAlwaysOnTop>(this);
            qIm::menuItemFromOperationArgs_<amD::operation::args::UaeResetAmiga>(this);
        }

        if (auto p2 = qIm::LockMenu("Window", true)) {
            qIm::menuItemFromOperationArgs_<qsr::operations::ShowDebuggerWnd>(this);
        }

        ImGui::EndMainMenuBar();
    }
    return TSuper::drawContentImp();
}


qd::IOperationEnvironment* UaeGuiDesktop::getOpEnvParent() const {
    assert(m_pUaeClientApp);
    return m_pUaeClientApp;
}


qd::EFlow UaeGuiDesktop::setupDefaultOperationArgsImp(qd::operation::args::Base* args) const {
    if (auto p = args->cast_<qsr::operations::ShowDebuggerWnd>()) {
        p->dbgSource = EQuaServerId::S_UAE;
        return EFlow::DONE;
    }
    return EFlow::NO_RESULT;
}


qd::EFlow UaeGuiDesktop::applyOperationMsgProcImp(qd::operation::args::Base* args) {
    if (auto p = args->cast_<qsr::operations::ShowUaeOptionsWnd>()) {
        unused(p);
        qsr::UaeOptionsDlg* pOptionsDlg = this->findChildByIdName_<qsr::UaeOptionsDlg>(DLG_TITLE_OPTIONS);
        IVm::VM* vm = getUaeClientApp()->getVm();
        pOptionsDlg->setVm(vm);
        this->showModal(pOptionsDlg);
        return EFlow::DONE;
    }
    return EFlow::NO_RESULT;
}


};  // namespace qsr
