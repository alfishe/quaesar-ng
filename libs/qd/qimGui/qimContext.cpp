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


bool Context::getOrCreateElement(const char* name_id, qim::Element** pOutElem, const qd::TypeInfo& behClass,
    const qd::TypeInfo& elemClass)
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
    Element* pNewElem = pBeh->createElementData(elemClass);
    if (!pNewElem)
        return true;

    ElementData* pNewData = new ElementData(pNewElem);
    m_pElemDataMap[pNewElem] = pNewData;

    m_pCurrStorage->setData(id, pNewElem);
    pNewElem->onAttach(pBeh);
    *pOutElem = pNewElem;
    return false;
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
    it.m_visitStage = EVisitStage::VCollect | EVisitStage::VProperty;
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


bool Context::nextCtrlLoop(CtrlElement* pElem)
{
    assert(getStackTreeTopElem() == pElem);
    StackItem& stack = m_pChildStack.back();
    EVisitStage st = stack.m_visitStage;
    ElementData* pData = stack.m_pElemData;
    if (st == EVisitStage::UNDEF || st.hasAny(EVisitStage::VProperty))
    {
        if (pData->m_supportStages.has(EVisitStage::VChild))
        {
            setCurVisitStage(EVisitStage::VChild);
            return true;
        }
        st = EVisitStage::VEventHandler;
    }
    if (st.hasAny(EVisitStage::VChild | EVisitStage::VEventHandler))
    {
        if (pElem->pollLoopEvent())
        {
            setCurVisitStage(EVisitStage::VEventHandler);
            return true;
        }
    }
    setCurVisitStage(EVisitStage::VDone);
    return false;
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
