#pragma once
#include "qd/qui/controls/desktop.h"
#include "qd/qui/uiOperation.h"
#include "qd/typeSystem/typeDeclare.h"

extern class QuasarApp* g_pApp;

class UaeWndDesktop : public qd::UiDesktop, public qd::IOperationEnvironment {
    TS_REFLECT_CLASS(UaeWndDesktop, qd::UiDesktop);

public:
    UaeWndDesktop() {
    }

    void setup();

    virtual void drawContentImp() override;


    virtual IOperationEnvironment* getOpEnvParent() const override;

    virtual void* getOpEnvPtr(const qd::TypeInfo& classType) const override;

};  // class UaeWndDesktop
