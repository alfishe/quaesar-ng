#pragma once
#include "qd/qui/controls/window.h"



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
