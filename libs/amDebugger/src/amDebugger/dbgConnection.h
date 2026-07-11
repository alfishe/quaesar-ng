#pragma once
#include "debuggerOps.h"
#include "qd/stl/ref_ptr.h"


namespace amD {
class UaeSharedConnectionImpl;


class IVmDbgServiceBridge : public qd::RefCounted
{
public:
    qtd::string m_name;

public:
    virtual ref_ptr<IVm::VM> getClientVm() = 0;
    virtual ref_ptr<IVm::VM> getServerVm() = 0;

}; // IVmDbgServiceBridge
//////////////////////////////////////////////////////////////////////////


ref_ptr<IVmDbgServiceBridge> create_dummy_connection();
ref_ptr<IVmDbgServiceBridge> create_uae_shared_connection(const char* name);

// Create a bridge that wraps a specific VM instance.
// Used to connect the debugger to the real UAE VM (which has m_pUaeThread
// properly set), replacing the dummy bridge created during early init.
ref_ptr<IVmDbgServiceBridge> create_shared_connection(ref_ptr<IVm::VM> vm);


}; // namespace amD
