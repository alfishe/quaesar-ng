#pragma once
#include <amDebugger/ui/ui_view.h>

namespace amD {
namespace window {

class ColorsWnd : public UiWindow {
    QDB_WINDOW_REGISTER(WndId::Colors, amD::window::ColorsWnd, amD::UiWindow);

public:
    virtual void onCreate(UiViewCreateCtx* cp) override {
        UiWindow::onCreate(cp);
        m_title = "Palette";
    }

    virtual void drawContentImp() override;

};
//////////////////////////////////////////////////////////////////////////

};  // namespace window
};  // namespace amD
