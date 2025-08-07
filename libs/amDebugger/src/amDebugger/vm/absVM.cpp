#include "absVM.h"

namespace amD {
AbsVM* AbsVM::staticVmInst = nullptr;


AbsVM::AbsVM() {
}


amD::AbsVM* AbsVM::setVmInst(AbsVM* vm_inst)
{
    AbsVM::staticVmInst = vm_inst;
    return AbsVM::staticVmInst;
}


void AbsVM::destrotVmInst()
{
    AbsVM* oldVm = AbsVM::staticVmInst;
    AbsVM::staticVmInst = nullptr;
    delete oldVm;
}


AbsVM::~AbsVM() {
}


};  // namespace amD
