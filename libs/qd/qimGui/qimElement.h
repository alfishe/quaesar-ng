#pragma once
#include "qd/base/point2.h"
#include "qd/base/ref_ptr.h"
#include "qd/base/tribool.h"
#include "qd/qimGui/qimBase.h"
#include "qd/qimGui/qimPtr.h"
#include "qd/qimGui/qimProperty.h"
#include "qd/typeSystem/attributesCommon.h"
#include "EASTL/hash_map.h"
#include "qd/typeSystem/typeInfo.h"


FORWARD_DECLARATION_3S(qim, msg, Base);


namespace qim {
class BehaviorElem;
class CtrlElement;
class Property;


//////////////////////////////////////////////////////////////////////////
class Element : public qd::RefCounted
{
    TS_REFLECT_CLASS_BASE(100, qim::Element, void);

public:
    ElemId m_localId = 0;

    int m_nStrongRefs = 0;
    int m_nWeakRefs = 0;

    const BehaviorElem* m_pBehavior = nullptr; // Behavior class that this element data belongs to

    Element* m_pParentElem = nullptr;

    Element* m_pNextElem = nullptr;
    Element* m_pPrevElem = nullptr;

    int m_nComps = 0;
    BehaviorElem* m_pCompsRoot = nullptr;
    int m_nChilds = 0;
    CtrlElement* m_pChildRoot = nullptr;

    CtrlElement* m_pTemplate = nullptr;
    eastl::hash_map<uint32_t, ref_ptr<qim::Property>> m_pProperties;
    bool m_bIsNew = true;


public:
    virtual ~Element() = default;

    void setup(const char* text) {}

    virtual const qd::TypeInfo* getBehaviorClass() const { return nullptr; }
    const qim::BehaviorElem* getBehavior() const { return m_pBehavior; }

    virtual void onAttach(const BehaviorElem* pBehavior) { m_pBehavior = pBehavior; }
    virtual void onDetach() { m_pBehavior = nullptr; }

    bool isNew() const { return m_bIsNew; }

    template<class T>
    T* cast_() const
    {
        const qd::TypeInfo& castToType = T::getStaticTypeInfo();
        const qd::TypeInfo& lh = getTypeInfo();
        if (lh.isDerivedFrom(castToType))
            return static_cast<T*>(const_cast<Element*>(this));
        return nullptr;
    }

    template<class T>
    qptr<T> childAdd_(const char* name_id) const;

    virtual qd::EFlow onMessageProcImp(qim::msg::Base& in_msg) { return qd::EFlow::CONTINUE; }

    qd::EFlow onMessageProc(qim::msg::Base& in_msg) { return notifyComps(in_msg); }

    qd::EFlow notifyComps(qim::msg::Base& in_msg);


    qd::EFlow notifyParents(qim::msg::Base& in_msg)
    {
        Element* pCurParent = m_pParentElem;
        while (pCurParent)
        {
            qd::EFlow f = pCurParent->onMessageProc(in_msg);
            if (f == qd::EFlow::STOP)
                return f;
            pCurParent = pCurParent->m_pParentElem;
        }
        return qd::EFlow::CONTINUE;
    }

    qd::EFlow notifyCompsOrParents(qim::msg::Base& in_msg)
    {
        qd::EFlow f = notifyComps(in_msg);
        if (f == qd::EFlow::STOP)
            return f;
        return notifyParents(in_msg);
    }


    Property* propFindByCid(uint32_t cid, bool include_parents) const;
    Property* propFindLocalByCid(uint32_t cid) const;
    void propAdd(ref_ptr<Property> pProp);

    template<class T>
    T& propAdd_()
    {
        if (Property* pProp = propFindLocalByCid(T::CID))
            return *(static_cast<T*>(pProp));
        ref_ptr<T> pNewProp(new T());
        propAdd(pNewProp);
        return *pNewProp.get();
    }
    template<class T>
    T* propFind_(bool include_parents = true)
    {
        if (Property* pProp = propFindByCid(T::CID, include_parents))
            return static_cast<T*>(pProp);
        return nullptr;
    }


    void setParent(qim::Element* ParentElem) { m_pParentElem = ParentElem; }
    qim::Element* getParent() const { return m_pParentElem; }

    template<class T>
    T* getParent_() const {
        if (!m_pParentElem)
            return nullptr;
        if (!m_pParentElem->getTypeInfo().isDerivedFrom_<T>())
            return nullptr;
        return static_cast<T*>(m_pParentElem);
    }


}; // class Element
//////////////////////////////////////////////////////////////////////////





struct ElemBehCreator {};


template<class TClass>
static qim::BehaviorElem* createElemBehCb_(const qd::TypeInfo& /*meta*/, qim::ElemBehCreator* cp)
{
    TClass* pNewInst = new TClass();
    pNewInst->onConstruct(cp);
    return pNewInst;
}


class BehaviorElem : public Element
{
    TS_REFLECT_CLASS_BASE(100, BehaviorElem, qim::Element);

public:
    virtual ~BehaviorElem() = default;

public:
    virtual void onConstruct(qim::ElemBehCreator* cp) {}
    virtual Element* createElementData(const qd::TypeInfo& type) = 0;

}; // class BehaviorElem
//////////////////////////////////////////////////////////////////////////



class CtrlElement : public Element
{
    TS_REFLECT_CLASS_BASE(50, qim::CtrlElement, qim::Element);
    using Color = Props::Color;
    using Text = Props::Text;

    qd::Tribool m_inPropsSection;
    qd::Tribool m_inChildSection;
    bool m_bVisible = true;

public:

    virtual void onBeginImp(qim::Context* ctx) {}
    virtual void onEndImp(qim::Context* ctx) {}
    virtual void onPropEndImp() {}

    bool isVisible(bool bCheckParents = false) const;
    bool setVisible(bool bVisible);

    void onBegin(qim::Context* ctx)
    {
        m_inPropsSection = qd::Tribool::True;
        m_inChildSection = qd::Tribool::Undef;
        onBeginImp(ctx);
    }
    void onEnd(qim::Context* ctx) {}

    void markPropsEnd()
    {
        if (m_inPropsSection.isFalse())
            return;
        m_inPropsSection = qd::Tribool::False;
        onPropEndImp();
    }

    virtual bool onSectChildBeginImp()
    {
        return true;
    }

    virtual void onSectChildEndImp() {}

    bool sectChildBegin()
    {
        if (m_inChildSection.isTrue())
            return false;
        m_inChildSection = qd::Tribool::True;
        bool bVis = onSectChildBeginImp();
        return bVis;
    }

    void sectChildEnd()
    {
        if (m_inChildSection.maybeFalse())
            return;
        m_inChildSection = qd::Tribool::False;
        onSectChildEndImp();
    }

    virtual bool isHovered();
    virtual bool isClicked(int mb = 0);
    virtual bool isMouseDown(int) { return false; }
    virtual bool isMouseReleased(int) { return false; }

}; // class CtrlElement




template<typename T>
qptr<T>::~qptr()
{
    if (m_ptr)
        qim::endCtrl(m_ptr);
}



template<class T>
qptr<T> qim::Element::childAdd_(const char* name_id) const
{
    Context* pCtx = getCurrentContext();
    T* pElem = pCtx->getOrCreateElem_<T>(name_id);
    if (pElem)
        _invokeBegin(pCtx, pElem);
    return qptr<T>(pElem);
}


}; // namespace qim
