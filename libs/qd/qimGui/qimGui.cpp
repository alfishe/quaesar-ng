#include "qd/qimGui/qimGui.h"
#include "qd/typeSystem/typeRegistry.h"
#include "qd/typeSystem/attributesCommon.h"
#include "qd/log/log.h"
#include "qimContext.h"


namespace qim {

static Context* g_pCtx = nullptr;



qim::Context* qim::getCurrentContext()
{
    return g_pCtx;
}


void beginFrame() {}
void endFrame() {}



void _invokeBegin(Context* ctx, CtrlElement* pElem)
{
    pElem->onBegin(g_pCtx);
    g_pCtx->stackPushChild(pElem);
}


void endCtrl(CtrlElement* pElem)
{
    g_pCtx->stackPopChild(pElem);
    pElem->onEnd(g_pCtx);
    pElem->m_bIsNew = false;
}


qim::Context* createContext()
{
    Context* pPrevCtx = getCurrentContext();
    Context* pNewCtx = new qim::Context();
    setCurrentContext(pNewCtx);
    pNewCtx->init();
    if (pPrevCtx)
        setCurrentContext(pPrevCtx); // Restore previous context if any, else keep new one.
    return pNewCtx;
}


void destroyContext(Context* ctx /*= nullptr*/)
{
    Context* prev_ctx = getCurrentContext();
    if (ctx == NULL) //-V1051
        ctx = prev_ctx;
    setCurrentContext(ctx);
    ctx->done();
    setCurrentContext((prev_ctx != ctx) ? prev_ctx : NULL);
    delete(ctx);
}

void setCurrentContext(Context* ctx)
{
    g_pCtx = ctx;
}


}; // namespace qim
