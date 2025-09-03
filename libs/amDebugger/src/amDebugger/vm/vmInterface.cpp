#include "vmInterface.h"

namespace IVm {


IVm::VM* VM::staticVmInst = nullptr;


VM::VM() {}


IVm::VM* VM::setVmInst(IVm::VM* vm_inst)
{
    IVm::VM::staticVmInst = vm_inst;
    return IVm::VM::staticVmInst;
}


void VM::destrotVmInst()
{
    IVm::VM* oldVm = IVm::VM::staticVmInst;
    IVm::VM::staticVmInst = nullptr;
    delete oldVm;
}


VM::~VM() {}


}; // namespace IVm
