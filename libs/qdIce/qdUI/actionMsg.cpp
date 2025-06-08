#include "actionMsg.h"
#include "qdIce/qdTypeSystem/typeInfo.h"


namespace qd::action::msg {


bool Base::tryCast(const qd::TypeInfo& msg_type)
{
    const qd::TypeInfo& typeInfo = getTypeInfo();
    return typeInfo.isDerivedFrom(msg_type);
}

};
