#include "qd/app/appPartsMgr.h"
#include "EASTL/sort.h"
#include "qd/app/appMessages.h"
#include "qd/debug/assert.h"
#include "qd/debug/exception.h"
#include "qd/stl/stlUtils.h"
#include "qd/typeSystem/typeInfo.h"
#include <ctime> // std::time


// DECLARE MODULE
QD_MODULE_REGISTRATION(qd::AppPartsManager);


namespace qd {

void AppPartsManager::sendAppEventMsg(qd::appMsg::BaseMsg& in_msg)
{
    for (size_t i = 0; i < m_pParts.size(); ++i)
    {
        AppPartBase* pCurPart = getPartByInd((int)i);
        if (!pCurPart)
            return;
        pCurPart->onAppEventProcImp(in_msg);
    }
}


qd::AppPartBase* AppPartsManager::findPartByName(const qd::string& strPartID) const
{
    for (auto it = m_pParts.begin(); it != m_pParts.end(); ++it)
    {
        AppPartBase* pCurPart = *it;
        if (pCurPart && pCurPart->getPartName() == strPartID)
            return pCurPart;
    }
    return nullptr;
}


int AppPartsManager::findPartIndex(AppPartBase* pPart) const
{
    int nSize = (int)m_pParts.size();
    for (int i = 0; i < nSize; ++i)
    {
        AppPartBase* pCurPart = m_pParts[i];
        if (pCurPart == pPart)
            return i;
    }
    return -1;
}


bool AppPartsManager::addPartTry(ref_ptr<AppPartBase> pPart)
{
    assert(pPart);
    if (!pPart)
        return false;

    const qd::string& strPartName = pPart->getPartName();
    assert(!strPartName.empty());

    if (qd::is_has(pPart, m_pParts))
        return false;

    if (findPartByName(strPartName))
    {
        assert2(0, "addPartTry() Error - Part with this name already exists");
        return false;
    }
    m_pParts.push_back(pPart);
    return true;
}


void AppPartsManager::addPart(ref_ptr<AppPartBase> pPart)
{
    assert(pPart);
    if (!pPart)
        return;

    const qd::string& strPartName = pPart->getPartName();
    if (strPartName.empty())
        pPart->setPartName(pPart->getTypeInfo().getTypeName());

    if (!addPartTry(std::move(pPart)))
    {
        G_THROW_OR_DO(Exception("AddPartError: Duplicate Part Found: \"%s\"", strPartName.c_str()), return);
    }
}


void AppPartsManager::addPart(ref_ptr<AppPartBase> p_part, const qd::string& part_name_id)
{
    p_part->setPartName(part_name_id);
    addPart(p_part);
}


void AppPartsManager::destroyPart(ref_ptr<AppPartBase> pPart)
{
    if (!pPart)
        return;

    int nInd = findPartIndex(pPart);
    if (nInd < 0)
        return;

    m_pParts[nInd] = nullptr;

    if (!pPart->isPartDone())
    {
        pPart->destroy();
        pPart->setPartDone(true);
    }
    pPart = nullptr;
}


void AppPartsManager::destroy()
{
    // CLog1::CSection cs("Destroy All Application Parts");

    for (int i = 0; i < (int)m_pParts.size(); i++)
    {
        int nPart = ((int)m_pParts.size() - 1) - i;
        AppPartBase* pPart = m_pParts[nPart];
        if (!pPart)
            continue;
        if (!pPart->isPartDone())
        {
            qd::appMsg::ON_PRE_DESTROY p;
            pPart->onAppEventProcImp(p);
        }
        c_def(0);
    }

    for (int i = 0; i < (int)m_pParts.size(); i++)
    {
        int nPart = ((int)m_pParts.size() - 1) - i;
        ref_ptr<AppPartBase> pPart = m_pParts[nPart];
        m_pParts[nPart] = nullptr;
        if (!pPart)
            continue;
        if (!pPart->isPartDone())
        {
            pPart->destroy();
            pPart->setPartDone(true);
        }
        c_def(0);
    }
}


AppPartsManager::~AppPartsManager()
{
    assert(m_pParts.empty());
}


void AppPartsManager::update(float dt, float time)
{
    m_TimeNowFrame = (TTime64)std::time(nullptr);
    for (int i = 0; i < getNumAppParts(); i++)
    {
        AppPartBase* pCurPart = getPartByInd(i);

        if (pCurPart && pCurPart->hasMtd(EAppPartMtd::UPDATE))
        {
            pCurPart->updateActivateTime();
            if (!pCurPart->isReadyToActivate())
                continue;

            pCurPart->update(dt, time);
        }
    }
}


void AppPartsManager::onSdlEventProc(SDL_Event& event)
{
    for (int i = 0; i < getNumAppParts(); i++)
    {
        AppPartBase* pCurPart = m_pParts[i];

        if (pCurPart && pCurPart->hasMtd(EAppPartMtd::UPDATE))
        {
            pCurPart->onSdlEventProc(event);
        }
    }

}


void AppPartsManager::render()
{
    static qd::vector<AppPartBase*> pActParts;
    pActParts.clear();

    // MAIN RENDER
    for (int i = 0; i < getNumAppParts(); i++)
    {
        AppPartBase* pCurPart = m_pParts[i];

        if (pCurPart && pCurPart->hasMtd(EAppPartMtd::RENDER))
        {
            pActParts.push_back(pCurPart);
        }
    }

    eastl::stable_sort(pActParts.begin(), pActParts.end(), &_getZOrderSort);

    for (int i = 0; i < (int)pActParts.size(); ++i)
    {
        AppPartBase* pCurPart = pActParts[i];
        pCurPart->render();
    }
}


void AppPartsManager::onModuleMessageProc(qd::moduleMsg::BaseMsg& in_msg)
{
    switch (in_msg.id)
    {
    case qd::moduleMsg::RENDER_IMGUI_DEBUG_INFO_TREE::ID:
    {
        // auto p = static_cast<Enm::EModuleMsg::RENDER_IMGUI_DEBUG_INFO_TREE_t*>(pMsgData);
        // qd::ImAPI::CImGuiBase& im = p->im;
        //_onImGuiDebugControl(im);
    }
    break;
    default:
        break;
    }

    TSuper::onModuleMessageProc(in_msg);
}


void AppPartsManager::_onImGuiDebugControl(qd::ImAPI::CImGuiBase& im)
{
#ifdef CGMOD_IMGUI
    for (AppPartBase* pCurPart : m_pParts)
    {
        if (!pCurPart)
            continue;

        if (!ImGui::TreeNodeEx(pCurPart, ImGuiTreeNodeFlags_Framed, CC(pCurPart->getPartName()), 0))
            continue;

        if (im.TreeNode("BasePartInfo"))
        {
            im.Text("Init: %i", (int)pCurPart->m_bPartInit);
            bool bVis = pCurPart->isPartVisible();
            if (im.Checkbox("IsVisible", &bVis))
            {
                pCurPart->setPartVisisble(bVis);
            }
            float zOrder = pCurPart->m_ZOrder.ToFloat();
            if (im.InputFloat("ZOrder", &zOrder))
                pCurPart->SetZOrder(Fixed32::CreateFromFloat(zOrder));

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
        pCurPart->onAppEventProcImp(&p);

        im.TreePop(); /*pCurPart */
    }
#endif
}


}; // namespace qd
