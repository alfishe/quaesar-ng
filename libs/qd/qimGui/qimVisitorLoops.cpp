#include "qd/qimGui/qimVisitorLoops.h"


namespace qim::loop
{


void CtrlElemVisitor::onMeetNodeEnd()
{
    if (m_meetIter == ELoopState::WANT_MEET_CHILD)
    {
        m_meetIter = ELoopState::S_END;
        m_pCtx->popStackElement(m_pElement);
    }
}


void CtrlElemVisitor::doneLoop()
{
    // m_pElement->onDrawEnd(m_pCtx);
    // m_pElement->m_bIsNew = false;
}



qim::EFlow CtrlElemVisitor::Loop1(qim::Context* ctx)
{
    EVisitStage st;
    EFlow r = ctx->onCtrlVisitLoopEnd(m_pElement, &st);
    return r;
}


qim::EFlow CtrlElemVisitor::Loop2(qim::Context* ctx)
{
    EFlow rr;
    EVisitStage newVState;
    rr = ctx->onCtrlVisitLoopEnd(m_pElement, &newVState);
    if (rr == qd::EFlow::REPEAT)
    {
        ctx->setCurVisitStage(newVState);
        return qd::EFlow::REPEAT;
    }
    ctx->setCurVisitStage(newVState);
    return rr;
}




bool PropsBase::isWantSetupElement()
{
    ElemData* pElemData = m_pCtx->getStackTreeTopElemData();
    ASSERT_AND_DO(pElemData, return false, "No Parent element");
    EVisitStage curStage = m_pCtx->getCurVisitStage();

    if (curStage.has(EVisitStage::VCollect))
        pElemData->m_supportedVStages |= m_classMeta->visitsAllowed;
    if (!curStage.hasAny(m_classMeta->visitsAllowed))
        return false;
    return true;
}




bool PropsBase::isSectEnterAllowed(Property* pProp, ElemData* pElemData)
{
    // if (pProp->isSectEnterAllowedImp(m_pCtx, pElemData))
    return true;
}




bool EventHandlerLoop::isSectEnterAllowed(Property* pProp, ElemData* pElemData)
{
    if (m_classMeta->visitsAllowed.has(EVisitStage::VEventHandler))
    {
        if (pProp->isSectEnterAllowedImp(m_pCtx, pElemData))
        {
            if (pElemData->m_eventApplied.isDerivedFrom(m_classMeta->primeId))
                return false; // already applied
            pElemData->m_eventApplied.addBaseClass(m_classMeta->primeId);
        }
    }
    return true;
}


bool DrawCtrlElem::meetCtrlElemBegin(const qd::TypeInfo& type, ElemData* pElem)
{
    //m_pCtx->pushNewLoop_<loop::DrawCtrlElem>(this, pElem);
    return true;
}




}; // namespace qim::loop

