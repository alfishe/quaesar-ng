#pragma once
#include "qd/qimGui/qimContext.h"
#include "qd/qimGui/qimElement.h"


namespace qim::loop {
class ApplyProps;
class ScanChild;
class LoopsParentGroup;



class LoopsParentGroup : public BaseLoop
{
    int m_nSubLoops = 0;

public:
    void setup(qim::Context* pCtx)
    {
        BaseLoop::setup(pCtx);
        m_meetIter = ELoopState::WANT_ITER_BODY_ONCE;
    }

    virtual void onSubLoopAttached(BaseLoop* pLoop) override
    {
        ++m_nSubLoops;
        return;
    }
    virtual ELoopState onSubLoopDetached(BaseLoop* pDoneLoop) override
    {
        assert(m_nSubLoops);
        --m_nSubLoops;

        if (m_nSubLoops > 0)
            return ELoopState::WANT_NODE_HEAD;
        return ELoopState::S_END;
    }
    ~LoopsParentGroup() { assert(m_nSubLoops == 0); }
}; // class LoopsParentGroup
////////////////////////////////////////////////////////////////////////////



class ScanChild : public BaseLoop
{
public:
    bool m_hasProps = false;
    bool m_hasChild = false;

public:
    void setup(qim::Context* pCtx, ElementData* pElemData)
    {
        BaseLoop::setup(pCtx);
        m_pIter = new BaseIter(pElemData->getId());
        m_pIter->cfg.visitRootHead = false;
        m_pIter->cfg.visitChild = false;
        m_pIter->cfg.visitSiblings = true;
        m_pIter->cfg.enterInBodyOnce = true;
        m_meetIter = ELoopState::WANT_ITER_BODY_ONCE;
    }

    virtual void onMeetNodeNext() override { m_meetIter = ELoopState::WANT_MEET_SIBLING; }

    virtual void onMeetNodeEnd() override { m_meetIter = ELoopState::S_END; }

    virtual bool meetCtrlElemBegin(const qd::TypeInfo& type, ElementData* pElem) override
    {
        m_hasChild = true;
        m_meetIter = ELoopState::WANT_MEET_SIBLING;
        return false;
    }
    virtual loop::PropsBase* newSectLoop(const PropertyClassMeta& propMeta) override
    {
        m_hasChild = true;
        m_meetIter = ELoopState::WANT_MEET_SIBLING;
        return nullptr;
    }
}; // class ScanChild
////////////////////////////////////////////////////////////////////////////



class DrawCtrlElem : public qim::BaseLoop
{
    ElementData* m_pElemData;

public:

    DrawCtrlElem()
    {
        c_def(0);
    }

    void setup(qim::Context* pCtx, ElementData* pElemData)
    {
        m_pElemData = pElemData;
        m_pIter = new BaseIter(pElemData->getId());
        m_pIter->cfg.visitRootHead = true;
        m_pIter->cfg.visitChild = true;
        BaseLoop::setup(pCtx);
    }

    virtual bool isWantSetupElement() override { return true; }

    virtual bool meetCtrlElemBegin(const qd::TypeInfo& type, ElementData* pElem) override;

    virtual bool startLoopWithCtrlElem(ElementData* pElemData) override
    {
        assert(m_pElemData);
        //m_pElemData = pElemData;
        auto pElem = static_cast<CtrlElement*>(m_pElemData->m_pElement);
        pElem->onDrawBegin(m_pCtx);
        return false;
    }
    virtual void onMeetNodeEnd() override
    {
        auto pElem = static_cast<CtrlElement*>(m_pElemData->m_pElement);
        pElem->onDrawEnd(m_pCtx);
        m_meetIter = ELoopState::S_END;
    }
}; // class DrawCtrlElem
//////////////////////////////////////////////////////////////////////////



class DrawCtrlElemList : public qim::BaseLoop
{
    int m_nSubLoops = 0;
public:

    void setup(qim::Context* pCtx, ElementData* pElemData)
    {
        BaseLoop::setup(pCtx);
        //m_meetIter = ELoopState::WANT_ITER_BODY_ONCE;
        m_pIter = new BaseIter(pElemData->getId());
        m_pIter->cfg.visitRootHead = true;
        m_pIter->cfg.visitChild = true;
    }

