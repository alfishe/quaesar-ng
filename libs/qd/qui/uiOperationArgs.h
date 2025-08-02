#pragma once
#include "qd/node/node.h"
#include "qd/mem/fnvHash.h"
#include "qd/base/variant16.h"
#include "qd/typeSystem/typeDeclare.h"


#define DECLARE_OPERATION(TArgClass, TOpClass) \
    TS_REFLECT_CLASS(TArgClass, qd::operation::args::Base); \
    inline static const qd::TypeInfo& s_OperationType = qd::typeof_by_name(#TOpClass);



namespace qd::operation::args {

template<int>
struct Base_;


struct Base {
    TS_REFLECT_CLASS(qd::operation::args::Base, void);
    template<int TClassId>
    friend struct Base_;

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

}; // struct args::Base
//////////////////////////////////////////////////////////////////////////




struct DoOperation : public Base {
    TS_REFLECT_CLASS(DoOperation, qd::operation::args::Base);
    //DECLARE_OPERATION(qd::operation::args::DoOperation, qd::operation::DoOperation);
    qd::Var16 arg0;

    DoOperation() = default;
};



}; // namespace qd::operation::args
