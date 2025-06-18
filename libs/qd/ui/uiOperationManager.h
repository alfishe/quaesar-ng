#pragma once
#include "EASTL/span.h"
#include "EASTL/vector_map.h"
#include "qd/node/node.h"
#include "qd/debug/assert.h"
#include "qd/stl/vector.h"
#include "qd/stl/vector_map.h"
#include "qd/typeSystem/typeDeclare.h"
#include "qd/ui/uiOperationMessages.h"
#include "qd/ui/uiOperation.h"


namespace qd {
class UiOperation;
struct UiOperationCreator;

struct IUiOperationsProvider {
    TS_REFLECT_CLASS(qd::IUiOperationsProvider, void);

    virtual UiOperation* findOperationByType(const qd::TypeInfo& type) const = 0;
    virtual eastl::span<UiOperation* const> getOperationsList() const = 0;
};
//////////////////////////////////////////////////////////////////////////



class UiOperationMgr
    : public qd::NodeComp
    , public qd::IUiOperationsProvider
{
    TS_REFLECT_CLASS(qd::UiOperationMgr, qd::NodeComp, qd::IUiOperationsProvider);

    friend class OperationMgrOperationsListImp;
    qd::vector<UiOperation*> m_pOperations;
    qd::vector_map<const qd::TypeInfo*, qd::UiOperation*> m_operationByOperationTypeMap;
    qd::vector_map<const qd::TypeInfo*, qd::vector<qd::UiOperation*>> m_operationsByMsgTypeMap;

    bool mInit = false;

public:
    inline static UiOperationMgr* g_pInstance = nullptr;
    static UiOperationMgr* get() { return g_pInstance; }

    UiOperationMgr();
    virtual void onNodeCreated(NodeCreator* mk) override;
    ~UiOperationMgr() { assert(!mInit); }

    void createOperations(qd::UiOperationCreator* ca);
    virtual void destroy() override;

    EFlow applyOperationMsg(qd::operation::msg::Base* p_msg) const;

    void addOperation(UiOperation* pNewOperation);

    UiOperation* findOperation(uint32_t class_id) const;

    virtual UiOperation* findOperationByType(const qd::TypeInfo& type) const override;

    virtual eastl::span<UiOperation* const> getOperationsList() const override
    {
        return eastl::span<UiOperation* const>(m_pOperations);
    }

    template<typename TClass>
    TClass* getOperation_() const
    {
        qd::UiOperation* pAct = findOperationByType(TClass::getStaticTypeInfo());
        return static_cast<TClass*>(pAct);
    }

    void sendOperationMsg(const qd::TypeInfo& msg_type, qd::operation::msg::Base& p_msg) const;

    template<class TMsg>
    void sendOperationMsgT(TMsg& msg) const
    {
        sendOperationMsg(TMsg::getStaticTypeInfo(), msg);
    }

}; // class UiOperationMgr
//////////////////////////////////////////////////////////////////////////



struct OperationSupportedMsgVisitor : public operation::msg::Base {
    qd::vector<const qd::TypeInfo*> m_pSupportedMtd;

public:
    virtual bool tryCast(const qd::TypeInfo& msg_type) override;
};

}; // namespace qd
