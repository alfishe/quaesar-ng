#pragma once
#include "qd/base/base.h"
#include "qd/base/classInfoReg.h"
#include "qd/base/eFlow.h"
#include "qd/base/baseTypes.h"
#include "qd/debug/assert.h"
#include "qd/qui/operationsRegistry.h"
#include "qd/stl/fixed_vector.h"
#include "qd/typeSystem/typeDeclare.h"
#include "qd/stl/ref_ptr.h"
#include "qd/typeSystem/typeInfo.h"


#define QD_REG_OPERATION(ClassName)                                              \
    TS_BEGIN_REFLECT_CLASS(ClassName, qd::operation::Operation);                 \
    TS_ATTRIBUTE(qd::ts::attr::CreateClassCb(&createUiOperationCb_<TRefClass>)); \
    TS_END();


#define DECLARE_OPERATION(TArgClass, TOpClass)              \
    TS_REFLECT_CLASS(TArgClass, qd::operation::BaseOpArgs); \
    inline static const qd::TypeInfo& s_OperationType = qd::typeof_by_name(#TOpClass);



template<class TClass>
struct AutoRegOpDesc_ {
    AutoRegOpDesc_() { qd::OperationsRegistry::get().regOperationDesc_<TClass>(); }
};


#define DECLARE_OPERATION_1(TArgClass)                                                                 \
    TS_REFLECT_CLASS(TArgClass, qd::operation::BaseOpArgs);                                            \
    virtual qd::operation::BaseOpArgs* clone(qd::operation::IOpArgAllocator& allocator) override       \
    {                                                                                                  \
        return qd::operation::clone_op_<TArgClass>(*this, allocator);                                  \
    }                                                                                                  \
    inline static AutoRegOpDesc_<TArgClass> s_AutoRegOpDesc;\



namespace qd {
class DebuggerDesktop;
class UiOperation;
class OperationsRegistry;
class IOperationEnvironment;
FORWARD_DECLARATION_3S(operation, args, Base);


// Operation's component classId
enum class EOperationCompsClassId {
    Shortcuts,
    MOST_COMMON_COMPS,
};

struct UiOperationCreator {
    OperationsRegistry* uiOpsMgr = nullptr;
    uint32_t id = 0;

    template<class TClass, typename... TArgs>
    TClass* make_(TArgs&&... args)
    {
        TClass* pNode = new TClass(args...);
        pNode->onNodeCreated(this);
        return pNode;
    } // make_
}; // struct UiOperationCreator
//////////////////////////////////////////////////////////////////////////


class OperationHistory
{
};


// An interface that can perform operations based on their arguments, or send them to its parent if it cannot perform
// them itself.
class IOperationEnvironment
{
    TS_REFLECT_CLASS(qd::IOperationEnvironment, void);

protected:
    virtual qd::EFlow applyOperationMsgProcImp(qd::operation::BaseOpArgs* /*args*/) { return EFlow::NO_RESULT; }
    virtual qd::EFlow setupDefaultOperationArgsImp(qd::operation::BaseOpArgs* /*args*/) const { return EFlow::NO_RESULT; }

public:
    virtual IOperationEnvironment* getOpEnvParent() const { return nullptr; }
    qd::EFlow applyOperationMsgProc(qd::operation::BaseOpArgs* args);
    qd::EFlow setupDefaultOperationArgs(qd::operation::BaseOpArgs* args) const;

    template<class TOpClass>
    void doOperationDefault_()
    {
        TOpClass opArgs;
        setupDefaultOperationArgs(&opArgs);
        applyOperationMsgProcImp(&opArgs);
    }

}; // IOperationEnvironment
//////////////////////////////////////////////////////////////////////////


template<class TClass>
static qd::UiOperation* createUiOperationCb_(const qd::TypeInfo& /*meta*/, qd::UiOperationCreator* cp)
{
    TClass* pNewInst = new TClass();
    pNewInst->onOperationCreate(cp);
    pNewInst->setup();
    return pNewInst;
}


//////////////////////////////////////////////////////////////////////////
namespace operation {
class IOpArgAllocator
{
public:
    virtual void* alloc(size_t mem_size) = 0;
    virtual void dealloc(void* pPtr) = 0;
};


template<class TClass>
TClass* clone_op_(const TClass& src, qd::operation::IOpArgAllocator& allocator)
{
    size_t size = sizeof(TClass);
    void* pBuffer = allocator.alloc(size);
    TClass* pInst = new (pBuffer) TClass();
    *pInst = src; // deep copy
    return pInst;
}


struct BaseOpArgs {
    TS_REFLECT_CLASS(qd::operation::BaseOpArgs, void);
public:
    inline BaseOpArgs() = default;
    virtual ~BaseOpArgs() = default;
    bool _tryCast(const qd::TypeInfo& msg_type);

    template<class T>
    T* cast_()
    {
        if (!c_def(this))
            return nullptr;
        const qd::TypeInfo& type = T::getStaticTypeInfo();
        if (!_tryCast(type))
            return nullptr;
        return static_cast<T*>(this);
    }

    virtual BaseOpArgs* clone(qd::operation::IOpArgAllocator& /*allocator*/) { ASSERT_AND_DO(0, return nullptr, ); }

    //SHOULD BE DECLARED
    //static void setup(qd::operation::OpDesc& d) {}

}; // struct BaseOpArgs
//////////////////////////////////////////////////////////////////////////


inline bool BaseOpArgs::_tryCast(const qd::TypeInfo& msg_type)
{
    const qd::TypeInfo& typeInfo = getTypeInfo();
    return typeInfo.isDerivedFrom(msg_type);
}


}; // namespace operation
}; // namespace qd
