#pragma once
#include "qd/base/types.h"
#include "qd/typeSystem/attributesCommon.h"
#include "qd/qui/uiOperation.h"
#include "qd/qui/uiOperationArgs.h"


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
    amD::DbgGuiDesktop* m_pDbgGui = nullptr;
    amD::Debugger* m_pDbg = nullptr;

public:
    virtual void onDebuggerOperationCreate(amD::operation::AmDebuggerOperationCreator* cp) {}

    void doOperationBase();
    void addShortcut(shortcut::EId sid) { UiOperation::addShortcut((int)sid); }
    amD::Debugger* getDbg() const;

    virtual qd::EFlow applyOperationMsgProc(qd::operation::args::Base* p_msg) override { return qd::EFlow::NO_RESULT; }

    virtual void onOperationCreate(qd::UiOperationCreator* cp) override;

}; // class Operation
//////////////////////////////////////////////////////////////////////////


}; // namespace operation
}; // namespace amD
