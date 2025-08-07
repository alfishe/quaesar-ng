#pragma once
#include <amDebugger/ui/uiView.h>

namespace amD {
namespace window {
class RegistersView : public amD::AmDbgWindow {
    QDB_WINDOW_REGISTER(WndId::Registers, amD::window::RegistersView, amD::AmDbgWindow);

public:
    virtual void onCreate(UiViewCreateCtx* cp) override {
        AmDbgWindow::onCreate(cp);
        m_title = "Registers";
    }

    virtual void drawContentImp() override;

}; // RegistersView

};  // namespace window
};  // namespace amD
