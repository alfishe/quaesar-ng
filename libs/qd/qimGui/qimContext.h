#pragma once
#include "qd/qimGui/qimBase.h"
#include "qd/qimGui/qimElement.h"
#include "qd/stl/vector_map.h"



namespace qim {
class Storage;
class Element;


class Context
{
    Storage* m_pCurrStorage = nullptr;
    Storage* m_pPrevStorage = nullptr;

    qd::vector_map<const qd::TypeInfo*, BehaviorElem*> m_pBehaviors;

    qd::vector<Element*> m_pChildStack;

public:
    void init();
    void done();

    Context();
    ~Context();

    BehaviorElem* findBehavior(const qd::TypeInfo& pBehClassInfo) const;
    bool getElementData(const char* name_id, qim::Element** pOut, const qd::TypeInfo& behClass,
        const qd::TypeInfo& elemClass) const;

    template<class T, typename... TArgs>
    T* getOrCreateElem_(const char* name_id, TArgs&&... args) const
    {
        Element* pElement;
        if (getElementData(name_id, &pElement, T::s_behClass, T::getStaticTypeInfo()))
            return static_cast<T*>(pElement);

        T* pInst = static_cast<T*>(pElement);
        return pInst;
    }

    void stackPushChild(Element* pElem) { m_pChildStack.push_back(pElem); }

    Element* getStackTreeTop() { return m_pChildStack.back(); }

    void stackPopChild(Element* pElem)
    {
        Element* pBack = m_pChildStack.back();
        assert(pBack == pElem);
        m_pChildStack.pop_back();
    }

private:
    void addBehavior(const qd::TypeInfo& pBehClassInfo, BehaviorElem* pInst);

}; // struct Context
//////////////////////////////////////////////////////////////////////////


}; // namespace qim
