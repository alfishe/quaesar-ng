#include "actionMgr.h"
#include <qdIce/qdBase/classInfoReg.h>
#include "qdIce/qdTypeSystem/typeRegistry.h"
#include "qdIce/qdTypeSystem/attributesCommon.h"
#include "qdIce/qdLog/log.h"



namespace qd
{

	void UiActionMgr::createActions(qd::UiActionCreator* ca) {
	    assert(!mInit);

        // create all actions
        qd::TypeInfoSpan classTypes = qd::TypeRegistry::get()->findAllDerivedFromTypesCached_<qd::UiAction>(false);
        for (const qd::TypeInfo* curActionType : classTypes)
        {
            auto* pCreator = curActionType->getAttribute_<qd::CreateClassCbAttr>();
            if (!pCreator)
            {
                SDL_Log("Creator not defined in class:'%s'", curActionType->getFullName().c_str());
                continue;
            }
            qd::UiActionCreator cv;
            cv.parent = this;
            qd::UiAction* pNewAction = pCreator->makeInstance_<qd::UiAction>(&cv);
            assert(pNewAction);
            mActions.push_back(pNewAction);
            mTypeToInstance[curActionType] = pNewAction;
        }

	    mInit = true;
	}


	void UiActionMgr::destroy() {
	    mInit = false;
	    while (!mActions.empty()) {
	        delete mActions.back();
	        mActions.pop_back();
	    }
	}


	UiActionMgr::ListByMtd UiActionMgr::getFilteredActionsByMtd(int id) {
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


	EFlow UiActionMgr::applyActionMsg(qd::action::msg::Base* p_msg) const {
	    for (UiAction* pCurAction : mActions) {
	        if (!pCurAction)
	            continue;
	        EFlow r = pCurAction->applyActionMsgProc(p_msg);
	        if (r != EFlow::NO_RESULT)
	            return r;
	    }
	    return EFlow::NO_RESULT;
	}




qd::UiAction* UiActionMgr::findAction(uint32_t class_id) const
    {
        for (UiAction* pCurAction : mActions)
        {
            if (!pCurAction || pCurAction->mClassId != class_id)
                continue;
            return pCurAction;
        }
        return nullptr;
    }


    }; // namespace qd
