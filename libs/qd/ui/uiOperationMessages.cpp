#include "uiOperationMessages.h"
#include "qd/typeSystem/typeInfo.h"


namespace qd::operation::msg {


bool Base::tryCast(const qd::TypeInfo& msg_type)
{
    const qd::TypeInfo& typeInfo = getTypeInfo();
    return typeInfo.isDerivedFrom(msg_type);
}

};
