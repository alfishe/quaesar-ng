#include "qd/qimGui/qimGui.h"
#include "qd/log/log.h"
#include "qd/typeSystem/attributesCommon.h"
#include "qd/typeSystem/typeRegistry.h"
#include "qd/qimGui/qimContext.h"
#include "qd/qimGui/qimVisitorLoops.h"


qim::Context* qim::g_pCtx = nullptr;


namespace qim {


qim::Context* qim::getCurrentContext()
{
    return g_pCtx;
}


void beginFrame()
{
    auto pLoop = new loop::CtrlElemVisitor();
    pLoop->setup(g_pCtx, nullptr);
    g_pCtx->pushLoop(pLoop);
}

void endFrame()
{
    g_pCtx->popLoop();
    g_pCtx->endFrame();
}




bool doCtrlElemForLoop(const DeferredCall& curNode, Element** pOutElement, size_t _nFor)
{
    EFlow rr;
    qim::Context* pCtx = getCurrentContext();
    int nFor = (int)_nFor;
    ItNodeState* pStateIt = nullptr;
    BaseIter* pIter = nullptr;

    for (;;)
    {
        BaseLoop* pLoop = pCtx->getCurLoop();
        pIter = pLoop->m_pIter;
        pStateIt = &pIter->getCurVisitIterState();
        if (nFor == 0)
            pStateIt->nodeBegin = true;
        if (nFor != 0)
            pStateIt->nodeEnd = true;

        ELoopState st = pLoop->getNextLoopIterState(*pStateIt);

        switch (st)
        {
        case ELoopState::WANT_NODE_HEAD:
        {
            pStateIt->visitRootHead = true;
            pLoop->onMeetNodeNext();

            pIter->onNextNodeByStr(curNode.m_strNameId);

            ElementData* pElemData = curNode.getOrCreateElement(pCtx);
            assert(*pOutElement == nullptr || *pOutElement == pElemData->m_pElement);
            *pOutElement = pElemData->m_pElement;
            if (pLoop->meetCtrlElemBegin(curNode.m_typeInfo, pElemData))
            {
                BaseLoop* pSubLoop = pCtx->getCurLoop();
                pLoop = pSubLoop;
                if (pSubLoop->isWantSetupElement())
                {
                    curNode.setupElement(pElemData);
                    if (pSubLoop->startLoopWithCtrlElem(pElemData)) // start element
                    {
                        continue;
                    }
                }
            }
        }
        break;

        case ELoopState::WANT_ITER_BODY_ONCE:
        case ELoopState::WANT_MEET_CHILD:
            pLoop->m_pIter->pushCurNode();
            return true; // go deeper to look for child

        case ELoopState::WANT_MEET_CHILD_END:
            pLoop->m_pIter->popNode();
            break;

        case ELoopState::S_CUR_NODE_END:
        {
            if (pStateIt->iterChildBegin && pStateIt->iterChildEnd)
            {
                pStateIt->iterChildEnd = true;
                pLoop->m_pIter->popNode();
            }
            else
            {
                pStateIt->endLoop = true;
                pLoop->onMeetNodeEnd();
            }
        }
        break;
        case ELoopState::WANT_MEET_SIBLING:
            return false;

        case ELoopState::S_END:
        {
            nFor = -1;
            //while (pLoop)
            {
                ref_ptr<BaseLoop> pLoopRef = pLoop;
                BaseLoop* pParentLoop = pLoopRef->m_pParent;
                pLoop->doneLoop();
                pCtx->popLoop(pLoop);
                if (pParentLoop)
                {
                    st = pParentLoop->onSubLoopDetached(pLoop); // notify parent to continue parent loop
                    break;
                    pLoop = pCtx->getCurLoop();
//                     if (/*st == ELoopState::WANT_NODE_HEAD || */!pLoop->m_iter.visitRootHead)
//                     {
//                         nFor = 0;
//                         break; // repeat to meet child again
//                     }
//                     if (/*st == ELoopState::S_RETURN_TO_PARENT*/ !pLoop->m_iter.endLoop)
//                     {
//                         nFor = 1;
//                         break; // invoke meet node end
//                     }
//                     if (st == ELoopState::S_BREAK_ALL)
//                         continue;

                    return false;
                }
                pLoop = pParentLoop;
            } // while (pLoop)

            if (nFor < 0)
                return false; // next sibling
        }
        } // switch
    } // for (;;)
}


void endCtrl(CtrlElement* pElem)
{
    //     g_pCtx->stackPopChild(pElem);
    //     pElem->onEnd(g_pCtx);
    //     pElem->m_bIsNew = false;
}


ELoopState loop_done_imp(BaseLoop* pLoop)
{
    Context* pCtx = pLoop->m_pCtx;
    ELoopState st;

    pLoop->doneLoop();
    ref_ptr<BaseLoop> pLoopRef = pLoop;
    pCtx->popLoop(pLoop);
    if (BaseLoop* pParentLoop = pLoopRef->m_pParent)
    {
        st = pParentLoop->onSubLoopDetached(pLoop); // notify parent to continue parent loop
        return st;
    }
    return ELoopState::S_END;
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
    delete (ctx);
}

void setCurrentContext(Context* ctx)
{
    g_pCtx = ctx;
}


loop::PropsBase* BaseLoop::newSectLoop(const PropertyClassMeta& propMeta)
{
    auto pLoop = new loop::PropsBase();
    pLoop->m_pParent = this;
    pLoop->setup(m_pCtx, propMeta);
    return pLoop;
}



}; // namespace qim
