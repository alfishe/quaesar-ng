#pragma once
#include "debuggerOps.h"
#include "qd/stl/ref_ptr.h"


namespace amD {
class UaeSharedConnectionImpl;


class IVmServiceProvider : public qd::RefCounted
{
public:
    qd::string m_name;

public:
    virtual ref_ptr<IVm::VM> getClientVm() = 0;
    virtual ref_ptr<IVm::VM> getServerVm() = 0;

}; // IVmServiceProvider
//////////////////////////////////////////////////////////////////////////


ref_ptr<IVmServiceProvider> create_dummy_connection();
ref_ptr<IVmServiceProvider> create_uae_shared_connection(const char* name);


}; // namespace amD
