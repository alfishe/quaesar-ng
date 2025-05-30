#pragma once
#include <imgui/imgui.h>
#include <amDebugger/ui/ui_view.h>

namespace qd {
namespace window {


class CustomRegsWnd : public UiWindow {
    QDB_WINDOW_REGISTER(WndId::CustomRegsWnd, qd::window::CustomRegsWnd, qd::UiWindow);
    ImGuiTextFilter mRegsFilter;

public:
    virtual void onCreate(UiViewCreateCtx* cp) override {
        UiWindow::onCreate(cp);
        mTitle = "Custom regs";
    }

    virtual void drawContent() override;

}; // CustomRegsWnd;
//////////////////////////////////////////////////////////////////////////


};  // namespace window
};  // namespace qd
