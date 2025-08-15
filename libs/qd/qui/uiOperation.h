#pragma once
#include "qd/base/base.h"
#include "qd/base/classInfoReg.h"
#include "qd/base/eFlow.h"
#include "qd/base/baseTypes.h"
#include "qd/debug/assert.h"
#include "qd/qui/shortcutHnd.h"
#include "qd/qui/uiOperationArgs.h"
#include "qd/qui/uiOperationMgr.h"
#include "qd/stl/fixed_vector.h"
#include "qd/stl/string.h"
#include "qd/typeSystem/typeDeclare.h"


#define QD_REG_OPERATION(ClassName)                                              \
    TS_BEGIN_REFLECT_CLASS(ClassName, qd::operation::Operation);                 \
    TS_ATTRIBUTE(qd::ts::attr::CreateClassCb(&createUiOperationCb_<TRefClass>)); \
    TS_END();


#define DECLARE_OPERATION(TArgClass, TOpClass)              \
    TS_REFLECT_CLASS(TArgClass, qd::operation::args::Base); \
    inline static const qd::TypeInfo& s_OperationType = qd::typeof_by_name(#TOpClass);


#define DECLARE_OPERATION_1(TArgClass)                                                                 \
    TS_REFLECT_CLASS(TArgClass, qd::operation::args::Base);                                            \
    virtual qd::operation::args::Base* clone(qd::operation::args::IOpArgAllocator& allocator) override \
    {                                                                                                  \
        return qd::operation::args::clone_op_<TArgClass>(*this, allocator);                            \
    }


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
    virtual qd::EFlow applyOperationMsg(qd::operation::args::Base* args)
    {
        IOperationEnvironment* pEnv = getOpEnvParent();
        while (pEnv)
        {
            qd::EFlow f = pEnv->applyOperationMsg(args);
            if (f.isDone())
                return f;
            pEnv = pEnv->getOpEnvParent();
        }
        return qd::EFlow::NO_RESULT;
    }

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
        applyOperationMsg(&opArgs);
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
class UiOperation : public qd::RefCounted
{
    TS_REFLECT_CLASS_BASE(200, qd::UiOperation, qd::RefCounted);

public:
    uint32_t mClassId = -1;
    qd::string m_name;
    qd::string m_description;
    bool m_bActive = true;
    ref_ptr<ShortcutHnd> m_pShortcuts;

public:
    UiOperation() = default;
    virtual ~UiOperation() = default;

    virtual void onOperationCreate(qd::UiOperationCreator* cp) { /*onNodeCreated(cp);*/ }
    virtual void destroy() {}

//     virtual qd::EFlow applyOperationMsgProc(qd::IOperationEnvironment* env, qd::operation::args::Base* p_msg)
//     {
//         return EFlow::NO_RESULT;
//     }

    // void doOperationBase();
    virtual void doOperation(qd::IOperationEnvironment* env)
    {
        assert(0);
        //         operation::args::DoOperation msg;
        //         applyOperationMsgProc(env, &msg);
    }
    //     virtual void undoOperation(IOperationEnvironment& history) {}

    virtual bool hasMtd(int id) const { return false; /*supportMtd[id];*/ }
    void addShortcut(int sid);

    bool isActive() const { return m_bActive; }
    void setActive(bool Active) { m_bActive = Active; }

    const qd::string& getName() const { return m_name; }

    qd::ShortcutHnd* getShortcuts() const { return m_pShortcuts; }


}; // class UiOperation
//////////////////////////////////////////////////////////////////////////




namespace operation::args {


class OpDesc
{
private:
    ref_ptr<ShortcutHnd> m_pShortcuts;

public:
    qd::string m_id;
    qd::string m_label;
    void addShortcut(int sid);
    template<typename TEnum>
    void addShortcut(TEnum sid)
    {
        addShortcut((int)sid);
    }
}; // class OpDesc


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

    template<class T>
    T* cast_()
    {
        if (!c_def(this))
            return nullptr;
        const qd::TypeInfo& type = T::getStaticTypeInfo();
        if (!tryCast(type))
            return nullptr;
        return static_cast<T*>(this);
    }

    virtual bool tryCast(const qd::TypeInfo& msg_type);
    virtual Base* clone(qd::operation::args::IOpArgAllocator& allocator) { ASSERT_AND_DO(0, return nullptr, ); }
    static void setup(qd::operation::args::OpDesc& d) {}

}; // struct args::Base
//////////////////////////////////////////////////////////////////////////



struct DoOperation : public operation::args::Base {
    TS_REFLECT_CLASS(DoOperation, qd::operation::args::Base);
    // DECLARE_OPERATION(qd::operation::args::DoOperation, qd::operation::DoOperation);
    qd::Var16 arg0;

    DoOperation() = default;
};

struct OperationSupportedMsgVisitor : public operation::args::Base {
    qd::vector<const qd::TypeInfo*> m_pSupportedMtd;

public:
    virtual bool tryCast(const qd::TypeInfo& msg_type) override;
};
//////////////////////////////////////////////////////////////////////////



}; // namespace operation::args
}; // namespace qd
