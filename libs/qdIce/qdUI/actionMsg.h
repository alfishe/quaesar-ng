#pragma once
#include <qdIce/qdCore/nodeBase.h>
#include <qdIce/qdMem/fnvHash.h>


namespace qd::action::msg {

template<int>
struct Base_;


struct Base {
    TS_REFLECT_CLASS(action::msg::Base, void);
    template<int TClassId>
    friend struct Base_;

public:
    inline Base()
    {}

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


struct DoAction : public Base
{
    TS_REFLECT_CLASS(DoAction, action::msg::Base);
};



}; // namespace qd
