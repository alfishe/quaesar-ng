#pragma once
#include <imgui/imgui.h>
#include <amDebugger/ui/ui_view.h>

namespace amD {
namespace window {


class CustomRegsWnd : public UiWindow {
    QDB_WINDOW_REGISTER(WndId::CustomRegsWnd, amD::window::CustomRegsWnd, amD::UiWindow);
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
};  // namespace amD
