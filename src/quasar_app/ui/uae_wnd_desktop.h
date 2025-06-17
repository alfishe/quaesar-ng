#pragma once
#include "qd/typeSystem/typeDeclare.h"
#include "qd/ui/controls/desktop.h"


class UaeWndDesktop : public qd::UiDesktop {
    TS_REFLECT_CLASS(UaeWndDesktop, qd::UiDesktop);

public:
    UaeWndDesktop() {
    }

    virtual void onNodeCreated(qd::NodeCreator* mk) override;


};  // class UaeWndDesktop
