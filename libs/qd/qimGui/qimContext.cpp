#include "qimContext.h"
#include "qimStorage.h"
#include "qd/typeSystem/typeRegistry.h"
#include "qimElement.h"
#include "imgui/imgui.h"
#include "qd/typeSystem/attributesCommon.h"
#include "SDL_log.h"


namespace qim
{
	

Context::~Context()
{
    SAFE_DELETE(m_pCurrStorage);
    SAFE_DELETE(m_pPrevStorage);

    while (!m_pBehaviors.empty())
    {
        BehaviorElem* pBeh = m_pBehaviors.back().second;
        delete pBeh;
        m_pBehaviors.pop_back();
    }
}


bool Context::getElementData(const char* name_id, qim::Element** pOutElem, const qd::TypeInfo& behClass,
    const qd::TypeInfo& elemClass) const
{
    ImGuiID id = ImGui::GetID(name_id);
    if (Element* pExist = m_pCurrStorage->findData(id))
    {
        *pOutElem = pExist;
        return true;
    }

    *pOutElem = nullptr;
    BehaviorElem* pBeh = findBehavior(behClass);
    ASSERT_F(pBeh, "Behavior class not found for type '%s', name:'%s'", behClass.getFullName().c_str(), name_id);
    if (!pBeh)
        return true;
    Element* pBaseCtrl = pBeh->createElementData(elemClass);
    if (!pBaseCtrl)
        return true;

    m_pCurrStorage->setData(id, pBaseCtrl);
    pBaseCtrl->onAttach(pBeh);
    *pOutElem = pBaseCtrl;
    return false;
}


qim::BehaviorElem* Context::findBehavior(const qd::TypeInfo& pBehClassInfo) const
{
    auto it = m_pBehaviors.find(&pBehClassInfo);
    if (it != m_pBehaviors.end())
        return it->second;
    return nullptr;
}


void Context::addBehavior(const qd::TypeInfo& pBehClassInfo, BehaviorElem* pInst)
{
    m_pBehaviors[&pBehClassInfo] = pInst;
}


void Context::init()
{
    auto behClassList = qd::TypeRegistry::get()->findAllDerivedFromTypes(qd::typeof_<qim::BehaviorElem>());

    for (const qd::TypeInfo* pCurBehClass : behClassList)
    {
        auto* pCreator = pCurBehClass->getAttribute_<qd::tsAttr::CreateClassCb>();
        if (!pCreator)
        {
            SDL_Log("Creator not defined in class:'%s'", pCurBehClass->getFullName().c_str());
            continue;
        }
        qim::ElemBehCreator cv;
        qim::BehaviorElem* pNewInstance = pCreator->makeInstance_<qim::BehaviorElem>(&cv);
        assert(pNewInstance);

        addBehavior(*pCurBehClass, pNewInstance);
    }

    // addBehavior(qd::typeof_<qim::UiMenuBeh>(), new qim::UiMenuBeh());
}

	
void Context::done()
{
    m_pCurrStorage->clear();
    m_pPrevStorage->clear();
    for (auto& it : m_pBehaviors)
    {
        delete it.second;
    }
    m_pBehaviors.clear();
}


Context::Context()
{
    m_pCurrStorage = new Storage();
    m_pPrevStorage = new Storage();
}


}; // namespace qim
