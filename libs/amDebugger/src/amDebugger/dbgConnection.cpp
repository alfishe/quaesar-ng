#include "dbgConnection.h"
#include "amDebugger/vm/vmInterface.h"


namespace amD
{


class DummyVmDbgServiceBridge : public IVmDbgServiceBridge
{
    ref_ptr<IVm::VM> m_vm;

public:
    DummyVmDbgServiceBridge()
    {
        m_vm = IVm::createByFactory_<IVm::VM>();
        if (m_vm)
            m_vm->init();
    }

    ref_ptr<IVm::VM> getClientVm() override { return m_vm; }
    ref_ptr<IVm::VM> getServerVm() override { return m_vm; }
};


ref_ptr<IVmDbgServiceBridge> create_dummy_connection()
{
    return new DummyVmDbgServiceBridge();
}


// -----------------------------------------------------------------------
// SharedVmDbgServiceBridge: wraps a pre-existing VM instance.
// Unlike DummyVmDbgServiceBridge (which creates a fresh UaeVmImp with
// m_pUaeThread=nullptr), this bridge wraps the real UaeVmImp from
// UaeServerThread, where m_pUaeThread is properly set so that
// setVmDebugMode() can actually activate/deactivate UAE's debugger.
//
class SharedVmDbgServiceBridge : public IVmDbgServiceBridge
{
    ref_ptr<IVm::VM> m_vm;

public:
    SharedVmDbgServiceBridge(ref_ptr<IVm::VM> vm) : m_vm(vm) {}

    ref_ptr<IVm::VM> getClientVm() override { return m_vm; }
    ref_ptr<IVm::VM> getServerVm() override { return m_vm; }
};


ref_ptr<IVmDbgServiceBridge> create_shared_connection(ref_ptr<IVm::VM> vm)
{
    return new SharedVmDbgServiceBridge(vm);
}


}; // namespace amD
