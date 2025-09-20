#pragma once
#include "qd/qui/uiOperation.h"
#include "amDebugger/vm/memory.h"
#include "amDebugger/vm/vmInterface.h"
#include "dbgConnection.h"


namespace amD {


class IVmConnectionBuilder : public qd::RefCounted, public qd::IOperationEnvironment
{
public:
    IVm::VM* vm = nullptr;

    void init();
    IVm::VM* getVm() const { return vm; }

    virtual ref_ptr<amD::IVmServiceConnection> createConnection() const = 0;

}; // class IVmConnectionBuilder


}; // namespace amD
