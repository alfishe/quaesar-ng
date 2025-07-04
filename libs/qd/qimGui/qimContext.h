#pragma once
#include "qd/qimGui/qimBase.h"
#include "qd/qimGui/qimElement.h"
#include "qd/stl/hash_map.h"
#include "qd/stl/vector_map.h"


namespace qim {
class Storage;
class Element;
class BaseLoop;
namespace loop {
class CtrlElemVisitor;
class PropsBase;
};


//////////////////////////////////////////////////////////////////////////
class Context
{
    Storage* m_pCurrStorage = nullptr;
    Storage* m_pPrevStorage = nullptr;

    struct StackItem {
        Element* m_pElement = nullptr;
        ElementData* m_pElemData = nullptr;
        EVisitStage m_curVisitStage = EVisitStage::UNDEF;
    };
    qd::vector<StackItem> m_pChildStack;

    qd::vector_map<const qd::TypeInfo*, BehaviorElem*> m_pBehaviors;
    qd::hash_map<const Element*, ElementData*> m_pElemDataMap;

    qd::vector<ref_ptr<BaseLoop>> m_pLoopStack;
    BaseLoop* m_pCurLoop = nullptr;


public:
    void init();
    void done();

    Context();
    ~Context();

    BehaviorElem* findBehavior(const qd::TypeInfo& pBehClassInfo) const;
    qim::ElementData* getOrCreateElement(const char* name_id, const qd::TypeInfo& behClass,
        const qd::TypeInfo& elemClass);

    template<class T, typename... TArgs>
    ElementData* getOrCreateElem_(const char* name_id, TArgs&&... args)
    {
        ElementData* pElement = getOrCreateElement(name_id, T::s_behClass, T::getStaticTypeInfo());
        return pElement;
        //         if (!pElement)
        //             return nullptr;
        //         return static_cast<T*>(pElement);
    }

    bool checkSectStage(EVisitStage suppStages, size_t& nFor);

    void endSect(Section* pOutSect);

    EVisitStage getCurVisitStage() const
    {
        const Context::StackItem& item = m_pChildStack.back();
        return item.m_curVisitStage;
    }
    void setCurVisitStage(EVisitStage st)
    {
        Context::StackItem& item = m_pChildStack.back();
        item.m_curVisitStage = st;
    }

    template<class T>
    T* makeSect_()
    {
        return new T();
    }


    template<class T, typename... TArgs>
    T* getOrCreateSect_(ElementData* pParentElem, TArgs&&... args)
    {
        return &pParentElem->propAdd_<T>();
    }


    Context::StackItem& pushStackElement(Element* pElem);
    void popStackElement(Element* pElem);

    Element* getStackTreeTopElem(int off = 0) const
    {
        if (!off)
            return m_pChildStack.back().m_pElement;
        auto it = m_pChildStack.rbegin() + -off;
        return it->m_pElement;
    }

    ElementData* getStackTreeTopElemData() const;

    ElementData* findElementData(const Element* pElem) const;

    void beginCtrl(CtrlElement* pElem)
    {
        Context* ctx = this;
        pElem->onDrawBegin(ctx);
        ctx->pushStackElement(pElem);
    }

    qd::EFlow endCtrl(CtrlElement* pElem)
    {
        Context* ctx = this;
        pElem->onBeforeDrawEnd(ctx);

        qd::EFlow rr;
        EVisitStage newVState;
        rr = ctx->onCtrlVisitLoopEnd(pElem, &newVState);
        if (rr == qd::EFlow::REPEAT)
        {
            setCurVisitStage(newVState);
            return qd::EFlow::REPEAT;
        }
        setCurVisitStage(newVState);

        ctx->popStackElement(pElem);
        pElem->onDrawEnd(ctx);
        pElem->m_bIsNew = false;
        return qd::EFlow::STOP;
    }

    qd::EFlow onCtrlVisitLoopEnd(CtrlElement* pElem, EVisitStage* pOutVisit) const;

    void endFrame();

    void pushLoop(ref_ptr<BaseLoop> pLoop);
    void popLoop(BaseLoop* pLoop = nullptr);

    BaseLoop* getCurLoop() const { return m_pLoopStack.back().get(); }

    template<class T, typename... TArgs>
    T* pushNewLoop_(BaseLoop* pParent, TArgs&&... args)
    {
        T* pLoop = new T();
        pLoop->setup(this, std::forward<TArgs>(args)...);
        pLoop->m_pParent = pParent;
        if (pParent)
            pParent->onSubLoopAttached(pLoop);
        pushLoop(pLoop);
        return pLoop;
    }

    int getLoopStackSize() const { return (int)m_pLoopStack.size(); }

private:
    void addBehavior(const qd::TypeInfo& pBehClassInfo, BehaviorElem* pInst);

}; // struct Context
//////////////////////////////////////////////////////////////////////////



struct ELoopState {
    enum EType {
        WANT_ITER_BODY_ONCE,
        WANT_MEET_CHILD, // go deeper
        WANT_MEET_CHILD_END,
        WANT_MEET_SIBLING, // break current loop & move to next
        WANT_MEET_SIBLING_END,
        S_END,
        WANT_NODE_HEAD,
        S_CUR_NODE_END,
        S_RETURN_TO_PARENT,
        S_BREAK_ALL,
    };
    ENUM_DECLARE_BASE(qim::, ELoopState, EType, ELoopState::WANT_MEET_CHILD);
};

struct ItNodeState {
    ImGuiID m_nodeId = 0;
    ImGuiID m_parentId = 0;

    bool isRoot = false;
    bool visitedHead = false;
    bool iterChildBegin = false;
    bool iterChildEnd = false;
    bool siblingsBegin = false;
    bool siblingsEnd = false;
    bool endLoop = false;
    bool enterInBodyOnce = false;

