#pragma once
#include <amDebugger/shortcut/shortcut_list.h>
#include <amDebugger/ui_defs.h>
#include <EASTL/bitset.h>
#include "qd/STL/vector_map.h"
#include "qd/Base/classInfoReg.h"
#include "qd/Base/types.h"
#include "qd/TypeSystem/ReflectedType.h"
#include "qd/UI/uiOperation.h"
#include "qd/UI/uiOperationMessages.h"


namespace qd {
class Debugger;
class GuiManager;

namespace operation {
class Operation;


//////////////////////////////////////////////////////////////////////////
template<class TClass>
static qd::operation::Operation* createOperationCb_(const qd::TypeInfo& meta, qd::UiOperationCreator* cp)
{
    TClass* pNewInst = new TClass();
    pNewInst->onOperationCreate(cp);
    pNewInst->setup();
    return pNewInst;
}


#define QDB_REG_OPERATION(ClassName)                                     \
    TS_BEGIN_REFLECT_CLASS(ClassName, qd::operation::Operation);            \
    TS_ATTRIBUTE(qd::CreateClassCbAttr(&createOperationCb_<ClassName>)); \
    TS_END();                                                         \
                                                                      \
public:

//////////////////////////////////////////////////////////////////////////


struct AmDebuggerOperationCreator : public qd::UiOperationCreator {
    GuiManager* gui = nullptr;
    Debugger* dbg = nullptr;
}; // struct AmDebuggerOperationCreator
//////////////////////////////////////////////////////////////////////////


class Operation : public qd::UiOperation
{
    TS_REFLECT_CLASS(qd::operation::Operation, qd::UiOperation);

public:
    GuiManager* gui = nullptr;
    Debugger* dbg = nullptr;

public:
    virtual void onDebuggerOperationCreate(qd::operation::AmDebuggerOperationCreator* cp) {}

    void doOperationBase();
    void addShortcut(shortcut::EId sid) { UiOperation::addShortcut((int)sid); }
    Debugger* getDbg() const;

    virtual EFlow applyOperationMsgProc(operation::msg::Base* p_msg) override;

private:
    virtual void onNodeCreated(NodeCreator* cp) override;

}; // class Operation
//////////////////////////////////////////////////////////////////////////


}; // namespace operation
}; // namespace qd
