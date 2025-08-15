#include "amDebugger/debuggerServer.h"
#include "amDebugger/vm/absVM.h"


namespace amD
{


void IDebuggerServer::init()
{
    vm = AbsVM::VM::setVmInst(AbsVM::createByFactory_<AbsVM::VM>());
    vm->init();
}



void* IDebuggerServer::getOpEnvPtr(const qd::TypeInfo& classType) const
{
    return nullptr;
}


qd::EFlow IDebuggerServer::applyOperationMsg(qd::operation::args::Base* args)
{
    return qd::EFlow::DONE;
}


}; // namespace amD
