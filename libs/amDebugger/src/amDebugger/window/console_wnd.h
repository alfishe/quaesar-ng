#pragma once
#include <amDebugger/ui/uiView.h>

namespace amD {
namespace window {

class ConsoleLogWriter;

class ConsoleWnd : public AmDbgWindow {
    QDB_WINDOW_REGISTER(WndId::Console, amD::window::ConsoleWnd, amD::AmDbgWindow);

    eastl::string m_inputStr;
    ConsoleLogWriter* mpConsoleWriter = nullptr;

public:
    virtual void onCreate(UiViewCreateCtx* cp) override;

    virtual void drawContentImp() override;

    virtual void destroy() override;

}; // class ConsoleWnd
//////////////////////////////////////////////////////////////////////////

};  // namespace window
};  // namespace amD
