#pragma once
#include "qd/base/base.h"
#include "qd/base/color.h"
#include "qd/base/flowEnum.h"
#include "qd/stl/string.h"
#include "qd/typeSystem/typeDeclare.h"



namespace qim {

class Context;
class BehaviorElem;
class Element;
class CtrlElement;
class CompElement;
using ElemId = uint32_t;


#define QIM_ELEMENT_CLASS(ElemClass, BaseClass, BehClass)                       \
    TS_BEGIN_REFLECT_CLASS(ElemClass, BaseClass);                               \
    TS_ATTRIBUTE(qd::tsAttr::CreateClassCb(&qim::createElemCb_<TRefClass>)); \
    TS_END();                                                                   \
    friend class BehClass;                                                      \
    using TBehClass = BehClass;                                                 \
    inline static const qd::TypeInfo& s_behClass = qd::typeof_<BehClass>();     \
    virtual const qd::TypeInfo* getBehaviorClass() const override               \
    {                                                                           \
        return &ElemClass::s_behClass;                                          \
    }                                                                           \
                                                                                \
public:
//////////////////////////////////////////////////////////////////////////




}; // namespace qim
