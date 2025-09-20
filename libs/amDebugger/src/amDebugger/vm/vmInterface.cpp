#include "vmInterface.h"

namespace IVm {


//IVm::VM* VM::staticVmInst = nullptr;


VM::VM() {}


// IVm::VM* VM::setVmInst(IVm::VM* vm_inst)
// {
//     IVm::VM::staticVmInst = vm_inst;
//     return IVm::VM::staticVmInst;
// }
// void VM::destrotVmInst()
// {
//     IVm::VM* oldVm = IVm::VM::staticVmInst;
//     IVm::VM::staticVmInst = nullptr;
//     delete oldVm;
// }


VM::~VM() {}


qd::EFlow VM::applyOperationMsgProcImp(qd::operation::BaseOpArgs* /*args*/)
{
    assert(0);
    return qd::EFlow::NO_RESULT;
}


void VM::applyVmConfig(CfgVmPrefs* /*prefs*/) {}


const amD::MemBank* Memory::findBankByAddr(AddrRef addr) const
{
    for (const amD::MemBank& bank : banks)
    {
        if (addr >= bank.m_startAddr && addr < (bank.m_startAddr + bank.m_size))
            return &bank;
    }
    return nullptr;
}


}; // namespace IVm
