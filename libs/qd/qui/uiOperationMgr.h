#pragma once
#include "qd/node/node.h"
#include "qd/debug/assert.h"
#include "qd/stl/vector_map.h"
#include "qd/stl/span.h"
#include "qd/stl/vector.h"
#include "qd/qui/uiOperation.h"
#include "qd/qui/uiOperationArgs.h"


namespace qd {
class UiOperation;
struct UiOperationCreator;
class IOperationEnvironment;
FORWARD_DECLARATION_3S(operation, args, Base);


class UiOperationMgr : public qd::RefCounted
{
    friend class OperationMgrOperationsListImp;
    qd::vector<ref_ptr<UiOperation>> m_pOperations;
    using TOpList = qd::vector<ref_ptr<UiOperation>>;
    qd::vector_map<const qd::TypeInfo*, qd::UiOperation*> m_operationByOperationTypeMap;
    qd::vector_map<const qd::TypeInfo*, qd::vector<qd::UiOperation*>> m_operationsByMsgTypeMap;

    bool mInit = false;

public:
//     inline static UiOperationMgr* g_pInstance = nullptr;
//     static UiOperationMgr* get() { return g_pInstance; }

    UiOperationMgr();
    virtual ~UiOperationMgr() { assert(!mInit); }

    static UiOperationMgr* get();

    void createOperations(qd::UiOperationCreator* ca);
    virtual void destroy();

    int getNumOps() const { return (int)m_pOperations.size(); }

    //EFlow applyOperationMsg(qd::IOperationEnvironment* env, qd::operation::args::Base* p_msg) const;

    void addOperation(UiOperation* pNewOperation);

    UiOperation* findOperation(uint32_t class_id) const;
    UiOperation* findOperationByType(const qd::TypeInfo& type) const;

    virtual eastl::span<UiOperation* const> getOperationsList() const;

    template<typename TClass>
    TClass* getOperation_() const
    {
        qd::UiOperation* pOp = findOperationByType(TClass::getStaticTypeInfo());
        return static_cast<TClass*>(pOp);
    }

//     template<typename TClass>
//     void doOperation_() const
//     {
//         qd::UiOperation* pOp = findOperationByType(TClass::getStaticTypeInfo());
//         assert(pOp);
//         if (pOp)
//             pOp->doOperation();
//     }

//     void sendOperationMsg(qd::IOperationEnvironment* env, const qd::TypeInfo& msg_type, qd::operation::args::Base& p_msg) const;
//
//     template<class TMsg>
//     void sendOperationMsgT(TMsg& msg) const
//     {
//         sendOperationMsg(TMsg::getStaticTypeInfo(), msg);
//     }

}; // class UiOperationMgr
//////////////////////////////////////////////////////////////////////////





}; // namespace qd
