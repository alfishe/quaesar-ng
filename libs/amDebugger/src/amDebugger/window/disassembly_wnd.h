#pragma once
#include <EASTL/optional.h>
#include <qd/stl/vector.h>
#include "amDebugger/vm/memory.h"
#include "amDebugger/ui/ui_view.h"
#include "amDebugger/exprValue.h"

FORWARD_DECLARATION_3(amD, cda, Item);

namespace amD {
namespace window {

class DisassemblyView : public amD::AmDbgWindow {
    QDB_WINDOW_REGISTER(WndId::Disassembly, amD::window::DisassemblyView, amD::AmDbgWindow);

    amD::ExprValStr m_addrInputStr;
    eastl::optional<AddrRef> m_viewBaseAddr;
    qd::vector<amD::cda::Item *> m_disasmLines;
    bool m_snapViewPc = true;
    AddrRef m_addrViewStart = 0;
    AddrRef m_addrViewEnd = 0;

public:
    virtual void onCreate(UiViewCreateCtx* cp) override {
        AmDbgWindow::onCreate(cp);
        m_title = "Disassembly";
    }

    virtual void drawContentImp() override;

};  // class DisassemblyView

};  // namespace window
};  // namespace amD
