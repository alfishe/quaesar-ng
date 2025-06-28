#pragma once
#include "qd/mem/ptrMath.h"
#include "qd/qimGui/qimBase.h"
#include "qd/qimGui/qimElement.h"
#include "qd/qimGui/qimPtr.h"
#include "qd/typeSystem/typeInfo.h"
#include "qd/qimGui/qimContext.h"
#include <imgui/imgui.h>



namespace qim {
class Context;
class Element;
extern Context* g_pCtx;
Context* getCurrentContext();


Context* createContext();
void destroyContext(Context* ctx = nullptr); // NULL = destroy current context
void setCurrentContext(Context* ctx);


void beginFrame();
void endFrame();


ELoopState loop_done_imp(BaseLoop* pLoop);


bool doCtrlElemForLoop(const DeferredCall& createElem, Element** pOutElement, size_t nFor);


template<class T, typename... TArgs>
bool doCtrlLoop_(T** pOutElement, T** pLocalForCounter, const char* strNameId, TArgs&&... args)
{
    DeferredCall_<T> createElem(strNameId, std::forward<TArgs>(args)...);
    Element*& pOut = reinterpret_cast<Element*&>(*pOutElement);
    size_t nFor = reinterpret_cast<size_t>(*pLocalForCounter);
    return qim::doCtrlElemForLoop(createElem, &pOut, nFor); // DO FOR LOOP
}


void endCtrl(CtrlElement* pElem);


template<class T>
inline void endCtrl_(T** pOutElement, T** pLocalForCounter)
{
    size_t& nFor = reinterpret_cast<size_t&>(*pLocalForCounter);
    ++nFor;
//     Context* pCtx = qim::getCurrentContext();
//     EVisitStage curStage = pCtx->getCurVisitStage();
//     ElementData* pElemData = pCtx->getStackTreeTopElemData();
//     pElemData->m_executedStages |= curStage;
}


#define QCTRL(TCtrlType, pPtrVar, pNameId, ...)                               \
    for (TCtrlType* pPtrVar = nullptr, *pLocalForCounter = nullptr;           \
         qim::doCtrlLoop_(&pPtrVar, &pLocalForCounter, pNameId, __VA_ARGS__); \
         qim::endCtrl_(&pPtrVar, &pLocalForCounter))


#define Q_IF(TCtrlType, pPtrVar, ...)                                                                              \
    for (TCtrlType* pPtrVar = nullptr, *pLocalForCounter = nullptr; qim::doSectLoop_(&pPtrVar, &pLocalForCounter); \
         qim::endSect_(&pPtrVar, &pLocalForCounter))



template<class TPropSect>
bool doSectLoop_(TPropSect** pOutSect, TPropSect** pForCounter)
{
    Context* pCtx = qim::getCurrentContext();
    size_t& nFor = reinterpret_cast<size_t&>(*pForCounter);
    BaseLoop* pLoop = pCtx->getCurLoop();
    assert(pLoop);
    if (nFor == 0)
    {
        const PropertyClassMeta& propMeta = TPropSect::s_classMeta;
        pLoop = pLoop->newSectLoop(propMeta);
        if (!pLoop)
            return false;
        pCtx->pushLoop(pLoop);
        if (pLoop->isWantSetupElement())
        {
            ElementData* pElemData = pCtx->getStackTreeTopElemData();
            TPropSect* pProp = pCtx->getOrCreateSect_<TPropSect>(pElemData);
            if (!pLoop->isSectEnterAllowed(pProp, pElemData))
                return false;
            *pOutSect = pProp;
            return true;
        }
        if (qim::loop_done_imp(pLoop))
            return true;
        return false;
    }
    EFlow rr;
    rr = pLoop->Loop1(pCtx);
    if (rr == EFlow::REPEAT)
        return true;

    pLoop->do1(pCtx);

    rr = pLoop->Loop2(pCtx);
    if (rr == EFlow::REPEAT)
        return true;

    if (qim::loop_done_imp(pLoop))
        return true;
    return false;
}


template<class T>
void endSect_(T** pOutSect, T** pForCounter)
{
    size_t& nFor = reinterpret_cast<size_t&>(*pForCounter);
    ++nFor;
}

}; // namespace qim
//////////////////////////////////////////////////////////////////////////
