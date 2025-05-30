#pragma once
#include <amDebugger/ui/ui_view.h>

namespace qd {
namespace window {

class ColorsWnd : public UiWindow {
    QDB_WINDOW_REGISTER(WndId::Colors, qd::window::ColorsWnd, qd::UiWindow);

public:
    virtual void onCreate(UiViewCreateCtx* cp) override {
        UiWindow::onCreate(cp);
        mTitle = "Palette";
    }

    virtual void drawContent() override;

};
//////////////////////////////////////////////////////////////////////////

};  // namespace window
};  // namespace qd
