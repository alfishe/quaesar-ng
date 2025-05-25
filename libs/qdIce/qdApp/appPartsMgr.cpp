#include <qdIce/qdApp/appPartsMgr.h>
#include <qdIce/qdDebug/assert.h>
#include <qdIce/qdDebug/exceptTryCatch.h>
#include <ctime> // std::time
#include <EASTL/sort.h>
#include <qdIce/qdSTL/stlUtils.h>



namespace qd {
	
	qd::BaseAppPart* AppPartsManager::findPartByName(const qd::string& strPartID) const {
	    for (auto it = m_pParts.begin(); it != m_pParts.end(); ++it) {
	        BaseAppPart* pCurPart = *it;
	        if (pCurPart && pCurPart->getPartName() == strPartID)
	            return pCurPart;
	    }
	    return nullptr;
	}


	bool AppPartsManager::addPartTry(ref_ptr<BaseAppPart> pPart) {
	    assert(pPart);
	    if (!pPart)
	        return false;
	
	    const qd::string& strPartName = pPart->getPartName();
	    assert(!strPartName.empty());
	
		if (qd::is_has(pPart, m_pParts)) {
	        return false;
	    }
	
	    if (findPartByName(strPartName)) {
	        assert2(0, "addPartTry() Error - Part with this name already exists");
	        return false;
	    }
	
	    m_pParts.push_back(pPart);
	    return true;
	}


	void AppPartsManager::addPart(ref_ptr<BaseAppPart> pPart) {
	    assert(pPart);
	    if (!pPart)
	        return;

        const qd::string& strPartName = pPart->getPartName();
        if (strPartName.empty())
            pPart->setPartName(pPart->classId().className);

	    if (!addPartTry(pPart)) {
	        G_THROW_OR_DO(Exception("AddPartError: Duplicate Part Found: \"%s\"", strPartName.c_str()), assert(0));
	    }
	}


	void AppPartsManager::destroyPart(ref_ptr<BaseAppPart> pPart) {
	    if (!pPart)
	        return;
	
	    int nInd = findPartIndex(pPart);
	    if (nInd < 0)
	        return;
	
	    m_pParts[nInd] = nullptr;
	
	    if (!pPart->isPartDone()) {
	        pPart->destroy();
	        pPart->setPartDone(true);
	    }
	    pPart = nullptr;
	}

	
	void AppPartsManager::destroy() {
	    //CLog1::CSection cs("Destroy All Application Parts");
	
	    for (int i = 0; i < (int)m_pParts.size(); i++) {
	        int nPart = ((int)m_pParts.size() - 1) - i;
	        BaseAppPart* pPart = m_pParts[nPart];
	        if (!pPart)
	            continue;
	        if (!pPart->isPartDone()) {
	            EAppPartEvent::ON_PRE_DESTROY p;
	            pPart->onAppPartMsgProc(&p);
	        }
	        c_def(0);
	    }
	
	    for (int i = 0; i < (int)m_pParts.size(); i++) {
	        int nPart = ((int)m_pParts.size() - 1) - i;
	        ref_ptr<BaseAppPart> pPart = m_pParts[nPart];
	        m_pParts[nPart] = nullptr;
	        if (!pPart)
	            continue;
	        if (!pPart->isPartDone()) {
	            pPart->destroy();
	            pPart->setPartDone(true);
	        }
	        c_def(0);
	    }
	}

	
	AppPartsManager::~AppPartsManager() {
	    // assert(m_pParts.empty());
	}


	void AppPartsManager::update(CFixed32 Delta, CFixed32 Time) {
	    m_TimeNowFrame = (TTime64)std::time(nullptr);
	    for (int i = 0; i < getNumAppParts(); i++) {
	        BaseAppPart* pCurPart = getPartByInd(i);
	
	        if (pCurPart && pCurPart->hasMtd(EAppPartMtd::UPDATE)) {
	            pCurPart->updateActivateTime();
	            if (!pCurPart->isReadyToActivate())
	                continue;
	
	            pCurPart->update(Delta, Time);
	        }
	    }
	}


