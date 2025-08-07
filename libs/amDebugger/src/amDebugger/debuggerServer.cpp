#include "amDebugger/debuggerServer.h"
#include "amDebugger/vm/absVM.h"


namespace amD
{


void DebuggerServer::init()
{
    vm = amD::AbsVM::setVmInst(createByFactory<amD::AbsVM>());
    vm->init();
}



void* DebuggerServer::getOpEnvPtr(const qd::TypeInfo& classType) const
{
    return nullptr;
}


qd::EFlow DebuggerServer::applyOperationMsg(qd::operation::args::Base* args)
{
    return qd::EFlow::DONE;
}


}; // namespace amD
