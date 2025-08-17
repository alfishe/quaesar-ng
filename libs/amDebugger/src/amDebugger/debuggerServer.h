#pragma once
#include "qd/qui/uiOperation.h"
#include "amDebugger/vm/memory.h"
#include "amDebugger/vm/vmInterface.h"
#include "dbgConnection.h"


namespace amD {


class IDebuggerServer : public qd::RefCounted, public qd::IOperationEnvironment
{
public:
    IVm::VM* vm = nullptr;

    void init();
    IVm::VM* getVm() const { return vm; }

    virtual void* getOpEnvPtr(const qd::TypeInfo& classType) const override;
    virtual qd::EFlow applyOperationMsgProc(qd::operation::args::Base* args) override;

    virtual ref_ptr<amD::IDbgConnection> createConnection() const = 0;

}; // class IDebuggerServer


}; // namespace amD
