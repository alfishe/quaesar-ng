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


}; // namespace amD
