#pragma once
#include <amDebugger/ui/ui_view.h>

namespace qd {
namespace window {
class RegistersView : public qd::UiWindow {
    QDB_WINDOW_REGISTER(WndId::Registers, qd::window::RegistersView, qd::UiWindow);

public:
    virtual void onCreate(UiViewCreateCtx* cp) override {
        UiWindow::onCreate(cp);
        m_title = "Registers";
    }

    virtual void drawContentImp() override;

}; // RegistersView

};  // namespace window
};  // namespace qd
