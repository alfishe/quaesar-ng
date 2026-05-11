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


}; // namespace amD
