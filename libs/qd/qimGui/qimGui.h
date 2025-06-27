#pragma once
#include "qd/qimGui/qimBase.h"
#include "qd/qimGui/qimElement.h"
#include "qd/qimGui/qimPtr.h"
#include "qd/typeSystem/typeInfo.h"
#include "qimContext.h"
#include <imgui/imgui.h>
#include "qd/mem/ptrMath.h"



namespace qim {
class Context;
class Element;
extern Context* g_pCtx;
Context* getCurrentContext();

namespace internal {
};


Context* createContext();
void destroyContext(Context* ctx = nullptr); // NULL = destroy current context
void setCurrentContext(Context* ctx);


void beginFrame();
void endFrame();


template<class T, typename... TArgs>
qim::qptr<T> beginChild_(const char* name_id, TArgs&&... args)
{
    qim::Context* pCtx = getCurrentContext();
    assert(pCtx);
    T* pElem = pCtx->getOrCreateElem_<T>(name_id);
    if (pElem)
    {
        pElem->setup(name_id, std::forward<TArgs>(args)...);
        pCtx->beginCtrl(pCtx, pElem);
    }
    return qim::qptr<T>(pElem);
}


#define QCTRL(TCtrlType, pPtrVar, pNameId, ...)                                                                       \
    for (TCtrlType* pPtrVar = nullptr, *pLocalForCounter = nullptr; qim::beginCtrl_(&pPtrVar, &pLocalForCounter, pNameId, __VA_ARGS__); \
         qim::endCtrl_(&pPtrVar, &pLocalForCounter))


template<class T, typename... TArgs>
bool beginCtrl_(T** pOutElement, T** pLocalForCounter, const char* strNameId, TArgs&&... args)
{
    size_t& nFor = reinterpret_cast<size_t&>(*pLocalForCounter);
    qim::Context* ctx = getCurrentContext();
    if (!nFor)
    {
        assert(*pOutElement == nullptr);
        T* pElem = ctx->getOrCreateElem_<T>(strNameId);
        if (pElem)
        {
            pElem->setup(strNameId, std::forward<TArgs>(args)...);
            ctx->beginCtrl(pElem);
        }
        *pOutElement = pElem;
        return true;
    }
    assert(*pOutElement);
    T* pElem = *pOutElement;

//     if (ctx->onCtrlVisitLoopEnd(pElem) == qd::EFlow::REPEAT)
//         return true;

    qd::EFlow r = ctx->endCtrl(pElem);
    if (r == qd::EFlow::REPEAT)
        return true;
    return false;
}


void endCtrl(CtrlElement* pElem);


template<class T>
void endCtrl_(T** pOutElement, T** pLocalForCounter)
{
    size_t& nFor = reinterpret_cast<size_t&>(*pLocalForCounter);
    ++ nFor;

    Context* pCtx = qim::getCurrentContext();
    EVisitStage curStage = pCtx->getCurVisitStage();
    ElementData* pElemData = pCtx->getStackTreeTopElemData();
    pElemData->m_executedStages |= curStage;
}



#define Q_IF(TCtrlType, pPtrVar, ...)                                                                   \
    for (TCtrlType* pPtrVar = nullptr, *pLocalForCounter = nullptr; qim::beginSect_(&pPtrVar, &pLocalForCounter); \
         qim::endSect_(&pPtrVar, &pLocalForCounter))


template<class TPropSect>
bool beginSect_(TPropSect** pOutSect, TPropSect** pForCounter)
{
    Context* pCtx = qim::getCurrentContext();
    size_t& nFor = reinterpret_cast<size_t&>(*pForCounter);
    if (nFor == 0)
    {
        ElementData* pElemData = pCtx->getStackTreeTopElemData();
        ASSERT_AND_DO(pElemData, return false, "No Parent element");
        EVisitStage curStage = pCtx->getCurVisitStage();
        const PropertyClassMeta& propMeta = TPropSect::s_classMeta;
        if (curStage.has(EVisitStage::VCollect))
            pElemData->m_supportedVStages |= propMeta.visitsAllowed;
        if (!curStage.hasAny(propMeta.visitsAllowed))
            return false;
        TPropSect* pProp = pCtx->getOrCreateSect_<TPropSect>(pElemData);
        *pOutSect = pProp;
        qim::Property* pBaseProp = pProp;
        if (pBaseProp->isSectEnterAllowed(curStage, pCtx, pElemData))
            return true;
        return false;
    }
    return false;
}


template<class T>
void endSect_(T** pOutSect, T** pForCounter)
{
    size_t& nFor = reinterpret_cast<size_t&>(*pForCounter);
    ++ nFor;
}

}; // namespace qim
//////////////////////////////////////////////////////////////////////////