    virtual bool meetCtrlElemBegin(const qd::TypeInfo& type, ElementData* pElemData) override
    {
        m_pCtx->pushNewLoop_<loop::DrawCtrlElem>(this, pElemData);
        return true;
    }

    virtual void onSubLoopAttached(BaseLoop* pLoop) override
    {
        ++m_nSubLoops;
        return;
    }
    virtual ELoopState onSubLoopDetached(BaseLoop* pDoneLoop) override
    {
        assert(m_nSubLoops);
        --m_nSubLoops;
        if (m_nSubLoops > 0)
            return ELoopState::WANT_NODE_HEAD;
        return ELoopState::S_END;
    }
    ~DrawCtrlElemList() { assert(m_nSubLoops == 0); }

}; // class DrawCtrlElemList
//////////////////////////////////////////////////////////////////////////



class ApplyProps : public qim::BaseLoop
{
public:
    virtual bool isWantSetupElement() override { return true; }
};
//////////////////////////////////////////////////////////////////////////



//------------------------------------------------------------------------
class CtrlElemVisitor : public qim::BaseLoop
{
    qim::ElementData* m_pElemData;
    qim::CtrlElement* m_pElement;
    ref_ptr <loop::LoopsParentGroup> m_pSubLoops;
    ref_ptr<loop::ScanChild> m_pScanner;
    int m_nLoopStackTop = -1;

public:
    void setup(qim::Context* pCtx, qim::ElementData* pElemData)
    {
        BaseLoop::setup(pCtx);
        m_pIter = new BaseIter(pElemData ? pElemData->getId() : 0);
        m_pIter->cfg.visitRootHead = true;
        m_pIter->cfg.visitChild = true;

        m_pElemData = pElemData;
        m_pElement = nullptr;
        if (pElemData)
            m_pElement = pElemData->getElem_<CtrlElement>();
    }

    virtual bool meetCtrlElemBegin(const qd::TypeInfo& type, ElementData* pElemData) override
    {
        m_pCtx->pushNewLoop_<loop::CtrlElemVisitor>(this, pElemData);
        return true;
    }

    virtual bool isWantSetupElement() override
    {
        m_pCtx->pushStackElement(m_pElement);
        return true;
    }
    virtual bool startLoopWithCtrlElem(ElementData* pElemData) override
    {
        //m_pSubLoops = m_pCtx->pushNewLoop_<loop::LoopsParentGroup>(this);
        m_pScanner = m_pCtx->pushNewLoop_<loop::ScanChild>(this, m_pElemData);
        return true;
    }

    virtual void onMeetNodeEnd() override;

    virtual void doneLoop();
    virtual EFlow Loop1(qim::Context* ctx) override;
    virtual void do1(qim::Context* ctx) override { m_pElement->onBeforeDrawEnd(ctx); }
    virtual EFlow Loop2(qim::Context* ctx) override;


    virtual ELoopState onSubLoopDetached(BaseLoop* pDoneLoop) override
    {
        if (m_pScanner == pDoneLoop)
        {
            BaseLoop* pLoop = nullptr;
            if (m_pScanner->m_hasChild)
            {
                // draw child if has
                pLoop = m_pCtx->pushNewLoop_<DrawCtrlElemList>(m_pSubLoops.get(), m_pElemData);
            }
            if (m_pScanner->m_hasProps)
            {
                pLoop = m_pCtx->pushNewLoop_<ApplyProps>(m_pSubLoops.get());
            }
            m_pScanner = nullptr;
            if (pLoop)
            {
                return ELoopState::WANT_NODE_HEAD;
                //ELoopState st = pLoop->getNextLoopIterState();
                //return st;
            }
        }
        return ELoopState::S_END; // last subLoob
    }


}; // class CtrlElemVisitor
//////////////////////////////////////////////////////////////////////////




class ChildList : public loop::CtrlElemVisitor
{
public:
    virtual bool isSectEnterAllowed(Property* pProp, ElementData* pElemData) override { return false; }
};


class EventHandlerLoop : public loop::PropsBase
{
public:
    virtual bool isSectEnterAllowed(Property* pProp, ElementData* pElemData) override;

}; // class EventHandlerLoop



}; // namespace qim::loop
//////////////////////////////////////////////////////////////////////////
