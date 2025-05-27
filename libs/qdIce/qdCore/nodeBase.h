#pragma once
#include <EASTL/fixed_vector.h>
#include <EASTL/vector.h>
#include <qdIce/qdBase/ref_ptr.h>
#include <qdIce/qdBase/types.h>
#include <qdIce/qdTypeSystem/ReflectedType.h>
#include <qdIce/qdTypeSystem/TypeInfo.h>


namespace qd {
class Node;
struct NodeMsgProcVisitor;
class CompBase;
class TypeInfo;


struct NodeMessage {
    int id = 0;

    NodeMessage(int msg_id = 0)
        : id(msg_id)
    {}
}; // struct


struct NodeMaker {
    Node* parent = nullptr;
};



class INodeChildList
{
public:
    // clang-format off
    virtual int getNumChild() { return 0; }
    virtual Node* getChild(int idx) { return nullptr; }
    // clang-format on
};



//////////////////////////////////////////////////////////////////////////
class Node
    : public qd::RefCounted
    , public qd::IReflectedType
{
    QD_REFLECT_TYPE(Node);

protected:
    Node* m_pParent = nullptr;
    INodeChildList* m_pChildList = nullptr;
    eastl::fixed_vector<Node*, 6> m_pComps;

public:
    Node() = default;
    virtual ~Node();

    virtual void onNodeCreated(NodeMaker* mk) {}

    virtual EFlow onNodeMessageProc(qd::NodeMessage* in_msg);


    int getNumChild() const { return m_pChildList ? m_pChildList->getNumChild() : 0; }

    Node* getChild(int idx) const { return m_pChildList->getChild(idx); }

    decltype(auto) getCompsBegin() { return m_pComps.begin(); }
    decltype(auto) getCompsEnd() { return m_pComps.end(); }

    template<class TComp, typename... TArgs>
    TComp* createComp_(TArgs&&... args)
    {
        if (TComp* pExist = getComp_<TComp>())
            return pExist;
        TComp* newComp = new TComp(args...);
        NodeMaker cp;
        newComp->onNodeCreated(&cp);
        m_pComps.push_back(newComp);
        return newComp;
    }

    Node* findComp(const qd::TypeId& id) const;

    template<class TComp>
    inline TComp* getComp_() const
    {
        Node* pComp = findComp(TComp::getStaticTypeId());
        return static_cast<TComp*>(pComp);
    }

    template<class T>
    T* findParentMixin_() const
    {
        if (!m_pParent)
            return nullptr;
        T::s_pTypeInfo;
        return nullptr;
    }

    template<class T>
    T* findParentNode_() const
    {
        if (!m_pParent)
            return nullptr;
        T::s_pTypeInfo;
        return nullptr;
    }


}; // class Node
//////////////////////////////////////////////////////////////////////////



class CompBase : public Node
{
public:
    CompBase() {}
};



}; // namespace qd
