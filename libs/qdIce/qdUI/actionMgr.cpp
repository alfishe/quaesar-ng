#include "actionMgr.h"
#include "qdIce/qdLog/log.h"
#include "qdIce/qdTypeSystem/attributesCommon.h"
#include "qdIce/qdTypeSystem/typeRegistry.h"
#include <qdIce/qdBase/classInfoReg.h>



namespace qd {

class ActionMgrActionsListImp : public qd::INodesChildList
{
    TS_REFLECT_CLASS(qd::ActionMgrActionsListImp, qd::INodesChildList);
    UiActionMgr* m_pMgr;

public:
    ActionMgrActionsListImp(UiActionMgr* p_mgr)
        : m_pMgr(p_mgr)
    {}
    virtual ~ActionMgrActionsListImp() = default;

public:
    virtual int getNumChild() override { return (int)m_pMgr->m_pActions.size(); }
    virtual Node* getChild(int idx) override { return m_pMgr->m_pActions[idx]; }
    virtual bool addChild(Node* child) override { return false; }
    virtual bool removeChild(Node* child) override { return false; };
    virtual bool beginIter(NodeIterator& buf) override { return false; };

}; // class NodesChildList


void UiActionMgr::onNodeCreated(NodeCreator* mk)
{
    createComp_<ActionMgrActionsListImp>(this);
    TSuper::onNodeCreated(mk);
}


void UiActionMgr::createActions(qd::UiActionCreator* ca)
{
    assert(!mInit);

    // create all actions
    qd::TypeInfoSpan classTypes = qd::TypeRegistry::get()->findAllDerivedFromTypesCached_<qd::UiAction>(false);
    for (const qd::TypeInfo* curActionType : classTypes)
    {
        auto* pCreator = curActionType->getAttribute_<qd::CreateClassCbAttr>();
        if (!pCreator)
        {
            SDL_Log("Creator not defined in class:'%s'", curActionType->getFullName().c_str());
            continue;
        }
        qd::UiActionCreator cv;
        cv.parent = this;
        qd::UiAction* pNewAction = pCreator->makeInstance_<qd::UiAction>(&cv);
        assert(pNewAction);
        addAction(pNewAction);
    }

    mInit = true;
}


void UiActionMgr::destroy()
{
    mInit = false;
    while (!m_pActions.empty())
    {
        delete m_pActions.back();
        m_pActions.pop_back();
    }
}


UiActionMgr::ListByMtd UiActionMgr::getFilteredActionsByMtd(int id)
{
    m_pFilteredActions.clear();
    for (UiAction* curAction : m_pActions)
    {
        if (!curAction->hasMtd(id))
            continue;
        m_pFilteredActions.push_back(curAction);
    }
    ListByMtd r;
    r.mpMgr = this;
    return r;
}


void UiActionMgr::sendActionMsg(const qd::TypeInfo& msg_type, qd::action::msg::Base& msg) const
{
    auto actionsIter = m_actionsByMsgTypeMap.find(&msg_type);
    if (actionsIter == m_actionsByMsgTypeMap.end())
        return;
    const eastl::vector<UiAction*>& actList = actionsIter->second;
    for (UiAction* pCurAct : actList)
    {
        pCurAct->applyActionMsgProc(&msg);
    }
}


EFlow UiActionMgr::applyActionMsg(qd::action::msg::Base* p_msg) const
{
    for (UiAction* pCurAction : m_pActions)
    {
        if (!pCurAction)
            continue;
        EFlow r = pCurAction->applyActionMsgProc(p_msg);
        if (r != EFlow::NO_RESULT)
            return r;
    }
    return EFlow::NO_RESULT;
}


void UiActionMgr::addAction(UiAction* pNewAction)
{
    m_pActions.push_back(pNewAction);
    const qd::TypeInfo* curActionType = &pNewAction->getTypeInfo();
    m_actionByActionTypeMap[curActionType] = pNewAction;

    ActionSupportedMsgVisitor visitor;
    pNewAction->applyActionMsgProc(&visitor);

    for(const qd::TypeInfo* pCurType : visitor.m_pSupportedMtd)
    {
        auto& actList = m_actionsByMsgTypeMap[pCurType];
        actList.push_back(pNewAction);
    }
}


qd::UiAction* UiActionMgr::findAction(uint32_t class_id) const
{
    for (UiAction* pCurAction : m_pActions)
    {
        if (!pCurAction || pCurAction->mClassId != class_id)
            continue;
        return pCurAction;
    }
    return nullptr;
}


qd::UiAction* UiActionMgr::findOperationByType(const qd::TypeInfo& type) const
{
    auto it = m_actionByActionTypeMap.find(&type);
    if (it == m_actionByActionTypeMap.end())
        return nullptr;
    return it->second;
}


bool ActionSupportedMsgVisitor::tryCast(const qd::TypeInfo& msg_type)
{
    msg_type.checkDefined();
    m_pSupportedMtd.push_back(&msg_type);

    return false;
}


}; // namespace qd
