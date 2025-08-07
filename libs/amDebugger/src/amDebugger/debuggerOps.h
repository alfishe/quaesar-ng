#pragma once
#include <amDebugger/shortcutsList.h>
#include <amDebugger/vm/memory.h>
#include <amDebugger/vm/absVM.h>
#include "qd/base/types.h"
#include "qd/qui/uiOperationArgs.h"


FORWARD_DECLARATION_2(amD, Debugger);
FORWARD_DECLARATION_2(amD, DebuggerDesktop);
FORWARD_DECLARATION_3(amD, operation, Operation);


#define DECLARE_OPERATION_1(TArgClass)                                       \
    TS_REFLECT_CLASS(TArgClass, amD::operation::OperationArgs);                  \
    virtual Base* clone(amD::operation::IOpArgAllocator& allocator) override \
    {                                                                        \
        return amD::operation::clone_op_(*this, allocator);                  \
    }


namespace amD {

namespace shortcut {
enum class EId;
};

namespace operation {


class OpDesc : public qd::operation::args::OpDesc
{
public:
};


class IOpArgAllocator
{
public:
    virtual void* alloc(size_t mem_size) = 0;
    virtual void dealloc(void* pPtr) = 0;
};


template<class TClass>
TClass* clone_op_(const TClass& src, operation::IOpArgAllocator& allocator)
{
    size_t size = sizeof(TClass);
    void* pBuffer = allocator.alloc(size);
    TClass* pInst = new (pBuffer) TClass();
    *pInst = src; // deep copy
    return pInst;
}


class OperationArgs : public qd::operation::args::Base
{
public:
    virtual Base* clone(amD::operation::IOpArgAllocator& allocator) { ASSERT_AND_DO(0, return nullptr, ); }

    static void setup(amD::operation::OpDesc& d) {}
};



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
    TS_BEGIN_REFLECT_CLASS(ClassName, amD::operation::Operation);            \
    TS_ATTRIBUTE(qd::tsAttr::CreateClassCb(&createOperationCb_<TRefClass>)); \
    TS_END();

//////////////////////////////////////////////////////////////////////////


struct AmDebuggerOperationCreator : public qd::UiOperationCreator {
    amD::DebuggerDesktop* gui = nullptr;
    amD::Debugger* dbg = nullptr;
}; // struct AmDebuggerOperationCreator
//////////////////////////////////////////////////////////////////////////


class Operation : public qd::UiOperation
{
    TS_REFLECT_CLASS(amD::operation::Operation, qd::UiOperation);

public:
    // amD::DebuggerDesktop* m_pDbgGui = nullptr;
    // amD::Debugger* m_pDbg = nullptr;

public:
    virtual void onDebuggerOperationCreate(amD::operation::AmDebuggerOperationCreator* cp) {}

    // void doOperationBase(qd::IOperationEnvironment* env);
    void addShortcut(shortcut::EId sid) { UiOperation::addShortcut((int)sid); }
    // amD::Debugger* getDbg() const;

    virtual qd::EFlow applyOperationMsgProc(qd::IOperationEnvironment* env, amD::operation::OperationArgs* p_msg)
    {
        return qd::EFlow::NO_RESULT;
    }
    virtual void onOperationCreate(qd::UiOperationCreator* cp) override;

}; // class Operation
//////////////////////////////////////////////////////////////////////////


namespace args {


struct UaeResetAmiga : public amD::operation::OperationArgs {
    DECLARE_OPERATION_1(amD::operation::args::UaeResetAmiga);
    static void setup(qd::operation::args::OpDesc& d)
    {
        d.m_id = "reset";
        d.m_label = "Reset Amiga";
        d.addShortcut(amD::shortcut::EId::ResetAmigaEmu);
    }
};

struct ExecConsoleCmd : public amD::operation::OperationArgs {
    DECLARE_OPERATION_1(amD::operation::args::ExecConsoleCmd);
    qd::string cmd;
    static void setup(qd::operation::args::OpDesc& d)
    {
        d.m_id = "console";
        d.m_label = "Console command";
    }
};


struct MenuItemStateGet : public amD::operation::OperationArgs {
    //TS_REFLECT_CLASS(MenuItemStateGet, amD::operation::OperationArgs);
    //DECLARE_OPERATION(amD::operation::args::MenuItemStateGet, amD::operation::UaeWndAlwaysOnTop);
    DECLARE_OPERATION_1(amD::operation::args::MenuItemStateGet);
    int checked = -1;
};



struct DoDebugTraceContinue : public amD::operation::OperationArgs {
    //TS_REFLECT_CLASS(DoDebugTraceContinue, amD::operation::OperationArgs);
    //DECLARE_OPERATION(amD::operation::args::DoDebugTraceContinue, amD::operation::DebugTraceContinue);
    DECLARE_OPERATION_1(amD::operation::args::DoDebugTraceContinue);
    amD::DebuggerMode debugMode = DebuggerMode_Live;

};


struct DisasmToggleBreakpoint : public amD::operation::OperationArgs {
    //TS_REFLECT_CLASS(amD::operation::args::DisasmToggleBreakpoint, amD::operation::OperationArgs);
    DECLARE_OPERATION(amD::operation::args::DisasmToggleBreakpoint, amD::operation::DisasmToggleBreakpoint);
    AddrRef address = {};
    EReg reg = EReg::PC;
    int setBreakpoint = -1;
    int nBreakpoint = -1;
};


struct CopperToggleBreakpoint : public amD::operation::OperationArgs {
    //TS_REFLECT_CLASS(CopperToggleBreakpoint, amD::operation::OperationArgs);
    DECLARE_OPERATION(amD::operation::args::CopperToggleBreakpoint, amD::operation::CopperToggleBreakpoint);
    AddrRef address = {};
};


struct CopperTraceStep : public amD::operation::OperationArgs {
    //TS_REFLECT_CLASS(CopperTraceStep, amD::operation::OperationArgs);
    DECLARE_OPERATION(amD::operation::args::CopperTraceStep, amD::operation::CopperTraceStep);
};


};  // namespace args
};  // namespace operation
};  // namespace amD