	void AppPartsManager::updateWhileLoading(CFixed32 Delta, CFixed32 Time) {
	    m_TimeNowFrame = (TTime64)std::time(nullptr);
	    for (int i = 0; i < getNumAppParts(); i++) {
	        BaseAppPart* pCurPart = getPartByInd(i);
	        if (pCurPart && pCurPart->hasMtd(EAppPartMtd::UPDATE_WHILE_LOADING)) {
	            pCurPart->updateActivateTime();
	            if (!pCurPart->isReadyToActivate())
	                continue;
	
	            pCurPart->update(Delta, Time);
	        }
	    }
	}

	
	void AppPartsManager::render() {
	    static eastl::vector<BaseAppPart*> pActParts;
	    pActParts.clear();
	
	    // MAIN RENDER
	    for (int i = 0; i < getNumAppParts(); i++) {
	        BaseAppPart* pCurPart = getPartByInd(i);
	
	        if (pCurPart && pCurPart->hasMtd(EAppPartMtd::RENDER)) {
	            pActParts.push_back(pCurPart);
	        }
	    }
	
	    eastl::stable_sort(pActParts.begin(), pActParts.end(), &_getZOrderSort);
	
	    for (int i = 0; i < (int)pActParts.size(); ++i) {
	        BaseAppPart* pCurPart = pActParts[i];
	        pCurPart->render();
	    }
	}


	void AppPartsManager::onModuleMessageProc(qd::Enm::EModuleMsg::Msg_t MsgId, void* pMsgData /*= nullptr*/) {
	    switch (MsgId) {
	        case Enm::EModuleMsg::RENDER_IMGUI_DEBUG_INFO_TREE: {
	            //auto p = static_cast<Enm::EModuleMsg::RENDER_IMGUI_DEBUG_INFO_TREE_t*>(pMsgData);
	            //qd::ImAPI::CImGuiBase& im = p->im;
	            //_onImGuiDebugControl(im);
	        } break;
	        default:
	            break;
	    }
	
	    TSuper::onModuleMessageProc(MsgId, pMsgData);
	}


	void AppPartsManager::_onImGuiDebugControl(qd::ImAPI::CImGuiBase& im) {
	#ifdef CGMOD_IMGUI
	    for (BaseAppPart* pCurPart : m_pParts) {
	        if (!pCurPart)
	            continue;
	
	        if (!ImGui::TreeNodeEx(pCurPart, ImGuiTreeNodeFlags_Framed, CC(pCurPart->getPartName()), 0))
	            continue;
	
	        if (im.TreeNode("BasePartInfo")) {
	            im.Text("Init: %i", (int)pCurPart->m_bPartInit);
	            bool bVis = pCurPart->isPartVisible();
	            if (im.Checkbox("IsVisible", &bVis)) {
	                pCurPart->setPartVisisble(bVis);
	            }
	            float zOrder = pCurPart->m_ZOrder.ToFloat();
	            if (im.InputFloat("ZOrder", &zOrder))
	                pCurPart->SetZOrder(CFixed32::CreateFromFloat(zOrder));
	
	            im.Text("hasMtd UPDATE: %i", (int)pCurPart->hasMtd(EAppPartMtd::UPDATE));
	            im.Text("hasMtd UPDATE_WHILE_LOADING: %i", (int)pCurPart->hasMtd(EAppPartMtd::UPDATE_WHILE_LOADING));
	
	            im.Text("Done: %i", (int)pCurPart->m_bPartDone);
	            im.Text("nUpdates: %i", (int)pCurPart->m_nUpdates);
	            im.TreePop();
	        }
	        im.Separator();
	
	        // SEND TO PART
	        EPartEvent::RENDER_IMGUI_DEBUG_INFO_TREE p;
	        p.pIm = &im;
	        pCurPart->onAppPartMsgProc(&p);
	
	        im.TreePop(); /*pCurPart */
	    }
	#endif
	}
	
	
}; // namespace qd
