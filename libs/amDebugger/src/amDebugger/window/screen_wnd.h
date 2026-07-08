#pragma once
#include <amDebugger/ui/uiView.h>

namespace amD {
namespace window {

// Debugger screen window
class ScreenWnd : public AmDbgWindow
{
    QDB_WINDOW_REGISTER(WndId::Screen, amD::window::ScreenWnd, amD::AmDbgWindow);

    ImTextureID mTextureId = 0;
    ImVec2 mLastWndSize = {0, 0};
    uint32_t m_lastRenderedFrameNo = 0;  // Frame-skip: only grab when emulator produced a new frame

public:
    virtual void onCreate(UiViewCreateCtx* cp) override
    {
        AmDbgWindow::onCreate(cp);
        m_title = "Screen";
    }

    virtual void drawContentImp() override;
    void grabScreenToTexture(Debugger* dbg);
}; // class

}; // namespace window
}; // namespace amD
