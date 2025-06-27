#pragma once
#include "qd/qimGui/qimBase.h"
#include "qd/qimGui/qimElement.h"
#include "qd/stl/vector_map.h"
#include "qd/stl/hash_map.h"



namespace qim {
class Storage;
class Element;



//////////////////////////////////////////////////////////////////////////
class Context
{
    Storage* m_pCurrStorage = nullptr;
    Storage* m_pPrevStorage = nullptr;

    struct StackItem {
        Element* m_pElement = nullptr;
        ElementData* m_pElemData = nullptr;
        EVisitStage m_curVisitStage = EVisitStage::UNDEF;
    };
    qd::vector<StackItem> m_pChildStack;

    qd::vector_map<const qd::TypeInfo*, BehaviorElem*> m_pBehaviors;
    qd::hash_map<const Element*, ElementData*> m_pElemDataMap;

public:
    void init();
    void done();

    Context();
    ~Context();

    BehaviorElem* findBehavior(const qd::TypeInfo& pBehClassInfo) const;
    qim::Element* getOrCreateElement(const char* name_id, const qd::TypeInfo& behClass,
        const qd::TypeInfo& elemClass);

    template<class T, typename... TArgs>
    T* getOrCreateElem_(const char* name_id, TArgs&&... args)
    {
        Element* pElement = getOrCreateElement(name_id, T::s_behClass, T::getStaticTypeInfo());
        if (!pElement)
            return nullptr;
        return static_cast<T*>(pElement);
    }

    bool checkSectStage(EVisitStage suppStages, size_t& nFor);

    void endSect(Section* pOutSect);

    EVisitStage getCurVisitStage() const
    {
        const Context::StackItem& item = m_pChildStack.back();
        return item.m_curVisitStage;
    }
    void setCurVisitStage(EVisitStage st)
    {
        Context::StackItem& item = m_pChildStack.back();
        item.m_curVisitStage = st;
    }

    template<class T>
    T* makeSect_()
    {
        return new T();
    }


    template<class T, typename... TArgs>
    T* getOrCreateSect_(ElementData* pParentElem, TArgs&&... args)
    {
       return &pParentElem->propAdd_<T>();
    }


    Context::StackItem& stackPushElement(Element* pElem);
    void stackPopChild(Element* pElem);

    Element* getStackTreeTopElem(int off = 0) const
    {
        if (!off)
            return m_pChildStack.back().m_pElement;
         auto it = m_pChildStack.rbegin() + -off;
         return it->m_pElement;
    }

    ElementData* getStackTreeTopElemData() const;

    ElementData* findElementData(const Element* pElem) const;

    void beginCtrl(CtrlElement* pElem)
    {
        Context* ctx = this;
        pElem->onBegin(ctx);
        ctx->stackPushElement(pElem);
    }

    qd::EFlow endCtrl(CtrlElement* pElem)
    {
        Context* ctx = this;
        pElem->onBeforeEnd(ctx);

        qd::EFlow rr;
        EVisitStage newVState;
        rr = ctx->onCtrlVisitLoopEnd(pElem, &newVState);
        if (rr == qd::EFlow::REPEAT)
        {
            setCurVisitStage(newVState);
            return qd::EFlow::REPEAT;
        }
        setCurVisitStage(newVState);

        ctx->stackPopChild(pElem);
        pElem->onEnd(ctx);
        pElem->m_bIsNew = false;
        return qd::EFlow::STOP;
    }

    qd::EFlow onCtrlVisitLoopEnd(CtrlElement* pElem, EVisitStage* pOutVisit) const;

    void endFrame();

private:
    void addBehavior(const qd::TypeInfo& pBehClassInfo, BehaviorElem* pInst);

}; // struct Context
//////////////////////////////////////////////////////////////////////////


}; // namespace qim
