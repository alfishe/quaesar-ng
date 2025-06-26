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
        EVisitStage m_visitStage = EVisitStage::UNDEF;
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
    bool getOrCreateElement(const char* name_id, qim::Element** pOut, const qd::TypeInfo& behClass,
        const qd::TypeInfo& elemClass);

    template<class T, typename... TArgs>
    T* getOrCreateElem_(const char* name_id, TArgs&&... args)
    {
        Element* pElement;
        if (getOrCreateElement(name_id, &pElement, T::s_behClass, T::getStaticTypeInfo()))
            return static_cast<T*>(pElement);

        T* pInst = static_cast<T*>(pElement);
        return pInst;
    }

    bool checkSectStage(EVisitStage suppStages, size_t& nFor);

    void endSect(Section* pOutSect);

    EVisitStage getCurVisitStage() const
    {
        const Context::StackItem& item = m_pChildStack.back();
        return item.m_visitStage;
    }
    void setCurVisitStage(EVisitStage st)
    {
        Context::StackItem& item = m_pChildStack.back();
        item.m_visitStage = st;
    }

    template<class T>
    T* makeSect_()
    {
        return new T();
    }


    template<class T, typename... TArgs>
    T* getOrCreateSect_(TArgs&&... args)
    {
        uint32_t cid = T::CID;

        ElementData* pParentElem = getStackTreeTopElemData();
        assert(pParentElem);

        if constexpr (T::getType() == ESectType::Proprty)
            return &pParentElem->propAdd_<T>();

        if constexpr (T::getType() == ESectType::Section)
            return makeSect_<T>();

        return nullptr;
    }


    Context::StackItem& stackPushElement(Element* pElem);
    void stackPopChild(Element* pElem);

    Element* getStackTreeTopElem() const { return m_pChildStack.back().m_pElement; }

    ElementData* getStackTreeTopElemData() const;

    ElementData* findElementData(const Element* pElem) const;

    void beginCtrl(CtrlElement* pElem)
    {
        Context* ctx = this;
        pElem->onBegin(ctx);
        ctx->stackPushElement(pElem);
    }

    void endCtrl(CtrlElement* pElem)
    {
        Context* ctx = this;
        ctx->stackPopChild(pElem);
        pElem->onEnd(ctx);
        pElem->m_bIsNew = false;
    }

    bool nextCtrlLoop(CtrlElement* pElem);

private:
    void addBehavior(const qd::TypeInfo& pBehClassInfo, BehaviorElem* pInst);

}; // struct Context
//////////////////////////////////////////////////////////////////////////


}; // namespace qim
