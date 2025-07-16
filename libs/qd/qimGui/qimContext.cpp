#include "qimContext.h"
#include "qimStorage.h"
#include "qd/typeSystem/typeRegistry.h"
#include "qimElement.h"
#include "imgui/imgui.h"
#include "qd/typeSystem/attributesCommon.h"
#include "SDL_log.h"
#include "qimGui.h"
#include <imgui/imgui_internal.h>


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


qim::ElemData* Context::getOrCreateElement(const char* name_id, const qd::TypeInfo& behClass,
    const qd::TypeInfo& elemClass)
{
    ImGuiID id = ImGui::GetID(name_id);
    if (ElemData* pExist = m_pCurrStorage->findDataById(id))
        return pExist;

    BehaviorElem* pBeh = findBehavior(behClass);
    ASSERT_F(pBeh, "Behavior class not found for type '%s', name:'%s'", behClass.getFullName().c_str(), name_id);
    if (!pBeh)
        return nullptr;

    Behavior* pNewElem = pBeh->createElementData(elemClass);
    if (!pNewElem)
        return nullptr;

    ElemData* pNewData = new ElemData();
    pNewData->m_pElement = pNewElem;
    pNewData->m_elemId = id;
    m_pElemDataMap[pNewElem] = pNewData;

    m_pCurrStorage->setData(id, pNewData);
    pNewElem->onAttach(pBeh);
    return pNewData;
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


Context::StackItem& Context::pushStackElement(Behavior* pElem)
{
    assert(pElem);
    assert(this);
    StackItem& it = m_pChildStack.push_back();
    it.m_pElement = pElem;
    it.m_pElemData = findElementData(pElem);
    it.m_curVisitStage = EVisitStage::VCollect | EVisitStage::VProperty | EVisitStage::VEventHandler; // INITAL FLAGS
    return it;
}


void Context::popStackElement(Behavior* pElem)
{
    Behavior* pBack = m_pChildStack.back().m_pElement;
    assert(pBack == pElem);
    m_pChildStack.pop_back();
}


qim::ElemData* Context::getStackTreeTopElemData() const
{
    ElemData* pData = m_pChildStack.back().m_pElemData;
    return pData;
}


qim::ElemData* Context::findElementData(const Behavior* pElem) const
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
    ElemData* pData = stack.m_pElemData;
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
    while (!m_pChildStack.empty())
    {
        assert(0);
        m_pChildStack.pop_back();
    }

    //for (auto& iter : m_pElemDataMap)
    {
//         ElemData* pCurData = iter.second;
//         delete pCurData;
//         iter.second = nullptr;
    }
}


void Context::pushLoop(ref_ptr<BaseLoop> pLoop)
{
    m_pLoopStack.push_back(pLoop);
    m_pCurLoop = pLoop;
}


void Context::popLoop(BaseLoop* pLoop)
{
    if (!pLoop || m_pLoopStack.back() == pLoop)
        m_pLoopStack.pop_back();
    else
        assert(0);
    //return std::move(pLoop);
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


qim::ItNodeState& BaseIter::onNextNodeByStr(const char* str, const char* str_end /*= nullptr*/)
{
    if (m_stack.empty())
    {
        ItNodeState& st = pushCurNode(str, str_end);
        st.isRoot = true;
        return st;
    }

    ImGuiID seed = m_parentId; // m_nodeMap.back().m_nodeId;
    ImGuiID id = ::ImHashStr(str, str_end ? (str_end - str) : 0, seed);

    ItNodeState& st = m_nodeMap[id];
    st.m_nodeId = id;
    st.m_parentId = m_parentId;
    m_curId = id;
    m_pCurState = &st;
    return st;
}


qim::ItNodeState& BaseIter::pushCurNode(const char* str, const char* str_end /*= nullptr*/)
{
    StackItem& it = m_stack.push_back();
    it.m_curNodeId = m_curId;
    it.m_parentId = m_parentId;

    return onNextNodeByStr(str, str_end);
}


qim::ELoopState BaseLoop::getNextLoopIterState(ItNodeState& iterSt) const
{
    assert(m_pIter);

    BaseIter* pIter = m_pIter;
    const BaseIter::Cfg& cfg = pIter->cfg;

    ItNodeState& st = iterSt;
    if (!st.visitedHead)
    {
        if (cfg.visitRootHead || !st.isRoot)
        {
            st.visitedHead = true;
            return ELoopState::WANT_NODE_HEAD;
        }
    }

    if (cfg.enterInBodyOnce && !pIter->m_rootState.enterInBodyOnce)
    {
        pIter->m_rootState.enterInBodyOnce = true;
        return ELoopState::WANT_ITER_BODY_ONCE;
    }

    if (cfg.visitChild)
    {
        if (!st.iterChildBegin)
        {
            st.iterChildBegin = true;
            return ELoopState::WANT_MEET_CHILD;
        }
        if (!st.iterChildEnd /*&& st.nodeEnd*/)
        {
            st.iterChildEnd = true;
            //st.nodeEnd = false;
            return ELoopState::WANT_MEET_CHILD_END;
        }
    }
    if (cfg.visitSiblings)
    {
        if (!st.siblingsBegin && !st.nodeEnd)
        {
            st.siblingsBegin = true;
            return ELoopState::WANT_MEET_SIBLING;
        }
        if (!st.siblingsEnd /*&& st.nodeEnd*/)
        {
            st.siblingsEnd = true;
            //st.nodeEnd = false;
            return ELoopState::WANT_MEET_SIBLING_END;
        }
    }

    if (st.nodeEnd)
    {
        return ELoopState::S_END;
    }

    assert(0);
    return m_meetIter;
}


}; // namespace qim
