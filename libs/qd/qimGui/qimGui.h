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
        _invokeBegin(pCtx, pElem);
    }
    return qim::qptr<T>(pElem);
}

#define QCTRL(TCtrlType, pPtrVar, ...)                                                  \
    for (TCtrlType* pPtrVar = qim::beginCtrl_<TCtrlType>(__VA_ARGS__), *once_ = nullptr; \
         qim::hasEventLoop(pPtrVar, once_); qd::ptrAddSelf(once_, 1))

template<class T, typename... TArgs>
T* beginCtrl_(const char* name_id, TArgs&&... args)
{
    qim::Context* pCtx = getCurrentContext();
    assert(pCtx);
    T* pElem = pCtx->getOrCreateElem_<T>(name_id);
    if (pElem)
    {
        pElem->setup(name_id, std::forward<TArgs>(args)...);
        _invokeBegin(pCtx, pElem);
    }
    return pElem;
}

void endCtrl(CtrlElement* pElem);


void _invokeBegin(Context* ctx, CtrlElement* pElem);

bool hasEventLoop(CtrlElement* pElem, const CtrlElement* loop_mark);


}; // namespace qim
//////////////////////////////////////////////////////////////////////////
