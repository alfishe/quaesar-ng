#include <EASTL/fixed_function.h>
#include <qdIce/qdCore/nodeBase.h>


namespace qd {

struct NodeMsgProcVisitor {
    void reg(uint32_t msg_id, eastl::fixed_function<8, void(Node*, NodeMessage*)>) {}
};


// void Node::nodeProcVisit(NodeMsgProcVisitor* vis) {
//     vis->reg(1, [](Node* p_inst, NodeMessage* in_msg) {
//
//     });
// }


EFlow Node::onNodeMessageProc(NodeMessage* in_msg)
{
    return EFlow::NO_RESULT;
}



Node* Node::findComp(const qd::TypeId& id) const
{
    auto it = eastl::find_if(m_pComps.begin(), m_pComps.end(),
        [id](const Node* pCurComp) { return pCurComp ? pCurComp->getTypeId() == id : false; });
    if (it != m_pComps.end())
        return *it;
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


}; // namespace qd
