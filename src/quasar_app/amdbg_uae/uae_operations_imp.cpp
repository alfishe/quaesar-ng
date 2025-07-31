// clang-format off
#include <uae_imp/sysconfig.h>
#include "sysdeps.h"
#include "options.h"
#include "keyboard.h"
#include "inputdevice.h"
#include "inputrecord.h"
#include "keybuf.h"
#include "custom.h"
#include "xwin.h"
#include "drawing.h"
#include <uae_lib/include/savestate.h>
#include <uae_lib/include/uae.h>
#include <uae_lib/include/debug.h>
// clang-format on
#include "amDebugger/commonOperations.h"
#include "amDebugger/debuggerApp.h"
#include "amDebugger/msg_list.h"
#include "quasar_app/quaesar.h"
#include "quasar_app/uae_app_part.h"


namespace amD::operation {


int DebugDmaOption::getCurDebugDmaMode() {
    return ::debug_dma;
}


void DebugDmaOption::changeDebugDmaMode(int nMode) {
    eastl::string buf(eastl::string::CtorSprintf(), "v -%d", nMode + 1);
    getDbg()->applyImmediateConsoleCmd(eastl::move(buf));
}


SDL_Window* UaeWndAlwaysOnTop::getEmulatorMainWindow() {
    return g_pApp->m_pUaeAppPart->getSdlWindow();
}


void DisasmTraceStep::doOperation(qd::OperationHistory* history /*= nullptr*/) {
    getDbg()->setDebugMode(DebuggerMode_Break);
    getDbg()->execConsoleCmd("t");
}


void DisasmTraceStepOut::doOperation(qd::OperationHistory* history /*= nullptr*/) {
    getDbg()->execConsoleCmd("z");
}


void DebugTraceStart::doOperation(qd::OperationHistory* history /*= nullptr*/) {
    getDbg()->setDebugMode(DebuggerMode_Break);
}


qd::EFlow DebugTraceContinue::applyOperationMsgProc(qd::operation::msg::Base* msg) {
    if (msg->cast_<qd::operation::msg::DoOperation>() || msg->cast_<amD::operation::msg::DoDebugTraceContinue>()) {
        getDbg()->execConsoleCmd("g");
        return qd::EFlow::SUCCESS;
    } else
        return Operation::applyOperationMsgProc(msg);
}


qd::EFlow DebugWaitScanLines::applyOperationMsgProc(qd::operation::msg::Base* msg) {
    if (msg->cast_<qd::operation::msg::DoOperation>() || msg->cast_<amD::operation::msg::DoDebugTraceContinue>()) {
        eastl::string cmd;
        cmd.append_sprintf("fs %i", getDbg()->getWaitScanLines());
        getDbg()->execConsoleCmd(eastl::move(cmd));
        return qd::EFlow::SUCCESS;
    } else
        return Operation::applyOperationMsgProc(msg);
}


qd::EFlow DisasmToggleBreakpoint::applyOperationMsgProc(qd::operation::msg::Base* msg) {
    if (auto p = msg->cast_<amD::operation::msg::DisasmToggleBreakpoint>()) {
        eastl::string cmd;
        cmd.sprintf("f %08x", (uint32_t)p->address);
        if (p->nBreakpoint >= 0)
            cmd.append_sprintf(" %i", p->nBreakpoint);
        getDbg()->execConsoleCmd(eastl::move(cmd));
        return qd::EFlow::SUCCESS;
    } else
        return Operation::applyOperationMsgProc(msg);
}


qd::EFlow CopperTraceStep::applyOperationMsgProc(qd::operation::msg::Base* msg) {
    if (msg->cast_<qd::operation::msg::DoOperation>() || msg->cast_<amD::operation::msg::CopperTraceStep>()) {
        getDbg()->execConsoleCmd("ot");
        return qd::EFlow::SUCCESS;
    } else
        return Operation::applyOperationMsgProc(msg);
}


qd::EFlow CopperToggleBreakpoint::applyOperationMsgProc(qd::operation::msg::Base* msg) {
    if (auto p = msg->cast_<amD::operation::msg::CopperToggleBreakpoint>()) {
        eastl::string cmd;
        cmd.sprintf("ob %08x", (uint32_t)p->address);
        getDbg()->execConsoleCmd(eastl::move(cmd));
        return qd::EFlow::SUCCESS;
    } else
        return Operation::applyOperationMsgProc(msg);
}


void ToggleTurboEmulation::doOperation(qd::OperationHistory* history /*= nullptr*/) {
    if (currprefs.turbo_emulation != 0) {
        // off
        warpmode(0);
    } else {
        // on
        warpmode(2);
    }
}


void UaeResetAmiga::doOperation(qd::OperationHistory* history /*= nullptr*/) {
    ::uae_reset(1, 1);
}


};  //namespace amD::operation
