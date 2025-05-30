#pragma once
#include <qdIce/qdCore/nodeBase.h>
#include <qdIce/qdMem/fnvHash.h>


namespace qd {
namespace action {
namespace msg {

template<int>
struct Base_;


struct Base : public qd::NodeMessage {
    TS_REFLECT_CLASS(action::msg::Base, qd::NodeMessage);
    template<int TClassId>
    friend struct Base_;

    template<class T>
    T* cast_()
    {
        if (!c_def(this))
            return nullptr;
        if (id != T::CID)
            return nullptr;
        return static_cast<T*>(this);
    }

public:
    inline Base(int msg_id = 0)
        : qd::NodeMessage(msg_id)
    {}

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


}; // namespace msg
}; // namespace action
}; // namespace qd
