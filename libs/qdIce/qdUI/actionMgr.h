#pragma once
#include <EASTL/vector_map.h>
#include <qdIce/qdUI/actionBase.h>
#include <qdIce/qdDebug/assert.h>
#include <qdIce/qdCore/nodeBase.h>
#include <qdIce/qdTypeSystem/typeInfoReflector.h>


namespace qd {


class UiActionMgr : public qd::NodeComp
{
    TS_REFLECT_CLASS(qd::UiActionMgr, qd::NodeComp);

    typedef eastl::vector<UiAction*> ActionsList;
    eastl::vector<UiAction*> mActions;
    eastl::vector<UiAction*> mFilteredActions;
    eastl::vector_map<const qd::TypeInfo*, qd::UiAction*> mTypeToInstance;
    bool mInit = false;

public:
    UiActionMgr() = default;
    ~UiActionMgr() { assert(!mInit); }

    void createActions(qd::UiActionCreator* ca);
    void destroy();

    EFlow applyActionMsg(qd::action::msg::Base* p_msg) const;

    UiAction* findAction(uint32_t class_id) const;

    template <typename TClass>
    qd::UiAction getAction_() const {
        auto it = mTypeToInstance.find(&qd::typeof_(TClass));
        if (it == mTypeToInstance.end())
            return nullptr;
        return static_cast<TClass*>(it->second);
    }

    const ActionsList& getActions() const {
        return mActions;
    }

    friend struct ListByMtd;
    struct ListByMtd {
        const UiActionMgr* mpMgr;
        decltype(auto) begin() {
            return mpMgr->mFilteredActions.begin();
        }
        decltype(auto) end() {
            return mpMgr->mFilteredActions.end();
        }
    }; // struct ListByMtd
    ListByMtd getFilteredActionsByMtd(int id);


};  // class UiActionMgr


};  // namespace qd
