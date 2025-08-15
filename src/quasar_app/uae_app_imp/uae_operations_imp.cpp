// clang-format off
#include <sysconfig.h>
#include <sysdeps.h>
#include <options.h>
#include <keyboard.h>
#include <inputdevice.h>
#include <inputrecord.h>
#include <keybuf.h>
#include <custom.h>
#include <xwin.h>
#include <drawing.h>
#include <uae_lib/include/savestate.h>
#include <uae_lib/include/uae.h>
#include <uae_lib/include/debug.h>
// clang-format on
#include "amDebugger/commonOperations.h"
#include "amDebugger/debuggerApp.h"
#include "amDebugger/debuggerOps.h"
#include "quasar_app/quaesar.h"
#include "quasar_app/uae_app_imp/uae_client_app_part.h"


namespace amD::operation {


int DebugDmaOption::getCurDebugDmaMode(qd::IOperationEnvironment* env) {
    return ::debug_dma;
}


void DebugDmaOption::changeDebugDmaMode(qd::IOperationEnvironment* env, int nMode) {
    eastl::string buf(eastl::string::CtorSprintf(), "v -%d", nMode + 1);
    amD::Debugger* pDbg = env->getPtr_<amD::Debugger>();
    pDbg->execConsoleCmd(eastl::move(buf));
}


SDL_Window* UaeWndAlwaysOnTop::getEmulatorMainWindow(qd::IOperationEnvironment* env) {
    return g_pApp->m_pUaeClientAppPart->getSdlWindow();
}


void DisasmTraceStep::doOperation(qd::IOperationEnvironment* env) {
    auto pDbg = env->getPtr_<amD::Debugger>();
    pDbg->setDebugMode(DebuggerMode_Break);
    pDbg->execConsoleCmd("t");
}


void DisasmTraceStepOut::doOperation(qd::IOperationEnvironment* env) {
    auto pDbg = env->getPtr_<amD::Debugger>();
    pDbg->execConsoleCmd("z");
}


void DebugTraceStart::doOperation(qd::IOperationEnvironment* env) {
    auto pDbg = env->getPtr_<amD::Debugger>();
    pDbg->setDebugMode(DebuggerMode_Break);
}


void uae_op_debug_trace_continue(qd::IOperationEnvironment* env, amD::operation::args::DoDebugTraceContinue* p) {
    auto pDbg = env->getPtr_<amD::Debugger>();
    pDbg->execConsoleCmd("g");
}


qd::EFlow DebugWaitScanLines::applyOperationMsgProc(qd::IOperationEnvironment* env,
                                                    amD::operation::OperationArgs* p_msg) {
    auto pDbg = env->getPtr_<amD::Debugger>();
    if (p_msg->cast_<qd::operation::args::DoOperation>() ||
        p_msg->cast_<amD::operation::args::DoDebugTraceContinue>()) {
        eastl::string cmd;
        cmd.append_sprintf("fs %i", pDbg->getWaitScanLines());
        pDbg->execConsoleCmd(eastl::move(cmd));
        return qd::EFlow::SUCCESS;
    } else
        return Operation::applyOperationMsgProc(env, p_msg);
}


qd::EFlow DisasmToggleBreakpoint::applyOperationMsgProc(qd::IOperationEnvironment* env,
                                                        amD::operation::OperationArgs* p_msg) {
    auto pDbg = env->getPtr_<amD::Debugger>();
    if (auto p = p_msg->cast_<amD::operation::args::DisasmToggleBreakpoint>()) {
        eastl::string cmd;
        cmd.sprintf("f %08x", (uint32_t)p->address);
        if (p->nBreakpoint >= 0)
            cmd.append_sprintf(" %i", p->nBreakpoint);
        pDbg->execConsoleCmd(eastl::move(cmd));
        return qd::EFlow::SUCCESS;
    } else
        return Operation::applyOperationMsgProc(env, p_msg);
}


qd::EFlow CopperTraceStep::applyOperationMsgProc(qd::IOperationEnvironment* env, amD::operation::OperationArgs* p_msg) {
    auto pDbg = env->getPtr_<amD::Debugger>();
    if (p_msg->cast_<qd::operation::args::DoOperation>() || p_msg->cast_<amD::operation::args::CopperTraceStep>()) {
        pDbg->execConsoleCmd("ot");
        return qd::EFlow::SUCCESS;
    } else
        return Operation::applyOperationMsgProc(env, p_msg);
}


qd::EFlow CopperToggleBreakpoint::applyOperationMsgProc(qd::IOperationEnvironment* env,
                                                        amD::operation::OperationArgs* p_msg) {
    auto pDbg = env->getPtr_<amD::Debugger>();
    if (auto p = p_msg->cast_<amD::operation::args::CopperToggleBreakpoint>()) {
        eastl::string cmd;
        cmd.sprintf("ob %08x", (uint32_t)p->address);
        pDbg->execConsoleCmd(eastl::move(cmd));
        return qd::EFlow::SUCCESS;
    } else
        return Operation::applyOperationMsgProc(env, p_msg);
}


void ToggleTurboEmulation::doOperation(qd::IOperationEnvironment* env /*= nullptr*/) {
    if (currprefs.turbo_emulation != 0) {
        // off
        warpmode(0);
    } else {
        // on
        warpmode(2);
    }
}


void UaeResetAmiga::doOperation(qd::IOperationEnvironment* env /*= nullptr*/) {
    ::uae_reset(1, 1);
}


};  //namespace amD::operation
