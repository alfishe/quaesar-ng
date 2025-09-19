#pragma once
#include "qd/qui/controls/desktop.h"
#include "qd/qui/uiOperation.h"
#include "qd/typeSystem/typeDeclare.h"

extern class QuasarApp* g_pApp;

namespace qsr {

class UaeGuiDesktop : public qd::UiDesktop, public qd::IOperationEnvironment {
    TS_REFLECT_CLASS(UaeGuiDesktop, qd::UiDesktop);
    class UaeClientAppPart* m_pUaeClientApp = nullptr;

public:
    UaeGuiDesktop(UaeClientAppPart* pEmuApp) : m_pUaeClientApp(pEmuApp) {
    }

    void init();

    virtual IOperationEnvironment* getOpEnvParent() const override;

    class UaeClientAppPart* getUaeClientApp() const {
        return m_pUaeClientApp;
    }

protected:
    virtual void drawContentImp() override;
    virtual qd::EFlow setupDefaultOperationArgsImp(qd::operation::BaseOpArgs* args) const override;
    virtual qd::EFlow applyOperationMsgProcImp(qd::operation::BaseOpArgs* args) override;
};  // class UaeGuiDesktop


};  // namespace qsr
