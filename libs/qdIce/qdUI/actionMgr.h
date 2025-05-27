#pragma once
#include <EASTL/vector_map.h>
#include <qdIce/qdUI/actionBase.h>
#include <qdIce/qdDebug/assert.h>
#include <qdIce/qdCore/nodeBase.h>


namespace qd {


class ActionManager : public qd::CompBase
{
    QD_REFLECT_TYPE(ActionManager);

    typedef eastl::vector<UiAction*> ActionsList;
    eastl::vector<UiAction*> mActions;
    eastl::vector<UiAction*> mFilteredActions;
    eastl::vector_map<const std::type_info*, UiAction*> mTypeToInstance;
    bool mInit = false;

public:
    ActionManager() = default;
    ~ActionManager() { assert(!mInit); }

    void createActions(action::ActionCreator* ca);
    void destroy();

    EFlow applyActionMsg(qd::action::msg::Base* p_msg) const;

    UiAction* findAction(uint32_t class_id) const {
        for (UiAction* pCurAction : mActions) {
            if (!pCurAction || pCurAction->mClassId != class_id)
                continue;
            return pCurAction;
        }
        return nullptr;
    }

    template <typename TClass>
    UiAction getAction_() const {
        auto it = mTypeToInstance.find(&typeid(TClass));
        if (it == mTypeToInstance.end())
            return nullptr;
        return static_cast<TClass*>(it->second);
    }

    const ActionsList& getActions() const {
        return mActions;
    }

    friend struct ListByMtd;
    struct ListByMtd {
        const ActionManager* mpMgr;
        decltype(auto) begin() {
            return mpMgr->mFilteredActions.begin();
        }
        decltype(auto) end() {
            return mpMgr->mFilteredActions.end();
        }
    }; // struct ListByMtd
    ListByMtd getFilteredActionsByMtd(int id);

    static ActionManager* get() {
        static ActionManager instance;
        return &instance;
    }


};  // class ActionManager


};  // namespace qd
