#pragma once
#include "qd/qimGui/qimBase.h"
#include "qd/qimGui/qimPtr.h"
#include "qd/qimGui/qimElement.h"
#include "qd/typeSystem/typeInfo.h"
#include "qimContext.h"
#include <imgui/imgui.h>



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

#define QCTRL(CtrlType, PtrVar, ...)                                                          \
    for (CtrlType* PtrVar = qim::beginCtrl_<CtrlType>(__VA_ARGS__), *once_ = nullptr; !once_; \
         qim::endCtrl(PtrVar), ++once_)

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



}; // namespace qim
//////////////////////////////////////////////////////////////////////////