    bool nodeBegin = false;
    bool nodeEnd = false;
};


class BaseIter : public qd::RefCounted
{
public:
    ItNodeState m_rootState;
    qd::hash_map<ImGuiID, ItNodeState> m_nodeMap;
    ItNodeState* m_pCurState = nullptr;

    struct StackItem
    {
        ImGuiID m_parentId = 0;
        ImGuiID m_curNodeId = 0;
    };
    qd::vector<StackItem> m_stack;
    ImGuiID m_parentId = 0;
    ImGuiID m_curId = 0;

    struct Cfg
    {
        bool visitRootHead = false;
        bool visitChild = false;
        bool visitSiblings = false;
        bool enterInBodyOnce = false;
    };
    Cfg cfg;

public:
    BaseIter(ImGuiID seed)
    {
//         StackItem& it = m_stack.push_back();
//         it.m_curNodeId = seed;
//         m_curId = seed;
//         ItNodeState& st = m_nodeMap[seed];
//         st.m_nodeId = seed;
//         m_pCurState = &st;
    }

    ItNodeState& onNextNodeByStr(const char* str, const char* str_end = nullptr);

    ItNodeState& getCurVisitIterState() const
    {
        assert(m_pCurState);
        return *m_pCurState;
    }

    qim::ItNodeState& pushCurNode(const char* str, const char* str_end = nullptr);

    void popNode()
    {
        const StackItem& it = m_stack.back();
        m_curId = it.m_curNodeId;
        m_parentId = it.m_parentId;
        m_stack.pop_back();
    }

}; // class BaseIter
//////////////////////////////////////////////////////////////////////////


class BaseLoop : public qd::RefCounted
{
public:
    BaseLoop* m_pParent = nullptr;
    qim::Context* m_pCtx = nullptr;
    ELoopState m_meetIter = ELoopState::WANT_MEET_CHILD;
    ref_ptr<BaseIter> m_pIter = new BaseIter(0);


public:
    void setup(qim::Context* pCtx) { m_pCtx = pCtx; }

    virtual void onMeetNodeNext()
    {
    }

    virtual bool meetCtrlElemBegin(const qd::TypeInfo& type, ElementData* pElem) { return false; }

    virtual void onMeetNodeEnd()
    {
        assert((uint32_t)m_meetIter < (uint32_t)ELoopState::WANT_MEET_SIBLING);
        m_meetIter++;
    }

    virtual loop::PropsBase* newSectLoop(const PropertyClassMeta& propMeta);

    virtual ~BaseLoop() = default;

    virtual ELoopState getNextLoopIterState(ItNodeState& iterSt) const;

    virtual bool isWantSetupElement() { return true; }

    virtual void doneLoop() {}
    virtual EFlow Loop1(qim::Context* ctx) { return EFlow::DONE; }
    virtual void do1(qim::Context* ctx) {}
    virtual EFlow Loop2(qim::Context* ctx) { return EFlow::DONE; }

    virtual bool startLoopWithCtrlElem(ElementData* pElemData) { return true; }
    virtual bool isSectEnterAllowed(Property* pProp, ElementData* pElemData) { return false; }
    virtual void onSubLoopAttached(BaseLoop* pLoop) {}
    virtual ELoopState onSubLoopDetached(BaseLoop* pLoop) { return ELoopState::S_RETURN_TO_PARENT; }

}; // class BaseLoop
//////////////////////////////////////////////////////////////////////////


namespace loop
{
    class PropsBase : public qim::BaseLoop
    {
    public:
        const PropertyClassMeta* m_classMeta = nullptr;
        qim::Property* m_pProp = nullptr;

    public:
        void setup(qim::Context* pCtx, const PropertyClassMeta& propMeta)
        {
            BaseLoop::setup(pCtx);
            m_classMeta = &propMeta;
        }

        virtual bool isWantSetupElement() override;
        virtual void doneLoop() override {}

        virtual EFlow Loop1(qim::Context* ctx) override { return EFlow::DONE; }
        virtual void do1(qim::Context* ctx) override {}
        virtual EFlow Loop2(qim::Context* ctx) override { return EFlow::DONE; }
        virtual bool isSectEnterAllowed(Property* pProp, ElementData* pElemData) override;

    }; // class PropsBase
    //////////////////////////////////////////////////////////////////////////
}




struct DeferredCall {
    const char* m_strNameId;
    const qd::TypeInfo& m_typeInfo;

public:
    DeferredCall(const char* strNameId, const qd::TypeInfo& typeInfo)
        : m_strNameId(strNameId)
        , m_typeInfo(typeInfo)
    {}

    virtual ElementData* getOrCreateElement(qim::Context* pCtx) const = 0;
    virtual void setupElement(ElementData* pElemData) const = 0;
}; // struct DeferredCall



template<typename T, typename... Args>
struct DeferredCall_ : public DeferredCall {
    std::tuple<Args...> m_args;

public:
    DeferredCall_(const char* strNameId, Args&&... args)
        : DeferredCall(strNameId, T::getStaticTypeInfo())
        , m_args(std::forward<Args>(args)...)
    {}

    virtual ElementData* getOrCreateElement(qim::Context* pCtx) const override
    {
        ElementData* pElemData = pCtx->getOrCreateElem_<T>(m_strNameId);
        return pElemData;
    }

    virtual void setupElement(ElementData* pElemData) const override
    {
        T* pElem = pElemData->getElem_<T>();
        auto args = std::tuple_cat(std::make_tuple(pElem, m_strNameId), m_args);
        std::apply(&T::setup, std::move(args));
    }
};



}; // namespace qim
