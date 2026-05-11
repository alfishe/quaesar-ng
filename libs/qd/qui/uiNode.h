#pragma once
#include "qd/base/base.h"
#include "qd/stl/ref_ptr.h"
#include "qd/typeSystem/typeDeclare.h"
#include "qd/typeSystem/typeInfo.h"
#include "qd/base/EFlow.h"
#include "qd/stl/fixed_vector.h"
#include "qd/stl/unique_ptr.h"


namespace qd {
class UiNode;
class UiNodeComp;


struct UiMessage {
    uint32_t id;
    UiMessage(uint32_t _id = 0)
        : id(_id)
    {}

    template<class T>
    T* cast_() const
    {
        if (!c_def(this))
            return nullptr;
        if (id != T::ID)
            return nullptr;
        return static_cast<T*>(const_cast<UiMessage*>(this));
    }

}; // struct UiMessage

template<uint32_t TId>
struct UiMessage_ : public UiMessage {
    static constexpr uint32_t ID = TId;
    UiMessage_()
        : UiMessage(TId)
    {}

#define UI_MSG_BASE(name) qd::UiMessage_<SCID(name)>

}; // struct UiMessage_
//////////////////////////////////////////////////////////////////////////


struct UiNodeCreator {
    UiNode* parent = nullptr;
    uint32_t id = 0;

    template<class TClass, typename... TArgs>
    TClass* make_(TArgs&&... args)
    {
        TClass* pNode = new TClass(args...);
        pNode->onUiNodeCreated(this);
        return pNode;
    } // make_

}; // struct UiNodeCreator
//////////////////////////////////////////////////////////////////////////



//------------------------------------------------------------------------
//
class UiNode : public qd::RefCounted
{
    TS_BEGIN_REFLECT_CLASS_BASE(50, qd::UiNode, void);
    TS_END();

    static constexpr uint32_t UNDEF_ID = 0;

protected:
    uint32_t m_id = 0;
    qtd::fixed_vector<qtd::unique_ptr<qd::UiNodeComp>, 4, true> m_pComps;
    UiNode* m_pParent = nullptr;

    struct ChildItem {
        uint32_t id = 0;
        ref_ptr<qd::UiNode> ptr;
        const qd::TypeInfo* typeInfo = nullptr;
        uint32_t getId() const { return id; }
        UiNode* get() const { return this->ptr; }
        const qd::TypeInfo& getTypeInfo() const {
            assert(typeInfo);
            return *typeInfo;
        }
    };
    qtd::vector<ChildItem> m_pChilds;

protected:
    bool m_bVisible = true;
    bool m_bFocus = false;

public:
    virtual void onUiNodeCreated(qd::UiNodeCreator* mk);
    virtual void destroy();

    void init() {}

    inline UiNode* getParent() const { return m_pParent; }
    void setParent(qd::UiNode* pParent);

    UiNode* findChildById(uint32_t id) const;
    UiNode* findChildByType(const qd::TypeInfo& ti) const;

    template<class TWnd>
    TWnd* findChildByType_() const
    {
        const qd::TypeInfo& wndType = qd::typeof_<TWnd>();
        for (const ChildItem& pWnd : m_pChilds)
        {
            if (!pWnd.ptr)
                continue;
            const qd::TypeInfo& classType = pWnd.getTypeInfo();
            if (classType.isDerivedFrom(wndType))
                return static_cast<TWnd*>(pWnd.get());
        }
        return nullptr;
    }


    int findChildIndex(UiNode* pChild) const;

    int getNumChild() const;
    UiNode* addChild(ref_ptr<qd::UiNode> pChild);
    void removeChild(ref_ptr<UiNode> pChild);

    template<class T, typename... TArgs>
    T* addChild_(const char* p_name = nullptr, TArgs&&... args);

    template<class T>
    T* findChildById_(uint32_t ID) const;

    template<class T>
    T* findChildByIdName_(const char* p_name) const;

    template<class T>
    T* getChildById_(uint32_t ID) const;

    UiNode* getChildById(uint32_t ID) const;
    int getChildIndex(UiNode* pChild) const;
    UiNode* getChildById(std::initializer_list<uint32_t> idList) const;
    UiNode* getChild(int idx) const { return m_pChilds[idx].get(); }

    template<class T>
    T* getParent_() const;

    UiNode* findParentNode(const qd::TypeInfo& needType) const;
    template<class T>
    T* findParentNode_() const;

    uint32_t getId() const { return m_id; }
    void setId(uint32_t newId);
    void setIdByName(const char* p_name) { setId(qd::fnv1aHash(p_name)); }

    virtual qtd::string getText() const { return ""; }

