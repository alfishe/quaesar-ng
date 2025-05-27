#include "actionMgr.h"
#include <qdIce/qdBase/classInfoReg.h>



namespace qd
{
	
	void ActionManager::createActions(action::ActionCreator* ca) {
	    EASTL_ASSERT(!mInit);
	    // create all actions
	    
	    auto actionClassReg = qd::details::ActionClassRegistry::get();
	    for (auto it : actionClassReg->mClassInfoMap) {
	        UiAction* curAction = actionClassReg->makeInstance(it.first, ca);
	        mActions.push_back(curAction);
	        const qd::details::ActionClassRegistry::MetaInfo& meta = it.second;
	        if (meta.rtti)
	            mTypeToInstance[meta.rtti] = curAction;
	    }
	    mInit = true;
	}
	
	
	void ActionManager::destroy() {
	    mInit = false;
	    while (!mActions.empty()) {
	        delete mActions.back();
	        mActions.pop_back();
	    }
	}
	
	
	ActionManager::ListByMtd ActionManager::getFilteredActionsByMtd(int id) {
	    mFilteredActions.clear();
	    for (UiAction* curAction : mActions) {
	        if (!curAction->hasMtd(id))
	            continue;
	        mFilteredActions.push_back(curAction);
	    }
	    ListByMtd r;
	    r.mpMgr = this;
	    return r;
	}
	
	
	EFlow ActionManager::applyActionMsg(qd::action::msg::Base* p_msg) const {
	    for (UiAction* pCurAction : mActions) {
	        if (!pCurAction)
	            continue;
	        EFlow r = pCurAction->applyMsgProc(p_msg);
	        if (r != EFlow::NO_RESULT)
	            return r;
	    }
	    return EFlow::NO_RESULT;
	}



	
}; // namespace qd
