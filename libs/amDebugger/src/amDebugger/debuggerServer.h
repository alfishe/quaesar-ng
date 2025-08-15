#pragma once
#include "qd/qui/uiOperation.h"
#include "amDebugger/vm/memory.h"
#include "amDebugger/vm/absVM.h"
#include "dbgConnection.h"


namespace amD {


class IDebuggerServer : public qd::RefCounted, public qd::IOperationEnvironment
{
public:
    AbsVM::VM* vm = nullptr;

    void init();
    AbsVM::VM* getVm() const { return vm; }

    virtual void* getOpEnvPtr(const qd::TypeInfo& classType) const override;
    virtual qd::EFlow applyOperationMsg(qd::operation::args::Base* args) override;

    virtual amD::IDbgConnection* getConnection() const = 0;

}; // class IDebuggerServer


}; // namespace amD
