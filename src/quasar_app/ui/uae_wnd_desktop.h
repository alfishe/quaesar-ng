#pragma once
#include "amDebugger/vm/vmInterface.h"
#include "qd/qui/controls/desktop.h"
#include "qd/qui/uiOperation.h"
#include "qd/typeSystem/typeDeclare.h"


namespace qsr {

class IOperationsVmEnvHandler : public qd::IOperationEnvironment, public IVm::IVmHandler {
public:
    virtual IVm::VM* getVm() const = 0;
};  // class IOperationsVmEnvHandler


//------------------------------------------------------------------------
class QsrMainClientGuiDesktop : public qd::UiDesktop, public qd::IOperationEnvironment {
    TS_REFLECT_CLASS(QsrMainClientGuiDesktop, qd::UiDesktop);
    IOperationsVmEnvHandler* m_pMainClientWndApp = nullptr;

public:
    QsrMainClientGuiDesktop(qsr::IOperationsVmEnvHandler* pEmuApp) : m_pMainClientWndApp(pEmuApp) {
    }

    void init();

    virtual IOperationEnvironment* getOpEnvParent() const override;

    IOperationsVmEnvHandler* getUaeClientApp() const {
        return m_pMainClientWndApp;
    }

protected:
    virtual void drawContentImp() override;
    virtual qd::EFlow setupDefaultOperationArgsImp(qd::operation::BaseOpArgs* args) const override;
    virtual qd::EFlow applyOperationMsgProcImp(qd::operation::BaseOpArgs* args) override;

};  // class QsrMainClientGuiDesktop
//////////////////////////////////////////////////////////////////////////


};  // namespace qsr
