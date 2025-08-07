#pragma once
#include <amDebugger/ui/uiView.h>

namespace amD {
namespace window {

class ColorsWnd : public AmDbgWindow {
    QDB_WINDOW_REGISTER(WndId::Colors, amD::window::ColorsWnd, amD::AmDbgWindow);

public:
    virtual void onCreate(UiViewCreateCtx* cp) override {
        AmDbgWindow::onCreate(cp);
        m_title = "Palette";
    }

    virtual void drawContentImp() override;

};
//////////////////////////////////////////////////////////////////////////

};  // namespace window
};  // namespace amD
