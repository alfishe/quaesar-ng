#pragma once
#include <imgui/imgui.h>
#include <amDebugger/ui/uiView.h>

namespace amD {
namespace window {


class CustomRegsWnd : public AmDbgWindow {
    QDB_WINDOW_REGISTER(WndId::CustomRegsWnd, amD::window::CustomRegsWnd, amD::AmDbgWindow);
    ImGuiTextFilter mRegsFilter;

public:
    virtual void onCreate(UiViewCreateCtx* cp) override {
        AmDbgWindow::onCreate(cp);
        m_title = "Custom regs";
    }

    virtual void drawContentImp() override;

}; // CustomRegsWnd;
//////////////////////////////////////////////////////////////////////////


};  // namespace window
};  // namespace amD
