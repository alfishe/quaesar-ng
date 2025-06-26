#include "qimElement.h"
#include "imgui/imgui.h"
#include "qimMessages.h"
#include "qimContext.h"


namespace qim
{


qd::EFlow Element::notifyComps(qim::msg::Base& in_msg)
{
    this->onMessageProc(in_msg); // notify owner element

    Element* pCurComp = m_pCompsRoot;
    while (pCurComp)
    {
        qd::EFlow flow = pCurComp->onMessageProc(in_msg);
        if (flow != qd::EFlow::CONTINUE)
            return flow;
        pCurComp = pCurComp->m_pNextElem;
    }
    return qd::EFlow::CONTINUE;
}


qim::Property* ElementData::propFindByCid(const Context* ctx, uint32_t cid, bool include_parents) const
{
    if (Property* pProp = propFindLocalByCid(cid))
        return pProp;

//     if (m_pOwner->m_pTemplate)
//         if (Property* pProp = m_pOwner->m_pTemplate->propFindLocalByCid(cid))
//             return pProp;

    if (!include_parents || !m_pOwner->m_pParentElem)
        return nullptr;

    if (ElementData* pParentData = ctx->findElementData(m_pOwner->m_pParentElem))
        return pParentData->propFindByCid(ctx, cid, true);

    return nullptr;
}


qim::Property* ElementData::propFindLocalByCid(uint32_t cid) const
{
    auto it = m_pProperties.find(cid);
    if (it == m_pProperties.end())
        return nullptr;
    return it->second;
}


void ElementData::propAdd(Property* pProp)
{
    uint32_t cid = pProp->getCID();
    pProp->_nStrongRefs ++;
    m_pProperties[cid] = pProp;
}


bool CtrlElement::isHovered()
{
    return ImGui::IsItemHovered();
}


qd::EFlow CtrlElement::onMessageProcImp(qim::msg::Base& in_msg)
{
    switch (in_msg.id)
    {
    case msg::OnElemClicked::ID:
    {
        m_eventHappens.onClick = true;
        break;
    }
    default:
        break;
    }
    return qd::EFlow::CONTINUE;
}


// bool CtrlElement::isClicked(int mb)
// {
//     return ImGui::IsMouseClicked(mb);
// }

bool CtrlElement::isVisible(bool bCheckParents /*= false*/) const
{
    if (!m_bVisible)
        return false;
    if (!bCheckParents || !getParent_<CtrlElement>())
        return true;
    return getParent_<CtrlElement>()->isVisible(true);
}


bool CtrlElement::setVisible(bool bVisible)
{
    if (m_bVisible == bVisible)
        return bVisible;
    m_bVisible = bVisible;
    return bVisible;
}


void CtrlElement::onBegin(qim::Context* ctx)
{
    m_inPropsSection = qd::Tribool::True;
    m_inChildSection = qd::Tribool::Undef;
    m_eventHappens.flags = 0;
    m_eventRequest.flags = 0;
    onBeginImp(ctx);
}


qim::Element* BehaviorElem::createElementData(const qd::TypeInfo& type)
{
    auto* pCreator = type.getAttribute_<qd::tsAttr::CreateClassCb>();
    if (!pCreator)
    {
        qdlog("Creator not defined in class:'%s'", type.getFullName().c_str());
        return nullptr;
    }
    qim::OnElementConstruct cv;
    qim::Element* pNewInstance = pCreator->makeInstance_<qim::Element>(&cv);
    return pNewInstance;
}


}; // namespace qim

