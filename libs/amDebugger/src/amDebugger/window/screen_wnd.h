#pragma once
#include <amDebugger/ui/ui_view.h>

namespace qd {
namespace window {

class ScreenWnd : public UiWindow {
    QDB_WINDOW_REGISTER(WndId::Screen, qd::window::ScreenWnd, qd::UiWindow);

    ImTextureID mTextureId = 0;

public:
    virtual void onCreate(UiViewCreateCtx* cp) override {
        UiWindow::onCreate(cp);
        m_title = "Screen";
    }

    virtual void drawContentImp() override;

};  // class

};  // namespace window
};  // namespace qd
