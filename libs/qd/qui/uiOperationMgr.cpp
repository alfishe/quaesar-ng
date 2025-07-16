#include "uiOperationMgr.h"
#include "qd/log/log.h"
#include "qd/typeSystem/attributesCommon.h"
#include "qd/typeSystem/typeRegistry.h"
#include <qd/base/classInfoReg.h>



namespace qd {



UiOperationMgr::UiOperationMgr()
{
//     if (!g_pInstance)
//         g_pInstance = this;
}


qd::UiOperationMgr* UiOperationMgr::get()
{
    static ref_ptr<UiOperationMgr> pInst(new UiOperationMgr());
    return pInst.get();
}


//////////////////////////////////////////////////////////////////////////


void UiOperationMgr::createOperations(qd::UiOperationCreator* ca)
{
    assert(!mInit);

    // create all operations
    qd::TypeInfoSpan classTypes = qd::TypeRegistry::get()->findAllDerivedFromTypesCached_<qd::UiOperation>(false);
    for (const qd::TypeInfo* curOperationType : classTypes)
    {
        auto* pCreator = curOperationType->getAttribute_<qd::tsAttr::CreateClassCb>();
        if (!pCreator)
        {
            SDL_Log("Creator not defined in class:'%s'", curOperationType->getFullName().c_str());
            continue;
        }
        ca->uiOpsMgr = this;
        qd::UiOperation* pNewInstance = pCreator->makeInstance_<qd::UiOperation>(ca);
        assert(pNewInstance);
        addOperation(pNewInstance);
    }

    mInit = true;
}


void UiOperationMgr::destroy()
{
    mInit = false;
    while (!m_pOperations.empty())
    {
        delete m_pOperations.back();
        m_pOperations.pop_back();
    }
}



void UiOperationMgr::sendOperationMsg(const qd::TypeInfo& msg_type, qd::operation::msg::Base& msg) const
{
    auto operationsIter = m_operationsByMsgTypeMap.find(&msg_type);
    if (operationsIter == m_operationsByMsgTypeMap.end())
        return;
    const eastl::vector<UiOperation*>& actList = operationsIter->second;
    for (UiOperation* pCurAct : actList)
    {
        pCurAct->applyOperationMsgProc(&msg);
    }
}


EFlow UiOperationMgr::applyOperationMsg(qd::operation::msg::Base* p_msg) const
{
    for (UiOperation* pCurOperation : m_pOperations)
    {
        if (!pCurOperation)
            continue;
        EFlow r = pCurOperation->applyOperationMsgProc(p_msg);
        if (r != EFlow::NO_RESULT)
            return r;
    }
    return EFlow::NO_RESULT;
}


void UiOperationMgr::addOperation(UiOperation* pNewOperation)
{
    m_pOperations.push_back(pNewOperation);
    const qd::TypeInfo* curOperationType = &pNewOperation->getTypeInfo();
    m_operationByOperationTypeMap[curOperationType] = pNewOperation;

    OperationSupportedMsgVisitor visitor;
    pNewOperation->applyOperationMsgProc(&visitor);

    for(const qd::TypeInfo* pCurType : visitor.m_pSupportedMtd)
    {
        auto& actList = m_operationsByMsgTypeMap[pCurType];
        actList.push_back(pNewOperation);
    }
}


qd::UiOperation* UiOperationMgr::findOperation(uint32_t class_id) const
{
    for (UiOperation* pCurOperation : m_pOperations)
    {
        if (!pCurOperation || pCurOperation->mClassId != class_id)
            continue;
        return pCurOperation;
    }
    return nullptr;
}




qd::UiOperation* UiOperationMgr::findOperationByType(const qd::TypeInfo& type) const
{
    auto it = m_operationByOperationTypeMap.find(&type);
    if (it == m_operationByOperationTypeMap.end())
        return nullptr;
    return it->second;
}


eastl::span<UiOperation* const> UiOperationMgr::getOperationsList() const
{
    return eastl::span<UiOperation* const>(reinterpret_cast<UiOperation* const*>(m_pOperations.data()),
        m_pOperations.size());
}


bool OperationSupportedMsgVisitor::tryCast(const qd::TypeInfo& msg_type)
{
    msg_type.checkDefined();
    m_pSupportedMtd.push_back(&msg_type);

    return false;
}


}; // namespace qd
