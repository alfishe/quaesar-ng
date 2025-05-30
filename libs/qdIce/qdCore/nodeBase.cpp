#include <EASTL/fixed_function.h>
#include <qdIce/qdCore/nodeBase.h>
#include <qdIce/qdTypeSystem/typeInfo.h>


namespace qd {

struct NodeMsgProcVisitor {
    void reg(uint32_t msg_id, eastl::fixed_function<8, void(Node*, NodeMessage*)>) {}
};



EFlow Node::onNodeMessageProc(NodeMessage* in_msg)
{
    return EFlow::NO_RESULT;
}



void Node::addComp(Node* pNewComp)
{
    if (pNewComp->getStaticTypeInfo()->isDerivedFrom_<qd::INodeChildList>())
        m_pChildList = static_cast<INodeChildList *>(pNewComp);
    m_pComps.push_back(pNewComp);
}


Node* Node::findComp(const THash32& id) const
{
    auto it = eastl::find_if(m_pComps.begin(), m_pComps.end(),
        [id](const Node* pCurComp) { return pCurComp ? pCurComp->getCID() == id : false; });
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
        m_pParent = mk->parent;
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


}; // namespace qd
