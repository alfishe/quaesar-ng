#include <EASTL/fixed_function.h>
#include <qd/Core/nodeBase.h>
#include <qd/TypeSystem/typeInfo.h>
#include "qd/STL/algorithm.h"


namespace qd {

struct NodeMsgProcVisitor {
    void reg(uint32_t msg_id, eastl::fixed_function<8, void(Node*, NodeMessage*)>) {}
};



EFlow Node::onNodeMessageProc(NodeMessage* in_msg)
{
    return EFlow::NO_RESULT;
}



void Node::addComp(NodeComp* pNewComp)
{
    const qd::TypeInfo& typeInfo = pNewComp->getTypeInfo();
    if (typeInfo.isDerivedFrom_<qd::INodesChildList>())
        m_pChildList = static_cast<INodesChildList *>(pNewComp);
    m_pComps.push_back(pNewComp);
}


NodeComp* Node::findComp(const qd::TypeInfo& comp) const
{
    auto it = eastl::find_if(m_pComps.begin(), m_pComps.end(),
        [comp](const NodeComp* pCurComp) { return pCurComp ? pCurComp->getTypeInfo().isDerivedFrom(comp) : false; });
    if (it != m_pComps.end())
        return *it;
    return nullptr;
}



qd::Node* Node::findChildNode(const qd::TypeInfo& ti)
{
    if (!m_pChildList)
        return nullptr;

    NodeIterator iter;
    for (m_pChildList->beginIter(iter); iter.hasNext(); /**/)
    {
        Node* pCurChild = iter.next();
        const qd::TypeInfo& curChildType = pCurChild->getTypeInfo();
        if (curChildType.isDerivedFrom(ti))
            return pCurChild;
    }
    return nullptr;
}


qd::Node* Node::findParentNode(const qd::TypeInfo& needType) const
{
    Node* pCurNode = m_pParent;
    while (pCurNode)
    {
        const qd::TypeInfo& curTypeInfo = pCurNode->getTypeInfo();
        if (curTypeInfo.isDerivedFrom(needType))
            return pCurNode;
        pCurNode = pCurNode->m_pParent;
    }
    return nullptr;
}


void Node::setParent(qd::Node* Parent)
{
    const_cast<qd::Node*&>(m_pParent) = Parent;
}


void Node::destroy()
{
    for (int i = 0; i < m_pComps.size(); ++i)
    {
        Node* pCurComp = m_pComps[i];
        if (!pCurComp)
            continue;
        pCurComp->destroy();
    }
}


Node::~Node()
{
    while (!m_pComps.empty())
    {
        Node* curComp = m_pComps.back();
        delete curComp;
        m_pComps.pop_back();
    }
}


void Node::onNodeCreated(NodeCreator* mk)
{
    if (!m_pParent && mk->parent)
        setParent(mk->parent);
}


 NodeIterator::~NodeIterator()
{
    if (m_pIter)
    {
        m_pIter->~INodeIterator();
        m_pIter = nullptr;
    }
}


bool NodesChildList::beginIter(NodeIterator& buf)
{
    struct VecIterator : public INodeIterator {
        qd::vector<Node*>::iterator m_Current;
        qd::vector<Node*>::iterator m_End;

        VecIterator(qd::vector<Node*>& children)
            : m_Current(children.begin())
            , m_End(children.end())
        {}

        virtual bool hasNext() const override { return m_Current != m_End; }

        virtual Node* next() override
        {
            if (hasNext())
                return *(m_Current++);
            return nullptr;
        }
    };

    buf.makeIter_<VecIterator>(m_ChildNodes);
    return true;
}


bool NodesChildList::addChild(Node* child)
{
    if (child && qd::find(m_ChildNodes.begin(), m_ChildNodes.end(), child) == m_ChildNodes.end())
    {
        m_ChildNodes.push_back(child);
        return true;
    }
    assert(0);
    return false;
}


bool NodesChildList::removeChild(Node* child)
{
    auto it = qd::find(m_ChildNodes.begin(), m_ChildNodes.end(), child);
    if (it == m_ChildNodes.end())
        return false;
    m_ChildNodes.erase(it);
    return true;
}


NodesChildList::~NodesChildList()
{
    while (!m_ChildNodes.empty())
    {
        Node* child = m_ChildNodes.back();
        m_ChildNodes.pop_back();
        delete child;
    }
}


int NodesChildList::getNumChild()
{
    return static_cast<int>(m_ChildNodes.size());
}


int Node::getNumChild() const
{
    return m_pChildList ? m_pChildList->getNumChild() : 0;
}


Node* Node::getChild(int idx) const
{
    return m_pChildList->getChild(idx);
}


qd::Node* NodesChildList::getChild(int idx)
{
    if (idx >= 0 && idx < static_cast<int>(m_ChildNodes.size()))
        return m_ChildNodes[idx];
    return nullptr;
}


bool NodeMessage::tryCast(const qd::TypeInfo& msg_type) const
{
    return msg_type.isDerivedFrom(getTypeInfo());
}


}; // namespace qd
