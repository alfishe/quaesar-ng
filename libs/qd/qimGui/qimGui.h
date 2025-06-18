#pragma once
#include "qd/stl/string.h"
#include "qd/stl/vector_map.h"
#include "qd/typeSystem/typeDeclare.h"
#include <imgui/imgui.h>
#include "qd/typeSystem/typeInfo.h"


template<typename T>
class qim_ptr
{
private:
    T* m_ptr;

public:
    qim_ptr() = default;
    qim_ptr(T* ptr)
        : m_ptr(ptr)
    {}

    T* get() const { return m_ptr.get(); }

    T& operator* () const
    {
        assert(m_ptr);
        return *m_ptr;
    }

    T* operator->() const
    {
        assert(m_ptr);
        return m_ptr;
    }

    explicit operator bool () const { return static_cast<bool>(m_ptr); }

    qim_ptr(const qim_ptr&) = delete; // Disable copy constructor
    qim_ptr& operator= (const qim_ptr&) = delete;

    qim_ptr(qim_ptr&& other) noexcept
        : m_ptr(other.m_ptr)
    {
        other.m_ptr = nullptr; // Transfer ownership
    }

    qim_ptr& operator= (qim_ptr&& other) noexcept
    {
        if (this != &other)
        {
            if (m_ptr)
                qim::endCtrl(m_ptr);
            m_ptr = other.m_ptr;
            other.m_ptr = nullptr;
        }
        return *this;
    }

    qim_ptr(T* ptr, bool takeOwnership)
        : m_ptr(ptr)
    {
        if (takeOwnership && m_ptr)
        {
            qim::beginCtrl(m_ptr); // Assuming beginCtrl takes ownership of the pointer
        }
    }

    T* release()
    {
        T* temp = m_ptr;
        m_ptr = nullptr; // Release ownership
        return temp;
    }

    ~qim_ptr();
};



namespace qim {
class Context;
class ElementBeh;


qim::Context* getContext();
inline static Context* g_pCtx = getContext();

void beginFrame();
void endFrame();



//////////////////////////////////////////////////////////////////////////
class ElementData
{
    TS_REFLECT_CLASS_BASE(100, ElementData, void);

    const ElementBeh* m_pBehavior = nullptr; // Behavior class that this element data belongs to

public:
    virtual ~ElementData() = default;

    void setup(const char* text) {}

    virtual const qd::TypeInfo* getBehaviorClass() const { return nullptr; }
    const qim::ElementBeh* getBehavior() const { return m_pBehavior; }

    virtual void onAttach(const ElementBeh* pBehavior) { m_pBehavior = pBehavior; }
    virtual void onDetach() { m_pBehavior = nullptr; }

    virtual void onBegin(qim::Context* ctx) {}
    virtual void onEnd(qim::Context* ctx) {}


    template<class T>
    T* cast_() const
    {
        const qd::TypeInfo& castToType = T::getStaticTypeInfo();
        const qd::TypeInfo& lh = getTypeInfo();
        if (lh.isDerivedFrom(castToType))
            return static_cast<T*>(const_cast<ElementData*>(this));
        return nullptr;
    }

    template<class T>
    qim_ptr<T> beginChild_(const char* name_id) const;




}; // class ElementData
//////////////////////////////////////////////////////////////////////////


struct ElemBehCreator {};


template<class TClass>
static qim::ElementBeh* createElemBehCb_(const qd::TypeInfo& /*meta*/, qim::ElemBehCreator* cp)
{
    TClass* pNewInst = new TClass();
    pNewInst->onConstruct(cp);
    return pNewInst;
}


class ElementBeh
{
    TS_REFLECT_CLASS_BASE(100, ElementBeh, void);

public:
    virtual ~ElementBeh() = default;

public:

    virtual void onConstruct(qim::ElemBehCreator* cp)
    {
    }
    virtual ElementData* createElementData(const qd::TypeInfo& type) = 0;

}; // class ElementBeh



class Storage
{
    qd::vector_map<ImGuiID, ElementData*> m_dataMap;

public:
    ElementData* findData(ImGuiID id)
    {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end())
        {
            return it->second;
        }
        return nullptr;
    }

    void setData(ImGuiID slot_id, ElementData* p_data_inst) //
    {
        m_dataMap[slot_id] = p_data_inst;
    }

}; // class Storage
//////////////////////////////////////////////////////////////////////////


class Context
{
    Storage* m_pCurrStorage = new Storage();
    Storage* m_pPrevStorage = new Storage();

    qd::vector_map<const qd::TypeInfo*, ElementBeh*> m_pBehaviors;

    qd::vector<ElementData*> m_pElementsStack;

public:
    void init();
    ~Context();

    ElementBeh* findBehavior(const qd::TypeInfo& pBehClassInfo) const;
    bool getElementData(const char* name_id, qim::ElementData** pOut, const qd::TypeInfo& behClass, const qd::TypeInfo& elemClass) const;

    template<class T, typename... TArgs>
    T* getOrCreateElem_(const char* name_id, TArgs&&... args) const
    {
        ElementData* pElement;
        if (getElementData(name_id, &pElement, T::s_behClass, T::getStaticTypeInfo()))
            return static_cast<T*>(pElement);

        T* pInst = static_cast<T*>(pElement);
        pInst->setup(name_id, std::forward<TArgs>(args)...);
        return pInst;
    }

    void stackPushElement(ElementData* pElem)
    {
        m_pElementsStack.push_back(pElem);
    }

    ElementData* getStackTreeTop()
    {
        return m_pElementsStack.back();
    }

    void stackPopElement(ElementData* pElem)
    {
        ElementData* pBack = m_pElementsStack.back();
        assert(pBack == pElem);
        m_pElementsStack.pop_back();
    }

private:
    void addBehavior(const qd::TypeInfo& pBehClassInfo, ElementBeh* pInst);

}; // struct Context
//////////////////////////////////////////////////////////////////////////


inline void _invokeBegin(Context* ctx, ElementData* pElem)
{
    pElem->onBegin(g_pCtx);
    g_pCtx->stackPushElement(pElem);
}


template<class T>
qim_ptr<T> beginChild_(const char* name_id)
{
    T* pElem = g_pCtx->getOrCreateElem_<T>(name_id);
    if (pElem)
        _invokeBegin(g_pCtx, pElem);
    return qim_ptr<T>(pElem);
}


inline void endCtrl(ElementData* pElem)
{
    g_pCtx->stackPopElement(pElem);
    pElem->onEnd(g_pCtx);
}


//////////////////////////////////////////////////////////////////////////



}; // namespace qim
//////////////////////////////////////////////////////////////////////////


template<typename T>
qim_ptr<T>::~qim_ptr()
{
    if (m_ptr)
        qim::endCtrl(m_ptr);
}



template<class T>
qim_ptr<T> qim::ElementData::beginChild_(const char* name_id) const
{
    T* pElem = g_pCtx->getOrCreateElem_<T>(name_id);
    if (pElem)
        _invokeBegin(g_pCtx, pElem);
    return qim_ptr<T>(pElem);
}
