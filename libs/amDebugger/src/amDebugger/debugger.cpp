#include "amDebugger/debugger.h"
#include "amDebugger/vm/absVM.h"
#include "amDebugger/debuggerOps.h"


namespace amD
{


void Debugger::init()
{
    vm = amD::AbsVM::setVmInst(amD::createByFactory<amD::AbsVM>());
    vm->init();
}



void* Debugger::getOpEnvPtr(const qd::TypeInfo& classType) const
{
    return nullptr;
}


qd::EFlow Debugger::applyOperationMsg(qd::operation::args::Base* args)
{
    return qd::EFlow::DONE;
}



void Debugger::execConsoleCmd(qd::string&& cmd)
{
    amD::operation::args::ExecConsoleCmd exec;
    exec.cmd = std::move(cmd);
    applyOperationMsg(&exec);
}


void Debugger::setDebugMode(DebuggerMode debug_mode)
{
    amD::operation::args::DoDebugTraceContinue dd;
    dd.debugMode = debug_mode;
    sendOperationBoth(&dd);
}


}; // namespace amD
