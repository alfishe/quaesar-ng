#include "amDebugger/debuggerServer.h"
#include "amDebugger/vm/vmInterface.h"


namespace amD
{


void IVmConnectionBuilder::init()
{
    vm = IVm::createByFactory_<IVm::VM>();
    vm->init();
}


}; // namespace amD
