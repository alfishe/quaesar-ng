#pragma once
#include <qd/Core/nodeBase.h>
#include <qd/Mem/fnvHash.h>
#include "qd/Base/variant16.h"


namespace qd::operation::msg {

template<int>
struct Base_;


struct Base {
    TS_REFLECT_CLASS(operation::msg::Base, void);
    template<int TClassId>
    friend struct Base_;

public:
    inline Base() {}

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

}; // struct msg::Base
//////////////////////////////////////////////////////////////////////////


template<int TClassId>
struct Base_ : public Base {
    static constexpr int ID = TClassId;
    Base_()
        : Base(TClassId)
    {}
}; // struct Base_



struct DoOperation : public Base {
    TS_REFLECT_CLASS(DoOperation, operation::msg::Base);
    qd::Var16 arg0;

    DoOperation() = default;
};



}; // namespace qd::operation::msg
