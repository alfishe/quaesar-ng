#pragma once
#include "qd/base/base.h"
#include "qd/base/classInfoReg.h"
#include "qd/base/eFlow.h"
#include "qd/base/baseTypes.h"
#include "qd/debug/assert.h"
#include "qd/qui/uiOperationMgr.h"
#include "qd/stl/fixed_vector.h"
#include "qd/typeSystem/typeDeclare.h"
#include "qd/stl/ref_ptr.h"
#include "qd/typeSystem/typeInfo.h"


#define QD_REG_OPERATION(ClassName)                                              \
    TS_BEGIN_REFLECT_CLASS(ClassName, qd::operation::Operation);                 \
    TS_ATTRIBUTE(qd::ts::attr::CreateClassCb(&createUiOperationCb_<TRefClass>)); \
    TS_END();


#define DECLARE_OPERATION(TArgClass, TOpClass)              \
    TS_REFLECT_CLASS(TArgClass, qd::operation::args::Base); \
    inline static const qd::TypeInfo& s_OperationType = qd::typeof_by_name(#TOpClass);



template<class TClass>
struct AutoRegOpDesc_ {
    AutoRegOpDesc_() { qd::UiOperationMgr::get().regOperationDesc_<TClass>(); }
};


#define DECLARE_OPERATION_1(TArgClass)                                                                 \
    TS_REFLECT_CLASS(TArgClass, qd::operation::args::Base);                                            \
    virtual qd::operation::args::Base* clone(qd::operation::args::IOpArgAllocator& allocator) override \
    {                                                                                                  \
        return qd::operation::args::clone_op_<TArgClass>(*this, allocator);                            \
    }                                                                                                  \
    inline static AutoRegOpDesc_<TArgClass> s_AutoRegOpDesc;\



namespace qd {
class DebuggerDesktop;
class UiOperation;
class UiOperationMgr;
class IOperationEnvironment;
FORWARD_DECLARATION_3S(operation, args, Base);


// Operation's component classId
enum class EOperationCompsClassId {
    Shortcuts,
    MOST_COMMON_COMPS,
};

struct UiOperationCreator {
    UiOperationMgr* uiOpsMgr = nullptr;
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
public:
};


class IOperationEnvironment
{
    TS_REFLECT_CLASS(qd::IOperationEnvironment, void);

public:
    virtual IOperationEnvironment* getOpEnvParent() const { return nullptr; }
    virtual void* getOpEnvPtr(const qd::TypeInfo& classType) const { return nullptr; }
    virtual qd::EFlow applyOperationMsgProc(qd::operation::args::Base* args);

    template<class TPtr>
    TPtr* getPtr_() const
    {
        const qd::TypeInfo& ptrType = qd::typeof_<TPtr>();
        void* pPtr = getOpEnvPtr(ptrType);
        return reinterpret_cast<TPtr*>(pPtr);
    }

    template<class TOpClass>
    void doOperation_()
    {
        TOpClass opArgs;
        applyOperationMsgProc(&opArgs);
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



namespace operation::args {
class IOpArgAllocator
{
public:
    virtual void* alloc(size_t mem_size) = 0;
    virtual void dealloc(void* pPtr) = 0;
};


template<class TClass>
TClass* clone_op_(const TClass& src, qd::operation::args::IOpArgAllocator& allocator)
{
    size_t size = sizeof(TClass);
    void* pBuffer = allocator.alloc(size);
    TClass* pInst = new (pBuffer) TClass();
    *pInst = src; // deep copy
    return pInst;
}


struct Base {
    TS_REFLECT_CLASS(qd::operation::args::Base, void);
public:
    inline Base() = default;

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

    virtual Base* clone(qd::operation::args::IOpArgAllocator& allocator) { ASSERT_AND_DO(0, return nullptr, ); }

    // setup() should be declared
    //static void setup(qd::operation::args::OpDesc& d) {}

}; // struct args::Base
//////////////////////////////////////////////////////////////////////////


inline bool Base::_tryCast(const qd::TypeInfo& msg_type)
{
    const qd::TypeInfo& typeInfo = getTypeInfo();
    return typeInfo.isDerivedFrom(msg_type);
}


}; // namespace operation::args
}; // namespace qd
