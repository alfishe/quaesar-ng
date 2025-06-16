#pragma once
#include "qd/base/types.h"
#include "qd/typeSystem/attributesCommon.h"
#include "qd/ui/uiOperation.h"
#include "qd/ui/uiOperationMessages.h"


namespace amD {
class Debugger;
class DbgGuiDesktop;

namespace shortcut {
enum class EId;
};

namespace operation {
class Operation;


//////////////////////////////////////////////////////////////////////////
template<class TClass>
static amD::operation::Operation* createOperationCb_(const qd::TypeInfo& meta, qd::UiOperationCreator* cp)
{
    TClass* pNewInst = new TClass();
    pNewInst->onOperationCreate(cp);
    pNewInst->setup();
    return pNewInst;
}


#define QDB_REG_OPERATION(ClassName)                                         \
    TS_BEGIN_REFLECT_CLASS(ClassName, amD::operation::Operation);             \
    TS_ATTRIBUTE(qd::tsAttr::CreateClassCb(&createOperationCb_<TRefClass>)); \
    TS_END();

//////////////////////////////////////////////////////////////////////////


struct AmDebuggerOperationCreator : public qd::UiOperationCreator {
    amD::DbgGuiDesktop* gui = nullptr;
    amD::Debugger* dbg = nullptr;
}; // struct AmDebuggerOperationCreator
//////////////////////////////////////////////////////////////////////////


class Operation : public qd::UiOperation
{
    TS_REFLECT_CLASS(amD::operation::Operation, qd::UiOperation);

public:
    amD::DbgGuiDesktop* gui = nullptr;
    amD::Debugger* dbg = nullptr;

public:
    virtual void onDebuggerOperationCreate(amD::operation::AmDebuggerOperationCreator* cp) {}

    void doOperationBase();
    void addShortcut(shortcut::EId sid) { UiOperation::addShortcut((int)sid); }
    amD::Debugger* getDbg() const;

    virtual qd::EFlow applyOperationMsgProc(qd::operation::msg::Base* p_msg) override;

private:
    virtual void onNodeCreated(qd::NodeCreator* cp) override;

}; // class Operation
//////////////////////////////////////////////////////////////////////////


}; // namespace operation
}; // namespace amD
