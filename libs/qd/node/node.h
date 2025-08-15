#pragma once
#include <EASTL/fixed_vector.h>
#include <EASTL/vector.h>
#include <qd/stl/forwardDecl.h>

#include <qd/stl/ref_ptr.h>
#include <qd/base/baseTypes.h>
#include <qd/typeSystem/typeDeclare.h>
#include "qd/base/eFlow.h"


namespace qd {
class Node;
struct NodeMsgProcVisitor;
struct NodeCreator;
class NodeComp;
class TypeInfo;
class INodesChildList;
class NodeIterator;


struct NodeMessage {
    TS_REFLECT_CLASS_BASE(400, qd::NodeMessage, void);

public:
    NodeMessage() = default;

    template<class T>
    T* cast_() const
    {
        if (!c_def(this))
            return nullptr;
        const qd::TypeInfo& type = T::getStaticTypeInfo();
        if (!tryCast(type))
            return nullptr;
        return static_cast<T*>(const_cast<NodeMessage*>(this));
    }

    bool tryCast(const qd::TypeInfo& msg_type) const;

}; // struct



//////////////////////////////////////////////////////////////////////////
class Node : public qd::RefCounted
{
    TS_BEGIN_REFLECT_CLASS_BASE(200, qd::Node, void);
    TS_END();

 public:
    Node* const m_pParent = nullptr;
    INodesChildList* m_pChildList = nullptr;
    eastl::fixed_vector<qd::NodeComp*, 6, true> m_pComps;

public:
    Node() = default;
    virtual ~Node();
    virtual void destroy();

    virtual void onNodeCreated(qd::NodeCreator* mk);
    virtual EFlow onNodeMessageProc(qd::NodeMessage* in_msg);

    int getNumChild() const;
    Node* getChild(int idx) const;
    Node* findChildNode(const qd::TypeInfo& ti);
    Node* findParentNode(const qd::TypeInfo& needType) const;
    template<class T>
    T* findParentNode_() const;

    qd::Node* getParent() const { return m_pParent; }
    void setParent(qd::Node* Parent);

    // Components
    decltype(auto) getCompsBegin() { return m_pComps.begin(); }
    decltype(auto) getCompsEnd() { return m_pComps.end(); }

    template<class TComp, typename... TArgs>
    TComp* createComp_(TArgs&&... args);

    void addComp(NodeComp* newComp);
    NodeComp* findComp(const qd::TypeInfo& id) const;

    template<class TComp>
    TComp* getComp_() const;

    template<class T>
    T* findParentComp_() const;

    // find components by interface with dynamic_cast
    template<class TComp>
    TComp* getCompI_() const;

    template<class T>
    T* findParentCompI_() const;


}; // class Node
//////////////////////////////////////////////////////////////////////////


template<class TComp, typename ...TArgs>
TComp* Node::createComp_(TArgs&&... args)
{
    if (TComp* pExist = getComp_<TComp>())
        return pExist;
    NodeCreator cp;
    cp.parent = this;
    TComp* newComp = cp.make_<TComp>(args...);
    addComp(newComp);
    return newComp;
}


template<class TComp>
TComp* Node::getComp_() const
{
    NodeComp* pComp = findComp(TComp::getStaticTypeInfo());
    return static_cast<TComp*>(pComp);
}


template<class T>
T* Node::findParentComp_() const
{
    Node* pCurNode = m_pParent;
    while (pCurNode)
    {
        if (T* pFoundComp = pCurNode->getComp_<T>())
            return pFoundComp;
        pCurNode = pCurNode->m_pParent;
    }
    return nullptr;
}


template<class TComp>
TComp* Node::getCompI_() const
{
    NodeComp* pComp = findComp(TComp::getStaticTypeInfo());
    return dynamic_cast<TComp*>(pComp);
}


template<class T>
T* Node::findParentCompI_() const
{
    Node* pCurNode = m_pParent;
    while (pCurNode)
    {
        if (T* pFoundComp = pCurNode->getCompI_<T>())
            return pFoundComp;
        pCurNode = pCurNode->m_pParent;
    }
    return nullptr;
}


template<class T>
T* Node::findParentNode_() const
{
    const qd::TypeInfo& needType = qd::typeof_<T>();
    return static_cast<T*>(findParentNode(needType));
}


//////////////////////////////////////////////////////////////////////////



class NodeComp : public Node
{
    TS_REFLECT_CLASS(qd::NodeComp, qd::Node);

public:
    NodeComp() {}
}; // class NodeComp



class INodesChildList : public qd::NodeComp
{
    TS_REFLECT_CLASS(qd::INodesChildList, qd::NodeComp);

public:
    // clang-format off
    virtual void onNodeCreated(qd::NodeCreator* mk) { TSuper::onNodeCreated(mk); }
    virtual int getNumChild() { return 0; }
    virtual Node* getChild(int idx) { return nullptr; }
    virtual bool beginIter(NodeIterator& buf) { return false; }
    virtual bool addChild(Node* child) { assert(0); return false; }
    virtual bool removeChild(Node* child) { return false; }
    // clang-format on
};


//------------------------------------------------------------------------
// Base implementation on NodesList interface
//
class NodesChildList : public qd::INodesChildList
{
    TS_REFLECT_CLASS(qd::NodesChildList, qd::INodesChildList);
    qd::vector<Node*> m_ChildNodes;

public:
    virtual ~NodesChildList();

public:
    virtual void onNodeCreated(qd::NodeCreator* mk) override { TSuper::onNodeCreated(mk); }
    virtual int getNumChild() override;
    virtual Node* getChild(int idx) override;
    virtual bool addChild(Node* child) override;
    virtual bool removeChild(Node* child) override;
    virtual bool beginIter(NodeIterator& buf) override;

}; // class NodesChildList
//////////////////////////////////////////////////////////////////////////



struct NodeCreator {
    Node* parent = nullptr;
    uint32_t id = 0;

    template<class TClass, typename... TArgs>
    TClass* make_(TArgs&&... args)
    {
        TClass* pNode = new TClass(args...);
        pNode->onNodeCreated(this);
        return pNode;
    } // make_

}; // struct NodeCreator




}; // namespace qd
