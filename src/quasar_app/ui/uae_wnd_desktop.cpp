#include "uae_wnd_desktop.h"
#include "amDebugger/debuggerOps.h"
#include "amDebugger/vm/vmInterface.h"
#include "qd/app/application.h"
#include "qd/imGui/imGuiContextManager.h"
#include "qd/imGui/imGuiHelperClass.h"
#include "qd/imGui/style/style.h"
#include "qd/qui/controls/menuItemOperation.h"
#include "qd/qui/operationsRegistry.h"
#include "qsr_application.h"
#include "qsr_main_wnd_client_app.h"
#include "qsr_operations.h"
#include "uae_options_wnd.h"

#define DLG_TITLE_OPTIONS "Options"


namespace qsr {

void UaeClientGuiDesktop::init() {
    qd::imGuiApplyStyleDark();
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);  // empty background

    auto pDlg = this->addChild_<qsr::UaeOptionsDlg>(DLG_TITLE_OPTIONS);
    pDlg->setVisible(false);
}


void UaeClientGuiDesktop::drawContentImp() {
    // Main TOOLBAR
    IVm::VM* vm = getUaeClientApp()->getVm();

    qd::OperationsRegistry& pOpsReg = qd::OperationsRegistry::get();
    pOpsReg.testOperationsShortcuts_<
        // clang-format off
        amD::operation::UaeResetAmiga
        , amD::operation::ToggleTurboEmulation
        , amD::operation::UaeWndAlwaysOnTop
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
                vm->setVmDebugMode(IVm::EVmDebugMode::Live);
                doOperation_<amD::operation::UaeResetAmiga>();
            }
            qIm::menuItemFromOperationArgs_<qsr::operations::ShowUaeOptionsWnd>(this);
            if (ImGui::MenuItem("Exit")) {
                g_pApp->requestAppToQuit();
            }
        }

        if (auto p2 = qIm::LockMenu("Emulator", true)) {
            qIm::menuItemFromOperationArgs_<amD::operation::ToggleTurboEmulation>(this);
            qIm::menuItemFromOperationArgs_<amD::operation::UaeWndAlwaysOnTop>(this);
            qIm::menuItemFromOperationArgs_<amD::operation::UaeResetAmiga>(this);
        }

        if (auto p2 = qIm::LockMenu("Window", true)) {
            qIm::menuItemFromOperationArgs_<qsr::operations::ShowDebuggerWnd>(this);
        }

        ImGui::EndMainMenuBar();
    }
    return TSuper::drawContentImp();
}


qd::IOperationEnvironment* UaeClientGuiDesktop::getOpEnvParent() const {
    assert(m_pUaeClientApp);
    return m_pUaeClientApp;
}


qd::EFlow UaeClientGuiDesktop::setupDefaultOperationArgsImp(qd::operation::BaseOpArgs* args) const {
    if (auto p = args->cast_<qsr::operations::ShowDebuggerWnd>()) {
        p->dbgSource = EQuaServerId::S_UAE;
        return EFlow::DONE;
    }
    return EFlow::NO_RESULT;
}


qd::EFlow UaeClientGuiDesktop::applyOperationMsgProcImp(qd::operation::BaseOpArgs* args) {
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
