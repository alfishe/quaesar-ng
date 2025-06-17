#pragma once
#include <amDebugger/ui/ui_view.h>

namespace amD {
namespace window {

class ScreenWnd : public AmDbgWindow {
    QDB_WINDOW_REGISTER(WndId::Screen, amD::window::ScreenWnd, amD::AmDbgWindow);

    ImTextureID mTextureId = 0;

public:
    virtual void onCreate(UiViewCreateCtx* cp) override {
        AmDbgWindow::onCreate(cp);
        m_title = "Screen";
    }

    virtual void drawContentImp() override;

};  // class

};  // namespace window
};  // namespace amD
