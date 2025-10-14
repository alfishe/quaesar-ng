#include "vmInterface.h"
#include "EASTL/span.h"

namespace IVm {


// IVm::VM* VM::staticVmInst = nullptr;


VM::VM()
{}


VM::~VM() {}


void VM::init()
{
    for (IModule* curModule : eastl::span<IModule *>(&m_modSectBeg, &m_modSectEnd))
    {
        IModule* pCurMod = curModule;
        if (pCurMod)
            pCurMod->init(this);
    }
    mInit = true;
}


void VM::fetchStateFromEmu()
{
    for (IModule* curModule : eastl::span<IModule *>(&m_modSectBeg, &m_modSectEnd))
    {
        IModule* pCurMod = curModule;
        if (pCurMod)
            pCurMod->fetch();
    }
}


qd::EFlow VM::applyOperationMsgProcImp(qd::operation::BaseOpArgs* /*args*/)
{
    assert(0);
    return qd::EFlow::NO_RESULT;
}


void VM::applyVmConfig(CfgVmPrefs* /*prefs*/) {}


const IVm::MemBank* Memory::findBankByAddr(AddrRef addr) const
{
    for (const IVm::MemBank& bank : m_banks)
    {
        if (addr >= bank.m_startAddr && addr < (bank.m_startAddr + bank.m_size))
            return &bank;
    }
    return nullptr;
}


}; // namespace IVm
