#pragma once
#include <EASTL/optional.h>
#include <amDebugger/vm/memory.h>
#include <amDebugger/ui/ui_view.h>

namespace amD {
namespace window {

class DisassemblyView : public amD::AmDbgWindow {
    QDB_WINDOW_REGISTER(WndId::Disassembly, amD::window::DisassemblyView, amD::AmDbgWindow);

    eastl::string addrInputStr;
    eastl::optional<AddrRef> mDisasmAddr;

public:
    virtual void onCreate(UiViewCreateCtx* cp) override {
        AmDbgWindow::onCreate(cp);
        m_title = "Disassembly";
    }

    virtual void drawContentImp() override;

};  // class DisassemblyView

};  // namespace window
};  // namespace amD
