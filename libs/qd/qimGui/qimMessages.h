#include "qimElement.h"


namespace qim {

namespace msg {
struct Base {
    int id;
    Base(int id = 0)
        : id(id)
    {}
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
    CtrlElement* m_pElem = nullptr;
    int m_mouseButton = 0;
};

}; // namespace msg



}; // namespace qim
