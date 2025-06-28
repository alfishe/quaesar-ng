#pragma once
#include "qd/math/point2.h"
#include "qd/base/ref_ptr.h"
#include "qd/base/tribool.h"
#include "qd/qimGui/qimBase.h"
#include "qd/qimGui/qimPtr.h"
#include "qd/qimGui/qimProperty.h"
#include "qd/typeSystem/attributesCommon.h"
#include "qd/stl/hash_map.h"
#include "qd/typeSystem/typeInfo.h"
#include "qd/log/log.h"
#include <unordered_map>


FORWARD_DECLARATION_3S(qim, msg, Base);


namespace qim {
class BehaviorElem;
class CtrlElement;
class Property;
extern Context* g_pCtx;
Context* getCurrentContext();


struct OnElementConstruct {};


class ElementData
{
public:
    Element* m_pElement = nullptr;
    ImGuiID m_elemId = 0;

    EVisitStage m_supportedVStages = 0;
    EVisitStage m_executedStages = 0;

    std::unordered_map<qd::ClassPrimeId, qim::Property*> m_pProperties;
    qd::ClassPrimeId m_propPrimeHash;
    qd::ClassPrimeId m_eventHappens;
    qd::ClassPrimeId m_eventApplied;

    ElementData() = default;

    Property* propFindByCid(const Context* ctx, const qim::PropertyClassMeta& cid, bool include_parents) const;

    Property* propFindLocalByCid(const qim::PropertyClassMeta& cid) const;
    void propAdd(qim::Property* pProp);

    ImGuiID getId() const {
        return m_elemId;
    }

    template<class T>
    T& propAdd_()
    {
        if (Property* pProp = propFindLocalByCid(T::s_classMeta))
            return *(static_cast<T*>(pProp));
        T* pNewProp = new T();
        pNewProp->setClassMeta(T::s_classMeta);
        propAdd(pNewProp);
        return *pNewProp;
    }
    template<class T>
    T* propFind_(const Context* ctx, bool include_parents = true)
    {
        if (Property* pProp = propFindByCid(ctx, T::s_classMeta, include_parents))
            return static_cast<T*>(pProp);
        return nullptr;
    }

    bool hasQueuedEvents();

    template<class T>
    T* getElem_() const
    {
        assert(!m_pElement || m_pElement->getTypeInfo().isDerivedFrom_<T>());
        return static_cast<T*>(m_pElement);
    }

}; // struct ElementData




//////////////////////////////////////////////////////////////////////////
class Element : public qim::QimBase
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
    bool m_bIsNew = true;

public:
    virtual ~Element() = default;

    virtual void onConstruct(qim::OnElementConstruct* cp) {}

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

//     template<class T, typename... TArgs >
//     qptr<T> childAdd_(const char* name_id, TArgs&&... args) const;

    virtual qd::EFlow onMessageProcImp(qim::msg::Base& in_msg) { return qd::EFlow::CONTINUE; }
    qd::EFlow onMessageProc(qim::msg::Base& in_msg) { return onMessageProcImp(in_msg); }

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

    template<class T>
    T* propFind_(bool include_parents = true)
    {
        Context* ctx = qim::getCurrentContext();
        ElementData* pData = ctx->findElementData(this);
        return pData->propFind_<T>(ctx, include_parents);
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



template<class TClass>
static qim::Element* createElemCb_(const qd::TypeInfo& /*meta*/, qim::OnElementConstruct* cp)
{
    TClass* pNewInst = new TClass();
    pNewInst->onConstruct(cp);
    return pNewInst;
}


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
    virtual Element* createElementData(const qd::TypeInfo& type);

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
    union EventRequistMask {
        uint32_t flags = 0;
        struct {
            bool onClick :1; // Element was clicked
            bool onHover :1;
        };
    };

public:

    virtual void onDrawBeginImp(qim::Context* ctx) {}
    virtual void onBeforeEndImp(qim::Context* ctx) {}
    virtual void onDrawEndImp(qim::Context* ctx) {}

    virtual void onPropEndImp() {}
    virtual qd::EFlow onMessageProcImp(qim::msg::Base& in_msg) override;

    bool isVisible(bool bCheckParents = false) const;
    bool setVisible(bool bVisible);

    void onDrawBegin(qim::Context* ctx);
    void onBeforeDrawEnd(qim::Context* ctx)
    {
        onBeforeEndImp(ctx);
    }
    void onDrawEnd(qim::Context* ctx)
    {
        onDrawEndImp(ctx);
    }

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

//     bool isClicked()
//     {
//         m_eventRequest.onClick = true;
//         return m_eventHappens.onClick;
//     }

    virtual bool isHovered();
    virtual bool isMouseDown(int) { return false; }
    virtual bool isMouseReleased(int) { return false; }

    //virtual bool pollLoopEventImp() { return false; }

}; // class CtrlElement
//////////////////////////////////////////////////////////////////////////



template<typename T>
qptr<T>::~qptr()
{
    if (m_ptr)
        qim::endCtrl(m_ptr);
}



// template<class T, typename... TArgs>
// qptr<T> Element::childAdd_(const char* name_id, TArgs&&... args) const
// {
//     T* pElem = qim::beginCtrl_<T>(name_id, std::forward<TArgs>(args)...);
//     return qptr<T>(pElem);
// }


}; // namespace qim
