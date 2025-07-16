#include "qimElement.h"


namespace qim {

namespace msg {
struct Base {
    int id;
    Base(int id = 0)
        : id(id)
    {}

    template<class T>
    T* cast() const
    {
        Base* pBase = const_cast<Base*>(this);
        if (!pBase || pBase->id != T::ID)
            return nullptr;
        return static_cast<T*>(pBase);
    }
};

template<int TID>
struct Base_ : public Base {
    constexpr static int ID = TID;
    Base_()
        : Base(TID)
    {}
};

#define QIM_MSG_BASE(name) qim::msg::Base_<SCID(name)>


struct OnElemClicked : QIM_MSG_BASE(OnElemClicked) {
    qim::ElemData* m_pElem = nullptr;
    int m_mouseButton = 0;
};

}; // namespace msg



}; // namespace qim
