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




}; // namespace qd
