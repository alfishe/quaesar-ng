#pragma once
#include <amDebugger/ui/ui_view.h>

namespace qd {
namespace window {

class ConsoleLogWriter;

class ConsoleWnd : public UiWindow {
    QDB_WINDOW_REGISTER(WndId::Console, qd::window::ConsoleWnd, qd::UiWindow);

    eastl::string inputStr;
    ConsoleLogWriter* mpConsoleWriter = nullptr;

public:
    virtual void onCreate(UiViewCreateCtx* cp) override;

    virtual void drawContentImp() override;

    virtual void destroy() override;

}; // class ConsoleWnd
//////////////////////////////////////////////////////////////////////////

};  // namespace window
};  // namespace qd
