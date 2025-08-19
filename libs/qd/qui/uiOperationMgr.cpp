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
    mInit = true;
}


void UiOperationMgr::destroy()
{
    mInit = false;
}



qd::span<qd::operation::args::OpDesc const> UiOperationMgr::getOperationsList() const
{
    return {m_OpDescList.data(), m_OpDescList.size()};
}


void UiOperationMgr::addOperationDesc(const qd::TypeInfo& ti, qd::operation::args::OpDesc&& desc)
{
    assert(ti.isDefined());
    THash32 cid = ti.getCid();
    if (findOpDesc(cid))
        ASSERT_AND_DO(0, return, "Operation args '%s' already registered", ti.getFullName().c_str());
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
