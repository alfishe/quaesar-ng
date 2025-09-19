#pragma once
#include "qd/stl/optional.h"
#include "qd/stl/vector.h"
#include "amDebugger/vm/memory.h"
#include "amDebugger/ui/uiView.h"
#include "amDebugger/exprValue.h"

FORWARD_DECLARATION_3(amD, cda, Item);

namespace amD {
namespace window {

class DisassemblyView : public amD::AmDbgWindow {
    QDB_WINDOW_REGISTER(WndId::Disassembly, amD::window::DisassemblyView, amD::AmDbgWindow);

    amD::ExprValStr m_addrInputStr;
    qd::optional<AddrRef> m_viewBaseAddr;
    qd::vector<amD::cda::Item *> m_vDisasmLines;
    AddrRef m_mustViewAddr = 0;
    int m_nMustViewAddrDesiredLine = 0;
    bool m_bSnapViewPc = true;
    AddrRef m_addrViewExtraStart = 0;
    AddrRef m_addrViewEnd = 0;
    AddrRef m_prevRegPc = 0;
    int m_nPrevLineCount = 0;
    constexpr static int g_extraScrollLines = 4;


public:
    virtual void onCreate(UiViewCreateCtx* cp) override {
        AmDbgWindow::onCreate(cp);
        m_title = "Disassembly";
    }

    AddrRef getCursorAddr() const;

    virtual void drawContentImp() override;

    virtual qd::EFlow applyOperationMsgProcImp(qd::operation::BaseOpArgs* args) override;

};  // class DisassemblyView

};  // namespace window
};  // namespace amD
