#pragma once
#include <qdIce/qdMem/fnvHash.h>
#include <qdIce/qdCore/nodeBase.h>


namespace qd {


namespace action {

//////////////////////////////////////////////////////////////////////////
namespace msg {

template <int>
struct Base_;


struct Base : public qd::NodeMessage {
    template <int TClassId>
    friend struct Base_;

    template <class T>
    T* cast() {
        if (!c_def(this))
            return nullptr;
        if (mId != T::ID)
            return nullptr;
        return static_cast<T*>(this);
    }

private:
    inline Base(int msg_id) : NodeMessage(msg_id) {
    }
};  // struct msg::Base



template <int TClassId>
struct Base_ : public Base {
    static constexpr int ID = TClassId;
    Base_() : Base(TClassId) {
    }
};  // struct Base_


struct DoAction : Base_<fnv1aHash("DoAction")> {};


};  // namespace msg
//////////////////////////////////////////////////////////////////////////

}; // namespace action
};
