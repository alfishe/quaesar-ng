#include "qimElement.h"
#include "imgui/imgui.h"
#include "qimContext.h"
#include "qimMessages.h"


namespace qim {


qd::EFlow Behavior::notifyComps(qim::ElemData* pInst, qim::msg::Base& in_msg)
{
    this->onMessageProc(pInst, in_msg); // notify owner element

    ElemData* pCurComp = pInst->m_pCompsRoot;
    while (pCurComp)
    {
        qd::EFlow flow = pCurComp->onMessageProc(in_msg);
        if (flow != qd::EFlow::CONTINUE)
            return flow;
        pCurComp = pCurComp->m_pNextElem;
    }
    return qd::EFlow::CONTINUE;
}


qim::Property* ElemData::propFindByCid(const qim::PropertyClassMeta& cid, bool include_parents) const
{
    if (Property* pProp = propFindLocalByCid(cid))
        return pProp;

    //     if (m_pElement->m_pTemplate)
    //         if (Property* pProp = m_pElement->m_pTemplate->propFindLocalByCid(cid))
    //             return pProp;

    if (!include_parents || !this->m_pParentElem)
        return nullptr;

    if (ElemData* pParentData = this->m_pParentElem)
        return pParentData->propFindByCid(cid, true);

    return nullptr;
}


qim::Property* ElemData::propFindLocalByCid(const qim::PropertyClassMeta& pid) const
{
    if (!m_propPrimeHash.isDerivedFrom(pid.primeId))
        return nullptr;
    auto it = m_pProperties.find(pid.primeId);
    if (it == m_pProperties.end())
        return nullptr;
    return it->second;
}


void ElemData::propAdd(qim::Property* pProp)
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


bool ElemData::hasQueuedEvents()
{
    if ((m_eventHappens == m_eventApplied))
        return false;
    return true;
}


qd::EFlow ElemData::onMessageProc(qim::msg::Base& in_msg)
{
    return m_pElement->onMessageProc(this, in_msg);
}


qd::EFlow CtrlElement::onMessageProcImp(qim::ElemData* pInst, qim::msg::Base& in_msg)
{
    switch (in_msg.id)
    {
    case msg::OnElemClicked::ID:
    {
        auto p = in_msg.cast<msg::OnElemClicked>();
        ElemData* pData = g_pCtx->findElementData(this);
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


bool ElemData::isVisible(bool bCheckParents /*= false*/) const
{
    if (!m_bVisible)
        return false;
    if (!bCheckParents || !getParent())
        return true;
    return getParent()->isVisible(true);
}


bool ElemData::setVisible(bool bVisible)
{
    if (m_bVisible == bVisible)
        return bVisible;
    m_bVisible = bVisible;
    return bVisible;
}


void ElemData::drawElem(qim::ElemBrush& brush)
{
    CtrlElement* pBeh = getElem_<CtrlElement>();
    pBeh->drawElem(this, brush);
}


void CtrlElement::onDrawBegin(qim::Context* ctx)
{
    m_inPropsSection = qd::Tribool::True;
    m_inChildSection = qd::Tribool::Undef;
    onDrawBeginImp(ctx);
}


qim::Behavior* BehaviorElem::createElementData(const qd::TypeInfo& type)
{
    auto* pCreator = type.getAttribute_<qd::tsAttr::CreateClassCb>();
    if (!pCreator)
    {
        log_debug("Creator not defined in class:'%s'", type.getFullName().c_str());
        return nullptr;
    }
    qim::OnElementConstruct cv;
    qim::Behavior* pNewInstance = pCreator->makeInstance_<qim::Behavior>(&cv);
    return pNewInstance;
}


}; // namespace qim
