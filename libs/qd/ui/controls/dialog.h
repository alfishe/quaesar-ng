#pragma once
#include "qd/base/base.h"
#include "qd/node/node.h"
#include "qd/stl/vector.h"
#include "qd/typeSystem/typeDeclare.h"
#include "qd/ui/uiNode.h"
#include "qd/ui/controls/window.h"



namespace qd {

class UiDialog : public qd::UiWindow
{
    TS_REFLECT_CLASS(qd::UiDialog, qd::UiWindow);

public:
    UiDialog() = default;
    virtual ~UiDialog() = default;

    virtual void drawImp() override;

}; // class UiDialog


}; // namespace qd
