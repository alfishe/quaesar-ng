#include "uiOperationMgr.h"
#include "qd/log/log.h"
#include "qd/typeSystem/attributesCommon.h"
#include "qd/typeSystem/typeRegistry.h"
#include "qd/base/classInfoReg.h"
#include "qd/debug/assert.h"
#include "qd/qui/uiOperation.h"
#include "qd/qui/shortcutMgr.h"



namespace qd {



UiOperationMgr::UiOperationMgr()
{
//     if (!g_pInstance)
//         g_pInstance = this;
}


 UiOperationMgr::~UiOperationMgr()
{
    assert(!mInit);
}


qd::UiOperationMgr& UiOperationMgr::get()
{
    static ref_ptr<UiOperationMgr> pInst(new UiOperationMgr());
    return *pInst.get();
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
            log_error("Creator not defined in class:'%s'", curOperationType->getFullName().c_str());
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
}


// void UiOperationMgr::sendOperationMsg(qd::IOperationEnvironment* env, const qd::TypeInfo& msg_type, qd::operation::args::Base& msg) const
// {
//     auto operationsIter = m_operationsByMsgTypeMap.find(&msg_type);
//     if (operationsIter == m_operationsByMsgTypeMap.end())
//         return;
//     const eastl::vector<UiOperation*>& actList = operationsIter->second;
//     for (UiOperation* pCurAct : actList)
//     {
//         pCurAct->applyOperationMsgProc(env, &msg);
//     }
// }


// EFlow UiOperationMgr::applyOperationMsg(qd::IOperationEnvironment* env, qd::operation::args::Base* p_msg) const
// {
//     for (UiOperation* pCurOperation : m_pOperations)
//     {
//         if (!pCurOperation)
//             continue;
//         EFlow r = pCurOperation->applyOperationMsgProc(env, p_msg);
//         if (r != EFlow::NO_RESULT)
//             return r;
//     }
//     return EFlow::NO_RESULT;
// }


void UiOperationMgr::addOperation(UiOperation* pNewOperation)
{
    assert(0);
//     m_pOperations.push_back(pNewOperation);
//     const qd::TypeInfo* curOperationType = &pNewOperation->getTypeInfo();
//     m_operationByOperationTypeMap[curOperationType] = pNewOperation;

//     OperationSupportedMsgVisitor visitor;
//     pNewOperation->applyOperationMsgProc(env, &visitor);
//     for(const qd::TypeInfo* pCurType : visitor.m_pSupportedMtd)
    {
        //auto& actList = m_operationsByMsgTypeMap[pCurType];

//         auto& actList = m_operationsByMsgTypeMap[curOperationType];
//         actList.push_back(pNewOperation);
    }
}


qd::UiOperation* UiOperationMgr::findOperation(uint32_t class_id) const
{
//     for (UiOperation* pCurOperation : m_pOperations)
//     {
//         if (!pCurOperation || pCurOperation->mClassId != class_id)
//             continue;
//         return pCurOperation;
//     }
    return nullptr;
}




qd::UiOperation* UiOperationMgr::findOperationByType(const qd::TypeInfo& type) const
{
    auto it = m_operationByOperationTypeMap.find(&type);
    if (it == m_operationByOperationTypeMap.end())
        return nullptr;
    return it->second;
}


qd::span<qd::operation::args::OpDesc const> UiOperationMgr::getOperationsList() const
{
    return qd::span<qd::operation::args::OpDesc const>(m_OpDescList.data(),
        //*reinterpret_cast<qd::operation::args::OpDesc* const*>(m_OpDescList.data()),
        m_OpDescList.size());
}


void UiOperationMgr::addOperationDesc(const qd::TypeInfo& ti, qd::operation::args::OpDesc&& desc)
{
    assert(ti.isDefined());
    THash32 cid = ti.getCid();
    if (findOpDesc(cid))
    {
        assert2(0, "Operation args '%s' already registered", ti.getFullName().c_str());
        return;
    }
    m_OpDescList.push_back(std::move(desc));
    uint32_t descIdx = (uint32_t)m_OpDescList.size() - 1;
    m_opsCidToDescIdx[cid] = descIdx;
}


 operation::args::OpDesc::~OpDesc()
{
    SAFE_DELETE(m_pShortcuts);
    SAFE_DELETE(m_pOpTemplate);
}


void operation::args::OpDesc::getShortcutGuiStr(qd::InlineString& out) const
{
    if (m_pShortcuts && m_pShortcuts->getNumShortcuts() > 0)
    {
        const qd::Shortcut* pSh = m_pShortcuts->getShortcut(0);
        out = pSh->toString();
    }
    else
    {
        out.clear();
    }
}


}; // namespace qd
