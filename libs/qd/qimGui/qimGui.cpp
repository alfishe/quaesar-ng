#include "qd/qimGui/qimGui.h"
#include "qd/typeSystem/typeRegistry.h"
#include "qd/typeSystem/attributesCommon.h"
#include "qd/log/log.h"


namespace qim {

qim::Context* qim::getContext()
{
    static Context ctx;
    return &ctx;
}


Context::~Context()
{
    SAFE_DELETE(m_pCurrStorage);
    SAFE_DELETE(m_pPrevStorage);

    while (!m_pBehaviors.empty())
    {
        ElementBeh* pBeh = m_pBehaviors.back().second;
        delete pBeh;
        m_pBehaviors.pop_back();
    }
}


bool Context::getElementData(const char* name_id, qim::ElemantData** pOutElem, const qd::TypeInfo& behClass,
    const qd::TypeInfo& elemClass) const
{
    ImGuiID id = ImGui::GetID(name_id);
    if (ElemantData* pExist = m_pCurrStorage->findData(id))
    {
        *pOutElem = pExist;
        return true;
    }

    *pOutElem = nullptr;
    ElementBeh* pBeh = findBehavior(behClass);
    ASSERT_F(pBeh, "Behavior class not found for type '%s', name:'%s'", behClass.getFullName().c_str(), name_id);
    if (!pBeh)
        return true;
    ElemantData* pBaseCtrl = pBeh->createElementData(elemClass);
    if (!pBaseCtrl)
        return true;

    m_pCurrStorage->setData(id, pBaseCtrl);
    pBaseCtrl->onAttach(pBeh);
    *pOutElem = pBaseCtrl;
    return false;
}


qim::ElementBeh* Context::findBehavior(const qd::TypeInfo& pBehClassInfo) const
{
    auto it = m_pBehaviors.find(&pBehClassInfo);
    if (it != m_pBehaviors.end())
        return it->second;
    return nullptr;
}


void Context::addBehavior(const qd::TypeInfo& pBehClassInfo, ElementBeh* pInst)
{
    m_pBehaviors[&pBehClassInfo] = pInst;
}


//////////////////////////////////////////////////////////////////////////




void Context::init()
{
    auto behClassList = qd::TypeRegistry::get()->findAllDerivedFromTypes(qd::typeof_<qim::ElementBeh>());

    for (const qd::TypeInfo* pCurBehClass : behClassList)
    {
        auto* pCreator = pCurBehClass->getAttribute_<qd::tsAttr::CreateClassCb>();
        if (!pCreator)
        {
            SDL_Log("Creator not defined in class:'%s'", pCurBehClass->getFullName().c_str());
            continue;
        }
        qim::ElemBehCreator cv;
        qim::ElementBeh* pNewInstance = pCreator->makeInstance_<qim::ElementBeh>(&cv);
        assert(pNewInstance);

        addBehavior(*pCurBehClass, pNewInstance);
    }

    //addBehavior(qd::typeof_<qim::UiMenuBeh>(), new qim::UiMenuBeh());
}

void beginFrame() {}
void endFrame() {}




}; // namespace qim
