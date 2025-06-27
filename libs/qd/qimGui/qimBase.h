#pragma once
#include "qd/base/base.h"
#include "qd/base/classPrimeId.h"
#include "qd/base/color.h"
#include "qd/base/flowEnum.h"
#include "qd/enum/enumBase.h"
#include "qd/stl/string.h"
#include "qd/typeSystem/typeDeclare.h"


namespace qim {

class Context;
class BehaviorElem;
class Element;
class ElementData;
class CtrlElement;
class CompElement;
using ElemId = uint32_t;


#define QIM_ELEMENT_CLASS(ElemClass, BaseClass, BehClass)                    \
    TS_BEGIN_REFLECT_CLASS(ElemClass, BaseClass);                            \
    TS_ATTRIBUTE(qd::tsAttr::CreateClassCb(&qim::createElemCb_<TRefClass>)); \
    TS_END();                                                                \
    friend class BehClass;                                                   \
    using TBehClass = BehClass;                                              \
    inline static const qd::TypeInfo& s_behClass = qd::typeof_<BehClass>();  \
    virtual const qd::TypeInfo* getBehaviorClass() const override            \
    {                                                                        \
        return &ElemClass::s_behClass;                                       \
    }                                                                        \
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


struct PropertyClassMeta {
    uint32_t cid = 0;
    qd::ClassPrimeId primeId;
    ESectType sectType = ESectType::Proprty;
    EVisitStage visitsAllowed = EVisitStage::UNDEF;
    bool isDefined() const { return cid; }
};

template<class TProp>
PropertyClassMeta& get_prop_class_meta_()
{
    static PropertyClassMeta inst;
    return inst;
}

class Property
{
public:
    const PropertyClassMeta* m_classMeta = nullptr;
    int _nStrongRefs = 0;
    int _nWeakRefs = 0;

    Property() {}

    const qim::PropertyClassMeta& getClassMeta() const { assert(m_classMeta); return *m_classMeta; }
    void setClassMeta(const qim::PropertyClassMeta& ClassMeta) { m_classMeta = &ClassMeta; }

    virtual ~Property() = default;

    virtual bool isSectEnterAllowedImp(qim::Context* ctx, ElementData* pData) { return true; }

    bool isSectEnterAllowed(EVisitStage curStage, qim::Context* ctx, ElementData* pData);

public:
    using PrimeIdClassMgr = qd::ClassPrimeIdMgr_<qim::Property>;
protected:
    static const PropertyClassMeta& gen_class_meta();

}; // class Property
//////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////
class Section : public Property
{
public:
    constexpr static ESectType getType() { return ESectType::Section; }

    Section() {}

protected:
    static const PropertyClassMeta& gen_class_meta();

}; // class Section
//////////////////////////////////////////////////////////////////////////



#define QIM_PROPERTY(PropName, TParent, TVisitStage)                       \
    using TThis = PropName;                                                \
    using TSuper = TParent;                                                \
    static const PropertyClassMeta& gen_class_meta()                       \
    {                                                                      \
        PropertyClassMeta& meta = qim::get_prop_class_meta_<PropName>();   \
        if (meta.isDefined())                                              \
            return meta;                                                   \
        const PropertyClassMeta& parentMeta = TParent::gen_class_meta();   \
        meta = parentMeta /*copy*/;                                        \
        meta.cid = qd::fnv1aHash(#PropName);                               \
        meta.primeId = PrimeIdClassMgr::get().registerNewType();           \
        meta.visitsAllowed = parentMeta.visitsAllowed | (TVisitStage);     \
        return meta;                                                       \
    }                                                                      \
    inline static const PropertyClassMeta& s_classMeta = gen_class_meta(); \
    static constexpr uint32_t CID = qd::fnv1aHash(#PropName);



}; // namespace qim
