#include "qimElement.h"
#include "imgui/imgui.h"
#include "qimContext.h"
#include "qimMessages.h"


namespace qim {


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


qim::Property* ElementData::propFindByCid(const Context* ctx, const qim::PropertyClassMeta& cid, bool include_parents) const
{
    if (Property* pProp = propFindLocalByCid(cid))
        return pProp;

    //     if (m_pElement->m_pTemplate)
    //         if (Property* pProp = m_pElement->m_pTemplate->propFindLocalByCid(cid))
    //             return pProp;

    if (!include_parents || !m_pElement->m_pParentElem)
        return nullptr;

    if (ElementData* pParentData = ctx->findElementData(m_pElement->m_pParentElem))
        return pParentData->propFindByCid(ctx, cid, true);

    return nullptr;
}


qim::Property* ElementData::propFindLocalByCid(const qim::PropertyClassMeta& pid) const
{
    if (!m_propPrimeHash.isDerivedFrom(pid.primeId))
        return nullptr;
    auto it = m_pProperties.find(pid.primeId);
    if (it == m_pProperties.end())
        return nullptr;
    return it->second;
}


void ElementData::propAdd(Property* pProp)
{
    const qim::PropertyClassMeta& cid = pProp->getClassMeta();
    assert(propFindLocalByCid(cid) == nullptr);

    pProp->_nStrongRefs++;
    m_pProperties[cid.primeId] = pProp;
    m_propPrimeHash.addBaseClass(cid.primeId);
}


bool CtrlElement::isHovered()
{
    return ImGui::IsItemHovered();
}


bool ElementData::hasQueuedEvents()
{
    if ((m_eventHappens == m_eventApplied))
        return false;
    return true;
}


qd::EFlow CtrlElement::onMessageProcImp(qim::msg::Base& in_msg)
{
    switch (in_msg.id)
    {
    case msg::OnElemClicked::ID:
    {
        auto p = in_msg.cast<msg::OnElemClicked>();
        ElementData* pData = g_pCtx->findElementData(this);
        pData->m_eventHappens.addBaseClass(qim::Sect::IsClicked::s_classMeta.primeId);

        auto pClickSect = p->m_pElem->propFind_<qim::Sect::IsClicked>(false);
        if (pClickSect)
            pClickSect->onClick(p->m_mouseButton);
        return qd::EFlow::DONE;
    }
    break;
    default:
        break;
    };
    return qd::EFlow::CONTINUE;
}


// bool CtrlElement::isClicked(int mb)
// {
//        break;
// return ImGui::IsMouseClicked(mb);
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


void CtrlElement::onDrawBegin(qim::Context* ctx)
{
    m_inPropsSection = qd::Tribool::True;
    m_inChildSection = qd::Tribool::Undef;
    onDrawBeginImp(ctx);
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
