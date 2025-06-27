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


qim::Element* Context::getOrCreateElement(const char* name_id, const qd::TypeInfo& behClass,
    const qd::TypeInfo& elemClass)
{
    ImGuiID id = ImGui::GetID(name_id);
    if (Element* pExist = m_pCurrStorage->findData(id))
        return pExist;

    BehaviorElem* pBeh = findBehavior(behClass);
    ASSERT_F(pBeh, "Behavior class not found for type '%s', name:'%s'", behClass.getFullName().c_str(), name_id);
    if (!pBeh)
        return nullptr;
    Element* pNewElem = pBeh->createElementData(elemClass);
    if (!pNewElem)
        return nullptr;

    ElementData* pNewData = new ElementData();
    pNewData->m_pElement = pNewElem;
    pNewData->m_elemId = id;
    m_pElemDataMap[pNewElem] = pNewData;

    m_pCurrStorage->setData(id, pNewElem);
    pNewElem->onAttach(pBeh);
    return pNewElem;
}


bool Context::checkSectStage(EVisitStage suppStages, size_t& nFor)
{
    for (EVisitStage st : {EVisitStage::VProperty, EVisitStage::VChild})
    {
        if (suppStages.has(st) /*&& curStage.has(st)*/)
            return true;
    }
    return false;
}


void Context::endSect(Section* pOutSect)
{
}


Context::StackItem& Context::stackPushElement(Element* pElem)
{
    assert(pElem);
    StackItem& it = m_pChildStack.push_back();
    it.m_pElement = pElem;
    it.m_pElemData = findElementData(pElem);
    it.m_curVisitStage = EVisitStage::VCollect | EVisitStage::VProperty | EVisitStage::VEventHandler; // INITAL FLAGS
    return it;
}


qim::ElementData* Context::getStackTreeTopElemData() const
{
    ElementData* pData = m_pChildStack.back().m_pElemData;
    return pData;
}


qim::ElementData* Context::findElementData(const Element* pElem) const
{
    if (!pElem)
        return nullptr;
    auto it = m_pElemDataMap.find(pElem);
    if (it == m_pElemDataMap.end())
        return nullptr;
    return it->second;
}


qd::EFlow Context::onCtrlVisitLoopEnd(CtrlElement* pElem, EVisitStage* pOutVisit) const
{
    assert(getStackTreeTopElem() == pElem);

    const StackItem& stack = m_pChildStack.back();
    EVisitStage st = stack.m_curVisitStage;
    ElementData* pData = stack.m_pElemData;
    if (st == EVisitStage::UNDEF || st.hasAny(EVisitStage::VCollect))
    {
        if (pData->m_supportedVStages.has(EVisitStage::VChild))
        {
            *pOutVisit = EVisitStage::VChild;
            return qd::EFlow::REPEAT;
        }
        st = EVisitStage::VEventHandler;
    }
    if (st.hasAny(EVisitStage::VChild | EVisitStage::VEventHandler))
    {
        if (pData->hasQueuedEvents())
        {
            *pOutVisit = EVisitStage::VEventHandler;
            return qd::EFlow::REPEAT;
        }
    }
    *pOutVisit = EVisitStage::VDone;
    return qd::EFlow::DONE;
}


void Context::endFrame()
{
    if (!m_pChildStack.empty())
    {
        assert(0);
    }

    for (auto& iter : m_pElemDataMap)
    {
//         ElementData* pCurData = iter.second;
//         delete pCurData;
//         iter.second = nullptr;
    }
}


void Context::stackPopChild(Element* pElem)
{
    Element* pBack = m_pChildStack.back().m_pElement;
    assert(pBack == pElem);
    m_pChildStack.pop_back();
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
