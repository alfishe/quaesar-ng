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
        m_title = "Custom regs";
    }

    virtual void drawContentImp() override;

}; // CustomRegsWnd;
//////////////////////////////////////////////////////////////////////////


};  // namespace window
};  // namespace qd
