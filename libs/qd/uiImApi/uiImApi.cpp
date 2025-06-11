#include "qd/uiImApi/uiImApi.h"
#include "qd/qdTypeSystem/typeRegistry.h"


namespace qim {

qim::Context* qim::getContext()
{
    static Context ctx;
    return &ctx;
}



Context::~Context()
{
    SAFE_DELETE(m_pCurrStorage);
    SAFE_DELETE(m_pPrevStorage);

    while (!m_pBehaviors.empty())
    {
        ElementBeh* pBeh = m_pBehaviors.back().second;
        delete pBeh;
        m_pBehaviors.pop_back();
    }

}



qim::ElemantData* Context::getElementData(const char* name_id, const qd::TypeInfo& behClass,
    const qd::TypeInfo& elemClass) const
{
    ImGuiID id = ImGui::GetID(name_id);

    if (ElemantData* pExist = m_pCurrStorage->findData(id))
        return pExist;
    ElementBeh* pBeh = findBehavior(behClass);
    ASSERT_F(pBeh, "Behavior class not found for type '%s', name:'%s'", behClass.getFullName().c_str(), name_id);
    if (!pBeh)
        return nullptr;
    ElemantData* pBaseCtrl = pBeh->createElementData(elemClass, name_id);
    if (!pBaseCtrl)
        return nullptr;
    m_pCurrStorage->setData(id, pBaseCtrl);
    pBaseCtrl->onAttach(pBeh);
    return pBaseCtrl;
}


qim::ElementBeh* Context::findBehavior(const qd::TypeInfo& pBehClassInfo) const
{
    auto it = m_pBehaviors.find(&pBehClassInfo);
    if (it != m_pBehaviors.end())
        return it->second;
    return nullptr;
}


void Context::addBehavior(const qd::TypeInfo& pBehClassInfo, ElementBeh* pInst)
{
    m_pBehaviors[&pBehClassInfo] = pInst;
}




qim::ElemantData* BehMenu::createElementData(const qd::TypeInfo& type, const char* name)
{
    if (type == qd::typeof_<qim::MenuItem>())
    {
        auto p = new MenuItem();
        p->m_text = name;
        return p;
    }
    auto p = new Menu();
    p->m_text = name;
    return p;
}


//////////////////////////////////////////////////////////////////////////




void Context::init()
{
    addBehavior(qd::typeof_<qim::BehMenu>(), new qim::BehMenu());
}

void beginFrame() {}
void endFrame() {}




}; // namespace qim
