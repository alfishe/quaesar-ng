#include "absVM.h"

namespace AbsVM
{
	AbsVM::VM* VM::staticVmInst = nullptr;


	VM::VM() {
	}


	AbsVM::VM* VM::setVmInst(AbsVM::VM* vm_inst)
	{
	    AbsVM::VM::staticVmInst = vm_inst;
	    return AbsVM::VM::staticVmInst;
	}


	void VM::destrotVmInst()
	{
	    AbsVM::VM* oldVm = AbsVM::VM::staticVmInst;
	    AbsVM::VM::staticVmInst = nullptr;
	    delete oldVm;
	}


	VM::~VM() {
	}


}; // namespace AbsVM
