#include "qimElement.h"



namespace qim
{
	
	namespace msg {
	struct Base {
	    int m_id;
	    Base(int id = 0)
	        : m_id(id)
	    {}
	};
	
	template<int TID>
	struct Base_ : public Base {
	    Base_()
	        : Base(TID)
	    {}
	};
	
	struct OnElemClicked : public Base_<0x1001> {
	    CtrlElement* m_pElem;
	    int m_mouseButton;
	};
	
	}; // namespace msg
	
	
	
}; // namespace qim
