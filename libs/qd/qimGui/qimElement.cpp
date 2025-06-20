#include "qimElement.h"
#include "imgui/imgui.h"


namespace qim
{


qd::EFlow Element::notifyComps(qim::msg::Base& in_msg)
{
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


qim::Property* Element::propFindByCid(uint32_t cid, bool include_parents) const
{
    if (Property* pProp = propFindLocalByCid(cid))
        return pProp;

    if (m_pTemplate)
        if (Property* pProp = m_pTemplate->propFindLocalByCid(cid))
            return pProp;

    if (!include_parents || !m_pParentElem)
        return nullptr;

    return m_pParentElem->propFindByCid(cid, true);
}


qim::Property* Element::propFindLocalByCid(uint32_t cid) const
{
    auto it = m_pProperties.find(cid);
    if (it == m_pProperties.end())
        return nullptr;
    return it->second;
}


void Element::propAdd(ref_ptr<Property> pProp)
{
    uint32_t cid = pProp->getCID();
    m_pProperties[cid] = pProp;
}


bool CtrlElement::isHovered()
{
    return ImGui::IsItemHovered();
}


bool CtrlElement::isClicked(int mb)
{
    return ImGui::IsMouseClicked(mb);
}

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


}; // namespace qim

