#include "uiNode.h"
#include "qd/debug/exception.h"
#include "qd/mem/ptrMath.h"
#include "qd/qui/uiMessages.h"


namespace qd {


void UiNode::onNodeCreated(UiNodeCreator* mk)
{
    m_pParent = mk->parent;
    if (m_pParent)
        assert(m_pParent->getTypeInfo().isDerivedFrom_<qd::UiNode>());

    if (mk->id)
        setId(mk->id);
}


void UiNode::setParent(qd::UiNode* pParent)
{
    assert(!pParent || pParent->getTypeInfo().isDerivedFrom_<qd::UiNode>());
    m_pParent = pParent;
}


UiNode* UiNode::findChildById(uint32_t id) const
{
    if (!isPtrValid(this))
        return nullptr;
    for (const ChildItem& item : m_pChilds)
    {
        if (item.id == id)
            return item.get();
    }
    return nullptr;
}


qd::UiNode* UiNode::findChildByType(const qd::TypeInfo& ti) const
{
    if (!isPtrValid(this))
        return nullptr;
    for (const ChildItem& item : m_pChilds)
    {
        if (ti.isDerivedFrom(*item.typeInfo))
            return item.get();
    }
    return nullptr;
}


int UiNode::getNumChild() const
{
    return (int)m_pChilds.size();
}


UiNode* UiNode::getChildById(uint32_t ID) const
{
    if (!isPtrValid(this))
        return nullptr;
    UiNode* pChild = findChildById(ID);
    if (!pChild)
        G_THROW_OR_DO(
            Exception(EException::NOT_FOUND, "UiNode::getChildById(...) : FAILED : Can't find child by id '%d'", ID),
            return nullptr);
    return pChild;
}


qd::UiNode* UiNode::getChildById(std::initializer_list<uint32_t> idList) const
{
    const UiNode* pChildCtrl = this;
    for (uint32_t curID : idList)
    {
        const UiNode* pCurCtrl = pChildCtrl;
        pChildCtrl = pCurCtrl->getChildById(curID);
    }
    return const_cast<UiNode*>(pChildCtrl);
}


int UiNode::getChildIndex(UiNode* pChild) const
{
    int nChild = findChildIndex(pChild);
    assert(nChild >= 0);
    if (nChild < 0)
        G_THROW_OR_DO(Exception("Can't find child with ID=%u", pChild->getId()), return -1);
    return nChild;
}


int UiNode::findChildIndex(UiNode* pChild) const
{
    int nInd = 0;
    size_t nChilds = m_pChilds.size();
    for (size_t i = 0; i < nChilds; ++i)
    {
        const ChildItem& pCtrl = m_pChilds[i];
        if (pCtrl.get() == pChild)
            return nInd;
        nInd++;
    }

    return -1;
}


qd::UiNode* UiNode::addChild(ref_ptr<UiNode> pChild)
{
    ASSERT_AND_DO(pChild.get(), return nullptr, "AddChild - Exception: Child is nullptr!");
    assert(pChild->getTypeInfo().isDerivedFrom_<qd::UiNode>());

    uint32_t id = pChild->getId();
    if (id == UiNode::UNDEF_ID)
    {
        id = qd::ptr2DW(pChild.get());
        setId(id);
    }

    if (findChildIndex(pChild) != -1)
    {
        qd::string err =
            qd::string_format("AddChild - Exception: Child already added to Parent Control! id=\"%i\" ", id);
        G_THROW_OR_DO(Exception(EException::INVALID_ARGUMENT, err), return nullptr);
    }

    if (findChildById(id))
    {
        // DUPLICATE id's FOUND
        qd::string err =
            qd::string_format("AddChild - Exception: Child already added to Parent Control! id=\"%i\" ", id);
        G_THROW_OR_DO(Exception(EException::INVALID_ARGUMENT, err), return nullptr);
    }

    // ADD CHILD
    m_pChilds.push_back({id, pChild, &pChild->getTypeInfo()});

    // NOTIFY COMPS
    uiMsg::OnChildAdded t;
    t.m_pCtrl = pChild;
    onUiNodeMessageProc(&t);
    // getComps()->invokeCompMtd(ECompMtd::ON_CHILD_ADDED, &p);

    return pChild;
}


void UiNode::removeChild(ref_ptr<UiNode> pChild)
{
    for (auto iter = m_pChilds.begin(); iter != m_pChilds.end(); ++iter)
    {
        if (iter->get() == pChild.get())
        {
            m_pChilds.erase(iter);

            //ECompMtdX::ON_CHILD_DELETED_t p(pChild);
            //getComps()->invokeCompMtd(ECompMtd::ON_CHILD_DELETED, &p);
            return;
        }
    }
}


void UiNode::setId(uint32_t newId)
{
    assert(newId != UiNode::UNDEF_ID);
    if (m_id == newId)
        return;

    UiNode* parent = getParent();
    if (parent)
    {
        assert(!parent->findChildById(newId) || parent->findChildById(newId) == this);
    }
    m_id = newId;
}


qd::EFlow UiNode::onUiNodeMessageProc(qd::UiMessage* /*in_msg*/)
{
    // if (auto p = in_msg->cast_<uiMsg::OnChildAdded>()) {}
    return qd::EFlow::SUCCESS;
}



void uinode_draw_child(UiNode* pNode)
{
    for (int i = 0; i < pNode->getNumChild(); ++i)
    {
        UiNode* pChild = pNode->getChild(i);
        if (pChild->isVisible())
        {
            pChild->draw();
        }
    }
}

void UiNode::drawContentImp()
{
    uinode_draw_child(this);
}


void UiNode::drawImp()
{
    drawContentImp();
}


void UiNode::draw()
{
    if (!isVisible())
        return;
    drawImp();
}


bool UiNode::isVisible(bool bCheckParents /*= false*/) const
{
    if (!m_bVisible)
        return false;
    if (!bCheckParents || !getParent())
        return true;
    return getParent()->isVisible(true);
}


bool UiNode::setVisible(bool bVisible)
{
    if (m_bVisible == bVisible)
        return bVisible;
    m_bVisible = bVisible;

    qd::uiMsg::OnVisibleChanged t;
    t.m_pCtrl = this;
    t.m_bVisible = bVisible;
    onUiNodeMessageProc(&t);

    if (!bVisible && m_bFocus)
        m_bFocus = false;

    return bVisible;
}


void UiNode::destroyRecursive()
{
    ref_ptr<UiNode> pThis(this); // PROTECT FOR SELF DESTROY

    while (!m_pChilds.empty())
    {
        ref_ptr<UiNode> pControl = m_pChilds.back().ptr;
        m_pChilds.pop_back();
        if (pControl)
        {
            pControl->destroy();
            pControl = nullptr;
        }
    }

    if (m_pParent)
    {
        ref_ptr<UiNode> pParent(m_pParent); // keep refs counter
        m_pParent->removeChild(pThis);
        m_pParent = nullptr;
    }
}


void UiNode::destroy()
{
    destroyRecursive();
}


void UiNode::addComp(qd::unique_ptr<qd::UiNodeComp> pNewComp)
{
    // const qd::TypeInfo& typeInfo = pNewComp->getTypeInfo();
    m_pComps.push_back(std::move(pNewComp));
}


UiNodeComp* UiNode::findComp(const qd::TypeInfo& comp) const
{
    auto it = eastl::find_if(m_pComps.begin(), m_pComps.end(), [comp](const qd::unique_ptr<qd::UiNodeComp>& pCurComp) {
        return pCurComp ? pCurComp->getTypeInfo().isDerivedFrom(comp) : false;
    });
    if (it != m_pComps.end())
        return (*it).get();
    return nullptr;
}


}; // namespace qd
