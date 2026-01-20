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

void QsrVmClientPlayerGuiDesktop::init() {
    qd::imGuiApplyStyleDark();
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.5f);  // empty background

    auto pDlg = this->addChild_<qsr::UaeOptionsDlg>(DLG_TITLE_OPTIONS);
    pDlg->setVisible(false);
}


void QsrVmClientPlayerGuiDesktop::drawContentImp() {
    // Main TOOLBAR
    qd::OperationsRegistry& pOpsReg = qd::OperationsRegistry::get();

    pOpsReg.testOperationsShortcuts_<  // clang-format off
        amD::operation::VmEmuReset
        , amD::operation::ToggleTurboEmulation
        , amD::operation::VmPlayerWndAlwaysOnTop
        , qsr::operations::ShowDebuggerWnd
        , qsr::operations::ShowUaeOptionsWnd
        >(this);  // clang-format on

    if (ImGui::BeginMainMenuBar()) {
        if (auto p1 = qIm::LockMenu("File")) {
            if (ImGui::MenuItem("Open DF0:")) {
                IVm::VM* vm = getVmOpsHandler()->getVm();
                IVm::Floppy* cfgFloppy = vm ? vm->floppy0 : nullptr;
                if (cfgFloppy) {
                    qsr::open_file_dlg_select_adf(*cfgFloppy);
                    vm->setVmDebugMode(IVm::EVmDebugMode::Live);
                }
                doOperation_<amD::operation::VmEmuReset>();
            }
            qIm::menuItemFromOperationArgs_<qsr::operations::ShowUaeOptionsWnd>(this);
            if (ImGui::MenuItem("Exit")) {
                g_pApp->requestAppToQuit();
            }
        }

        if (auto p2 = qIm::LockMenu("Emulator", true)) {
            qIm::menuItemFromOperationArgs_<amD::operation::ToggleTurboEmulation>(this);
            qIm::menuItemFromOperationArgs_<amD::operation::VmPlayerWndAlwaysOnTop>(this);
            qIm::menuItemFromOperationArgs_<amD::operation::VmEmuReset>(this);
            ImGui::Separator();
            // todo VM providers list
            //m_pVmOpsHandler->getVmProvider()->
        }


        if (auto p2 = qIm::LockMenu("Window", true)) {
            qIm::menuItemFromOperationArgs_<qsr::operations::ShowDebuggerWnd>(this);
        }
        ImGui::EndMainMenuBar();
    }
    return TSuper::drawContentImp();
}


qd::IOperationEnvironment* QsrVmClientPlayerGuiDesktop::getOpEnvParent() const {
    assert(m_pVmOpsHandler);
    return m_pVmOpsHandler;
}


qsr::IVmOperationsHandler* QsrVmClientPlayerGuiDesktop::getVmOpsHandler() const {
    return m_pVmOpsHandler;
}


qd::EFlow QsrVmClientPlayerGuiDesktop::setupDefaultOperationArgsImp(qd::operation::BaseOpArgs* args) const {
    if (auto p = args->cast_<qsr::operations::ShowDebuggerWnd>()) {
        return EFlow::DONE;
    }
    return EFlow::NO_RESULT;
}


qd::EFlow QsrVmClientPlayerGuiDesktop::applyOperationMsgProcImp(qd::operation::BaseOpArgs* args) {
    if (auto p = args->cast_<qsr::operations::ShowUaeOptionsWnd>()) {
        qd::unused(p);
        qsr::UaeOptionsDlg* pOptionsDlg = this->findChildByIdName_<qsr::UaeOptionsDlg>(DLG_TITLE_OPTIONS);
        IVm::VM* vm = getVmOpsHandler()->getVm();
        pOptionsDlg->setVm(vm);
        this->showModal(pOptionsDlg);
        return EFlow::DONE;
    }
    return EFlow::NO_RESULT;
}


};  // namespace qsr