    virtual EFlow onUiNodeMessageProc(qd::UiMessage* in_msg);

public:
    virtual void updateBeforeDraw() {}
    virtual void drawContentImp();
    virtual void drawImp();
    void draw();

    bool isVisible(bool bCheckParents = false) const;
    bool setVisible(bool bVisible);
    void destroyRecursive();

    bool hasFocus() const {
        return m_bFocus;
    };

public:
    // Components
    decltype(auto) getCompsBegin() { return m_pComps.begin(); }
    decltype(auto) getCompsEnd() { return m_pComps.end(); }

    template<class TComp, typename... TArgs>
    TComp* createComp_(TArgs&&... args);

    void addComp(qtd::unique_ptr<qd::UiNodeComp> newComp);
    qd::UiNodeComp* findComp(const qd::TypeInfo& id) const;

    template<class TComp>
    TComp* getComp_() const;

    template<class T>
    T* findParentComp_() const;

    // find components by interface with dynamic_cast
    template<class TComp>
    TComp* getCompI_() const;

    template<class T>
    T* findParentCompI_() const;

}; // class UiNode
//////////////////////////////////////////////////////////////////////////


struct UiNodeCompCreator
{
    UiNode* owner = nullptr;
    uint32_t id = 0;

    template<class TClass, typename... TArgs>
    TClass* make_(TArgs&&... args)
    {
        TClass* pNode = new TClass(args...);
        pNode->onCompCreated(this);
        return pNode;
    } // make_
};


class UiNodeComp : public qd::RefCounted
{
    TS_REFLECT_CLASS(qd::UiNodeComp, void);
    uint32_t m_id = 0;
    UiNode* m_pOwner = nullptr;

public:
    UiNodeComp() {}
    virtual ~UiNodeComp() = default;

    virtual void onCompCreated(UiNodeCompCreator* cp)
    {
        m_id = cp->id;
        m_pOwner = cp->owner;
    }
}; // class UiNodeComp
//////////////////////////////////////////////////////////////////////////



template<class TComp, typename... TArgs>
TComp* UiNode::createComp_(TArgs&&... args)
{
    if (TComp* pExist = getComp_<TComp>())
        return pExist;
    UiNodeCompCreator cp;
    cp.owner = this;
    TComp* newComp = cp.make_<TComp>(args...);
    addComp(newComp);
    return newComp;
}


template<class TComp>
TComp* UiNode::getComp_() const
{
    UiNodeComp* pComp = findComp(TComp::getStaticTypeInfo());
    return static_cast<TComp*>(pComp);
}


template<class T>
T* UiNode::findParentComp_() const
{
    UiNode* pCurNode = m_pParent;
    while (pCurNode)
    {
        if (T* pFoundComp = pCurNode->getComp_<T>())
            return pFoundComp;
        pCurNode = pCurNode->m_pParent;
    }
    return nullptr;
}


template<class TComp>
TComp* UiNode::getCompI_() const
{
    UiNodeComp* pComp = findComp(TComp::getStaticTypeInfo());
    return dynamic_cast<TComp*>(pComp);
}


template<class T>
T* UiNode::findParentCompI_() const
{
    UiNode* pCurNode = m_pParent;
    while (pCurNode)
    {
        if (T* pFoundComp = pCurNode->getCompI_<T>())
            return pFoundComp;
        pCurNode = pCurNode->m_pParent;
    }
    return nullptr;
}


template<class T>
T* UiNode::findParentNode_() const
{
    const qd::TypeInfo& needType = qd::typeof_<T>();
    return static_cast<T*>(findParentNode(needType));
}


void uinode_draw_child(UiNode* pNode);


template<class T>
T* UiNode::getParent_() const
{
    return static_cast<T>(m_pParent);
}


template<class T>
T* UiNode::findChildByIdName_(const char* p_name) const
{
    return static_cast<T*>(findChildById(qd::fnv1aHash(p_name)));
}


template<class T>
T* UiNode::findChildById_(uint32_t ID) const
{
    return static_cast<T*>(findChildById(ID));
}


template<class T>
T* UiNode::getChildById_(uint32_t ID) const
{
    return static_cast<T*>(getChildById(ID));
}


template<class T, typename... TArgs>
T* UiNode::addChild_(const char* p_name, TArgs&&... args)
{
    ref_ptr<T> pChild = new T();
    UiNodeCreator mk;
    mk.parent = this;
    mk.id = p_name ? qd::fnv1aHash(p_name) : 0;
    pChild->onUiNodeCreated(&mk);
    if (p_name)
        pChild->setText(p_name);
    pChild->init(std::forward<TArgs>(args)...);
    UiNode* pNode = addChild(pChild);
    return static_cast<T*>(pNode);
}


}; // namespace qd
