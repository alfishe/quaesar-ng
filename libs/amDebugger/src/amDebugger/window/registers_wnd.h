#pragma once
#include <amDebugger/ui/ui_view.h>

namespace amD {
namespace window {
class RegistersView : public amD::UiWindow {
    QDB_WINDOW_REGISTER(WndId::Registers, amD::window::RegistersView, amD::UiWindow);

public:
    virtual void onCreate(UiViewCreateCtx* cp) override {
        UiWindow::onCreate(cp);
        m_title = "Registers";
    }

    virtual void drawContentImp() override;

}; // RegistersView

};  // namespace window
};  // namespace amD
