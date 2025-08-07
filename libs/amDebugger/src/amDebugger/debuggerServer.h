#pragma once
#include "qd/qui/uiOperation.h"
#include "amDebugger/vm/memory.h"
#include "amDebugger/vm/absVM.h"



namespace amD {

class DebuggerServer : public qd::RefCounted, public qd::IOperationEnvironment
{
public:
    amD::AbsVM* vm = nullptr;

    void init();
    amD::AbsVM* getVm() const { return vm; }

    virtual void* getOpEnvPtr(const qd::TypeInfo& classType) const override;
    virtual qd::EFlow applyOperationMsg(qd::operation::args::Base* args) override;

}; // class DebuggerServer


}; // namespace amD
