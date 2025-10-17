#pragma once
#include "amDebugger/vm/vmInterface.h"
#include "qd/qui/controls/desktop.h"
#include "qd/qui/uiOperation.h"
#include "qd/typeSystem/typeDeclare.h"


namespace qsr {
class QsrMainClientWndApp;


// Combined IOperationEnvironment and IVmHandler
class IVmOperationsHandler : public qd::IOperationEnvironment, public IVm::IVmHandler {
public:
    virtual IVm::VM* getVm() const override = 0;
};  // class IVmOperationsHandler


//------------------------------------------------------------------------
class QsrVmClientPlayerGuiDesktop : public qd::UiDesktop, public qd::IOperationEnvironment {
    TS_REFLECT_CLASS(QsrVmClientPlayerGuiDesktop, qd::UiDesktop);
    qsr::QsrMainClientWndApp* m_pVmOpsHandler = nullptr;

public:
    QsrVmClientPlayerGuiDesktop(qsr::QsrMainClientWndApp* pEmuApp) : m_pVmOpsHandler(pEmuApp) {
    }

    void init();

    virtual IOperationEnvironment* getOpEnvParent() const override;

    IVmOperationsHandler* getVmOpsHandler() const;

protected:
    virtual void drawContentImp() override;
    virtual qd::EFlow setupDefaultOperationArgsImp(qd::operation::BaseOpArgs* args) const override;
    virtual qd::EFlow applyOperationMsgProcImp(qd::operation::BaseOpArgs* args) override;

};  // class QsrVmClientPlayerGuiDesktop
//////////////////////////////////////////////////////////////////////////


};  // namespace qsr
