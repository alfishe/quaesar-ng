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
    qtd::vector<amD::cda::Item *> m_vDisasmLines;
    AddrRef m_mustViewAddr = 0;
    int m_nMustViewAddrDesiredLine = 0;
    bool m_bSnapViewPc = true;
    AddrRef m_addrViewExtraStart = 0;
    AddrRef m_addrViewEnd = 0;
    AddrRef m_prevRegPc = 0;
    int m_nPrevLineCount = 0;
    int m_nPrevLinesReq = 0;
    constexpr static int g_extraScrollLines = 4;

    // View convergence state: the view adjustment (m_addrViewExtraStart)
    // only runs when m_bViewNeedsAdjust is true, preventing per-frame drift.
    // Set true when PC changes, user scrolls, goto, or window resizes.
    // Cleared when the adjustment converges (no change needed) or after
    // m_nMaxAdjustAttempts frames to prevent infinite loops.
    bool m_bViewNeedsAdjust = true; // start dirty to trigger initial layout
    int m_nAdjustAttempts = 0;
    static constexpr int m_nMaxAdjustAttempts = 5;

    // Disassembly cache: requestM68DisasmLines() is expensive (backward walk
    // + capstone decode). Only re-fetch when inputs actually change.
    // When paused, none of these change, so the cache stays valid and the
    // widget renders identical content without rebuilding the ImGui table.
    AddrRef m_lastDisasmStart = (AddrRef)-1;   // last m_addrViewExtraStart
    int     m_lastDisasmLines = -1;            // last nLinesReq
    AddrRef m_lastDisasmAnchor = (AddrRef)-1;  // last regPc (anchor)
    bool    m_bDisasmValid = false;            // false = needs re-fetch


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
