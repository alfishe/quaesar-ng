#include "operationsRegistry.h"
#include "qd/log/log.h"
#include "qd/typeSystem/attributesCommon.h"
#include "qd/typeSystem/typeRegistry.h"
#include "qd/base/classInfoReg.h"
#include "qd/debug/assert.h"
#include "qd/qui/uiOperation.h"
#include "qd/qui/shortcutMgr.h"



namespace qd {



OperationsRegistry::OperationsRegistry()
{
}


 OperationsRegistry::~OperationsRegistry()
{
    assert(!mInit);
}


qd::OperationsRegistry& OperationsRegistry::get()
{
    static ref_ptr<OperationsRegistry> pInst(new OperationsRegistry());
    return *pInst.get();
}


//////////////////////////////////////////////////////////////////////////


void OperationsRegistry::createOperations(qd::UiOperationCreator* /*ca*/)
{
    mInit = true;
}


void OperationsRegistry::destroy()
{
    mInit = false;
}



qd::span<qd::operation::OpDesc const> OperationsRegistry::getOperationsList() const
{
    return {m_OpDescList.data(), m_OpDescList.size()};
}


void OperationsRegistry::addOperationDesc(const qd::TypeInfo& ti, qd::operation::OpDesc&& desc)
{
    assert(ti.isDefined());
    THash32 cid = ti.getCid();
    if (findOpDesc(cid))
    {
        ASSERT_AND_DO(0, return, "Operation args '%s' already registered", ti.getFullName().c_str());
    }
    m_OpDescList.push_back(std::move(desc));
    uint32_t descIdx = (uint32_t)m_OpDescList.size() - 1;
    m_opsCidToDescIdx[cid] = descIdx;
}


void OperationsRegistry::testOperationsShortcuts(qd::IOperationEnvironment* pEnv,
    qd::span<qd::operation::OpDesc* const> opDescs)
{
    ShortcutsMgr* pShMgr = ShortcutsMgr::get();
    for(const qd::operation::OpDesc* pCurOpDesc : opDescs)
    {
        if (!pCurOpDesc || !pCurOpDesc->m_pShortcuts)
            continue;
        for (int i = 0; i < pCurOpDesc->m_pShortcuts->getNumShortcuts(); ++i)
        {
            const qd::Shortcut* pSh = pCurOpDesc->m_pShortcuts->getShortcut(i);
            if (pSh && pShMgr->isShortcutTriggered(pSh))
            {
                if (pEnv && pCurOpDesc->m_pOpTemplate)
                {
                    qd::operation::BaseOpArgs* pOpArgs = pCurOpDesc->m_pOpTemplate;
                    if (pOpArgs)
                    {
                        pEnv->setupDefaultOperationArgs(pOpArgs);
                        pEnv->applyOperationMsgProc(pOpArgs);
                    }
                }
                return; // only one operation per frame
            }
        }
    }
}


operation::OpDesc::~OpDesc()
{
    SAFE_DELETE(m_pShortcuts);
    SAFE_DELETE(m_pOpTemplate);
}


const char* operation::OpDesc::getShortcutGuiStr() const
{
    if (m_pShortcuts && m_pShortcuts->getNumShortcuts() > 0)
    {
        const qd::Shortcut* pSh = m_pShortcuts->getShortcut(0);
        return pSh->toString();
    }
    return "";
}


}; // namespace qd
