#pragma once
#include <EASTL/vector_map.h>
#include <qdIce/qdCore/nodeBase.h>
#include <qdIce/qdDebug/assert.h>
#include <qdIce/qdSTL/vector.h>
#include <qdIce/qdSTL/vector_map.h>
#include <qdIce/qdTypeSystem/typeDeclare.h>
#include <qdIce/qdUI/actionBase.h>
#include <qdIce/qdUI/actionMsg.h>
#include <EASTL/span.h>

namespace qd {


struct IUiOperationsProvider
{
    TS_REFLECT_CLASS(qd::IUiOperationsProvider, void);

    virtual UiAction* findOperationByType(const qd::TypeInfo& type) const = 0;
    virtual eastl::span<UiAction* const> getOperationsList() const = 0;
};
//////////////////////////////////////////////////////////////////////////



class UiActionMgr : public qd::NodeComp, public qd::IUiOperationsProvider
{
    TS_REFLECT_CLASS(qd::UiActionMgr, qd::NodeComp, qd::IUiOperationsProvider);

    friend class ActionMgrActionsListImp;
    qd::vector<UiAction*> m_pActions;
    qd::vector<UiAction*> m_pFilteredActions; // deprecated
    qd::vector_map<const qd::TypeInfo*, qd::UiAction*> m_actionByActionTypeMap;
    qd::vector_map<const qd::TypeInfo*, qd::vector<qd::UiAction*>> m_actionsByMsgTypeMap;

    bool mInit = false;

public:
    UiActionMgr() = default;
    virtual void onNodeCreated(NodeCreator* mk);
    ~UiActionMgr() { assert(!mInit); }

    void createActions(qd::UiActionCreator* ca);
    void destroy();

    EFlow applyActionMsg(qd::action::msg::Base* p_msg) const;

    void addAction(UiAction* pNewAction);

    UiAction* findAction(uint32_t class_id) const;

    virtual UiAction* findOperationByType(const qd::TypeInfo& type) const override;

    virtual eastl::span<UiAction* const> getOperationsList() const override
    {
        return eastl::span<UiAction* const>(m_pActions);
    }

    template<typename TClass>
    qd::UiAction* getAction_() const
    {
        qd::UiAction* pAct = findOperationByType(qd::typeof_(TClass));
        return static_cast<TClass*>(pAct);
    }

    friend struct ListByMtd;
    struct ListByMtd {
        const UiActionMgr* mpMgr;
        decltype(auto) begin() { return mpMgr->m_pFilteredActions.begin(); }
        decltype(auto) end() { return mpMgr->m_pFilteredActions.end(); }
    }; // struct ListByMtd
    ListByMtd getFilteredActionsByMtd(int id);

    void sendActionMsg(const qd::TypeInfo& msg_type, qd::action::msg::Base& p_msg) const;

    template<class TMsg>
    void sendActionMsgT(TMsg& msg) const
    {
        sendActionMsg(TMsg::getStaticTypeInfo(), msg);
    }

}; // class UiActionMgr
//////////////////////////////////////////////////////////////////////////

struct ActionSupportedMsgVisitor : public action::msg::Base {
    qd::vector<const qd::TypeInfo*> m_pSupportedMtd;

public:
    virtual bool tryCast(const qd::TypeInfo& msg_type) override;
};

}; // namespace qd
