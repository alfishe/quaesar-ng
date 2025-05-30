#pragma once
#include <EASTL/fixed_vector.h>
#include <EASTL/vector.h>
#include <qdIce/qdSTL/forwardDecl.h>

#include <qdIce/qdBase/ref_ptr.h>
#include <qdIce/qdBase/types.h>
#include <qdIce/qdTypeSystem/typeDeclare.h>


namespace qd {
class Node;
struct NodeMsgProcVisitor;
class NodeComp;
class TypeInfo;
class INodeChildList;


struct NodeMessage {
    TS_REFLECT_CLASS_BASE(1000, qd::NodeMessage, void);
    uint32_t id = 0;

public:
    NodeMessage(uint32_t msg_id = 0)
        : id(msg_id)
    {}
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
class Node : public qd::RefCounted
{
    TS_BEGIN_REFLECT_CLASS_BASE(1000, qd::Node, void);
    TS_END();

protected:
    Node* m_pParent = nullptr;
    eastl::fixed_vector<Node*, 6, true> m_pComps;
    INodeChildList* m_pChildList = nullptr;

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

    void addComp(Node* newComp);
    Node* findComp(const THash32& id) const;
    Node* findChildNode(const qd::TypeInfo& ti);

    template<class TComp>
    inline TComp* getComp_() const
    {
        Node* pComp = findComp(TComp::CID);
        return static_cast<TComp*>(pComp);
    }

    template<class T>
    T* findParentComp_() const
    {
        Node* pCurNode = m_pParent;
        while (pCurNode)
        {
            if (Node* pFoundComp = pCurNode->getComp_<T>())
                return static_cast<T*>(pFoundComp);
            pCurNode = pCurNode->m_pParent;
        }
        return nullptr;
    }

    template<class T>
    T* findParentNode_() const
    {
        const qd::TypeInfo& needType = qd::typeof_<T>();
        Node* pCurNode = m_pParent;
        while (pCurNode)
        {
            const qd::TypeInfo& curTypeInfo = pCurNode->getTypeInfo();
            if (curTypeInfo.isDerivedFrom(needType))
                return static_cast<T*>(pCurNode);
            pCurNode = pCurNode->m_pParent;
        }
        return nullptr;
    }

    qd::Node* getParent() const { return m_pParent; }
    void setParent(qd::Node* Parent) { m_pParent = Parent; }
}; // class Node
//////////////////////////////////////////////////////////////////////////



class NodeComp : public Node
{
    TS_REFLECT_CLASS(qd::NodeComp, qd::Node);

public:
    NodeComp() {}
};



class INodeChildList : public qd::NodeComp
{
    TS_REFLECT_CLASS(qd::INodeChildList, qd::NodeComp);

public:
    // clang-format off
    virtual int getNumChild() { return 0; }
    virtual Node* getChild(int idx) { return nullptr; }
    virtual bool beginIter(NodeIterator& buf) { return false; }
    virtual bool addChild(Node* child) { assert(0); return false; }
    virtual bool removeChild(Node* child) { return false; }
    // clang-format on
};



//////////////////////////////////////////////////////////////////////////
class NodesChildList : public INodeChildList
{
    qd::vector<Node*> m_ChildNodes;

public:
    virtual int getNumChild() override { return static_cast<int>(m_ChildNodes.size()); }

    virtual Node* getChild(int idx) override;

    virtual bool beginIter(NodeIterator& buf) override;
    virtual bool addChild(Node* child) override;
    virtual bool removeChild(Node* child) override;
    virtual ~NodesChildList();

private:
    TS_REFLECT_CLASS(qd::NodesChildList, qd::INodeChildList);

}; // class NodesChildList




}; // namespace qd
