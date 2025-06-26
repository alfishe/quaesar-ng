#pragma once
#include "qd/base/base.h"
#include "qd/base/color.h"
#include "qd/base/flowEnum.h"
#include "qd/stl/string.h"
#include "qd/typeSystem/typeDeclare.h"
#include "qd/enum/enumBase.h"


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


struct EVisitStage {
    enum EType : uint32_t {
        UNDEF = 0,
        VCollect = 0x01,
        VProperty = 0x02,
        VChild = 0x04,
        VEventHandler = 0x08,
        VDone = 0x10,
    };
    ENUM_DECLARE_BASE(qim::, EVisitStage, EType, 0);
    ENUM_DECLARE_FLAGS;
};


enum class ESectType {
    Section,
    Proprty,
};

//////////////////////////////////////////////////////////////////////////
class Section
{
public:
    constexpr static EVisitStage s_BaseStage = EVisitStage::UNDEF;

public:
    ESectType m_classType;
    constexpr static ESectType getType() { return ESectType::Section; }
    virtual uint32_t getCID() const = 0;

    Section(ESectType type = getType())
        : m_classType(type)
    {}

    virtual ~Section() {}

}; // class Section


class Property : public Section
{
public:
    constexpr static ESectType getType() { return ESectType::Proprty; }

    int _nStrongRefs = 0;
    int _nWeakRefs = 0;

    Property()
        : Section(getType())
    {}
}; // class Property


#define QIM_PROPERTY(PropName, TParent, TVisitStage)          \
    using TThis = PropName;                                   \
    using TSuper = TParent;                                   \
    static constexpr uint32_t CID = qd::fnv1aHash(#PropName); \
    virtual uint32_t getCID() const override                  \
    {                                                         \
        return CID;                                           \
    }                                                         \
    constexpr static EVisitStage getVisitFlagsStatic()        \
    {                                                         \
        return TSuper::s_BaseStage | TVisitStage;             \
    }; \



}; // namespace qim
