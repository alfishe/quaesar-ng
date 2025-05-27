#pragma once
#include <amDebugger/ui/ui_view.h>

namespace qd {
namespace window {
class RegistersView : public UiWindow {
    QDB_CLASS_ID(WndId::Registers);

public:
    virtual void onCreate(UiViewCreateCtx* cp) override {
        UiWindow::onCreate(cp);
        mTitle = "Registers";
    }

    virtual void drawContent() override;

} QDB_WINDOW_REGISTER(RegistersView);  // class

};  // namespace window
};  // namespace qd
