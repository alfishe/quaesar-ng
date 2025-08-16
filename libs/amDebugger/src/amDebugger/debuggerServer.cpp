#include "amDebugger/debuggerServer.h"
#include "amDebugger/vm/vmInterface.h"


namespace amD
{


void IDebuggerServer::init()
{
    vm = IVm::VM::setVmInst(IVm::createByFactory_<IVm::VM>());
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
