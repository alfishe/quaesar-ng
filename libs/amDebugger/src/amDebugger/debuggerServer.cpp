#include "amDebugger/debuggerServer.h"
#include "amDebugger/vm/vmInterface.h"


namespace amD
{


void IDebuggerServer::init()
{
    vm = IVm::VM::setVmInst(IVm::createByFactory_<IVm::VM>());
    vm->init();
}


}; // namespace amD
