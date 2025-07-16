#pragma once
#include "EASTL/span.h"
#include "qd/qui/uiNode.h"
#include "qd/typeSystem/typeDeclare.h"
#include "qd/qui/uiOperationMgr.h"


namespace qd {

struct IUiOperationsProvider {
    TS_REFLECT_CLASS(qd::IUiOperationsProvider, void);

    virtual UiOperation* findOperationByType(const qd::TypeInfo& type) const = 0;
    virtual eastl::span<UiOperation* const> getOperationsList() const = 0;
};
//////////////////////////////////////////////////////////////////////////



class UiOperationMgrComp
    : public qd::UiNodeComp
    , public qd::IUiOperationsProvider
{
    TS_REFLECT_CLASS(qd::UiOperationMgrComp, qd::UiNodeComp, qd::IUiOperationsProvider);
public:
    ref_ptr<UiOperationMgr> m_pOpMgr;

public:
    // implements IUiOperationsProvider
    virtual UiOperation* findOperationByType(const qd::TypeInfo& type) const override
    {
        return m_pOpMgr->findOperationByType(type);
    }

    virtual eastl::span<UiOperation* const> getOperationsList() const override
    {
        auto refSpan = m_pOpMgr->getOperationsList();
        return refSpan;
    }
}; // class UiOperationMgrComp
//////////////////////////////////////////////////////////////////////////




}; // namespace qd
