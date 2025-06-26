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

namespace internal {
};


Context* createContext();
void destroyContext(Context* ctx = nullptr); // NULL = destroy current context
Context* getCurrentContext();
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
    for (TCtrlType* pPtrVar = nullptr, *pForCounter = nullptr; qim::beginCtrl_(&pPtrVar, &pForCounter, pNameId, __VA_ARGS__); \
         qim::endCtrl_(&pPtrVar, &pForCounter))

#define Q_IF(TCtrlType, pPtrVar, ...)                                                                   \
    for (TCtrlType* pPtrVar = nullptr, *pForCounter = nullptr; qim::beginSect_(&pPtrVar, &pForCounter); \
         qim::endSect_(&pPtrVar, &pForCounter))


template<class T, typename... TArgs>
bool beginCtrl_(T** pOutSect, T** pForCounter, const char* name_id, TArgs&&... args)
{
    size_t& nFor = reinterpret_cast<size_t&>(*pForCounter);
    qim::Context* ctx = getCurrentContext();
    if (!nFor)
    {
        assert(*pOutSect == nullptr);
        T* pElem = ctx->getOrCreateElem_<T>(name_id);
        if (pElem)
        {
            pElem->setup(name_id, std::forward<TArgs>(args)...);
            ctx->beginCtrl(pElem);
        }
        *pOutSect = pElem;
        return true;
    }
    assert(*pOutSect);
    T* pElem = *pOutSect;
    if (ctx->nextCtrlLoop(pElem))
        return true;

    ctx->endCtrl(pElem);
    return false;
}

void endCtrl(CtrlElement* pElem);

template<class T>
void endCtrl_(T** pOutSect, T** pForCounter)
{
    size_t& nFor = reinterpret_cast<size_t&>(*pForCounter);
    ++ nFor;

//     qim::Context* ctx = getCurrentContext();
//     size_t& nFor = reinterpret_cast<size_t&>(*pForCounter);
//     Section** pCurSect = reinterpret_cast<Section**>(pOutSect);
//     return g_pCtx->endCtrl(*pCurSect, nFor);
}



bool hasEventLoop(CtrlElement* pElem, const CtrlElement* loop_mark);



template<class T>
bool beginSect_(T** pOutSect, T** pForCounter)
{
    Context* pCtx = g_pCtx;
    ElementData* pData = pCtx->getStackTreeTopElemData();
    ASSERT_AND_DO(pData, return false, "No Parent element");
    EVisitStage curStage = pCtx->getCurVisitStage();
    const EVisitStage& vf = T::getVisitFlagsStatic();
    if (curStage.has(EVisitStage::VCollect))
        pData->m_supportStages |= vf;
    if (!curStage.hasAny(vf))
        return false;

//     size_t& nFor = reinterpret_cast<size_t&>(*pForCounter);
//     if (!(*pOutSect) && !pCtx->checkSectStage(vf, nFor))
//         return false;

    *pOutSect = pCtx->getOrCreateSect_<T>();
    return true;
}


template<class T>
void endSect_(T** pOutSect, T** pForCounter)
{
    size_t& nFor = reinterpret_cast<size_t&>(*pForCounter);
    ++ nFor;
    //Section** pCurSect = reinterpret_cast<Section**>(pOutSect);
    //return g_pCtx->endSect(*pCurSect, nFor);
}

}; // namespace qim
//////////////////////////////////////////////////////////////////////////
