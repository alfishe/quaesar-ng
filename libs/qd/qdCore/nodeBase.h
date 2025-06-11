#pragma once
#include <EASTL/fixed_vector.h>
#include <EASTL/vector.h>
#include <qd/qdSTL/forwardDecl.h>

#include <qd/qdBase/ref_ptr.h>
#include <qd/qdBase/types.h>
#include <qd/qdTypeSystem/typeDeclare.h>


namespace qd {
class Node;
struct NodeMsgProcVisitor;
class NodeComp;
class TypeInfo;
class INodesChildList;


struct NodeMessage {
    TS_REFLECT_CLASS_BASE(1000, qd::NodeMessage, void);

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


struct NodeCreator {
    Node* parent = nullptr;

    template<class TClass, typename... TArgs>
    TClass* createNode_(TArgs&&... args)
    {
        TClass* pNode = new TClass(std::forward<TArgs>(args)...);
        pNode->onNodeCreated((NodeCreator*)this);
        return pNode;
    } // createNode_

}; // struct NodeCreator



class INodeIterator
{
public:
    virtual bool hasNext() const = 0;
    virtual Node* next() = 0;
    virtual ~INodeIterator() = default;
};


class NodeIterator
{
protected:
    static constexpr size_t IteratorBufferSize = 256;
    static constexpr size_t IteratorBufferAlignment = alignof(std::max_align_t);
    std::aligned_storage_t<IteratorBufferSize, IteratorBufferAlignment> m_inplaceBuf;
    INodeIterator* m_pIter = nullptr;

public:
    bool hasNext() const { return m_pIter->hasNext(); };
    Node* next() { return m_pIter->next(); }
    ~NodeIterator();

    template<typename TIter, typename... Args>
    TIter& makeIter_(Args&&... args)
    {
        assert(!m_pIter);
        static_assert(std::is_base_of_v<INodeIterator, TIter>, "TIter must be derived from NodeIterator");
        static_assert(sizeof(TIter) < IteratorBufferSize);
        m_pIter = new (&m_inplaceBuf) TIter(std::forward<Args>(args)...);
        return static_cast<TIter&>(*m_pIter);
    }
}; // class NodeIterator





//////////////////////////////////////////////////////////////////////////
class Node : public RefCounted
{
    TS_BEGIN_REFLECT_CLASS_BASE(9000, qd::Node, void);
    TS_END();

 public:
    Node* const m_pParent = nullptr;
    INodesChildList* m_pChildList = nullptr;
    eastl::fixed_vector<NodeComp*, 6, true> m_pComps;

public:
    Node() = default;
    virtual ~Node();

    virtual void onNodeCreated(NodeCreator* mk);
    virtual EFlow onNodeMessageProc(qd::NodeMessage* in_msg);

    int getNumChild() const;
    Node* getChild(int idx) const;

    decltype(auto) getCompsBegin() { return m_pComps.begin(); }
    decltype(auto) getCompsEnd() { return m_pComps.end(); }

    template<class TComp, typename... TArgs>
    TComp* createComp_(TArgs&&... args)
    {
        if (TComp* pExist = getComp_<TComp>())
            return pExist;
        NodeCreator cp;
        cp.parent = this;
        TComp* newComp = cp.createNode_<TComp>(args...);
        addComp(newComp);
        return newComp;
    }

    void addComp(NodeComp* newComp);
    NodeComp* findComp(const qd::TypeInfo& id) const;
    Node* findChildNode(const qd::TypeInfo& ti);
    Node* findParentNode(const qd::TypeInfo& needType) const;

    template<class TComp>
    inline TComp* getComp_() const
    {
        NodeComp* pComp = findComp(TComp::getStaticTypeInfo());
        return static_cast<TComp*>(pComp);
    }

    template<class T>
    T* findParentComp_() const
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

    // find components by interface with dynamic_cast
    template<class TComp>
    inline TComp* getCompI_() const
    {
        NodeComp* pComp = findComp(TComp::getStaticTypeInfo());
        return dynamic_cast<TComp*>(pComp);
    }

    template<class T>
    T* findParentCompI_() const
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
    T* findParentNode_() const
    {
        const qd::TypeInfo& needType = qd::typeof_<T>();
        return static_cast<T*>(findParentNode(needType));
    }

    qd::Node* getParent() const { return m_pParent; }
    void setParent(qd::Node* Parent);

    virtual void destroy();


}; // class Node
//////////////////////////////////////////////////////////////////////////



class NodeComp : public Node
{
    TS_REFLECT_CLASS(qd::NodeComp, qd::Node);

public:
    NodeComp() {}
};



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
    virtual void onNodeCreated(qd::NodeCreator* mk) { TSuper::onNodeCreated(mk); }
    virtual int getNumChild() override;
    virtual Node* getChild(int idx) override;
    virtual bool addChild(Node* child) override;
    virtual bool removeChild(Node* child) override;
    virtual bool beginIter(NodeIterator& buf) override;

}; // class NodesChildList
//////////////////////////////////////////////////////////////////////////



}; // namespace qd
