#pragma once
#include <qdIce/qdBase/base.h>
#include <qdIce/qdDebug/assert.h>
#include <qdIce/qdUI/actionMsg.h>
#include <qdIce/qdBase/types.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/bitset.h>
#include <EASTL/string.h>
#include <qdIce/qdBase/classInfoReg.h>
#include <qdIce/qdCore/nodeBase.h>
#include <qdIce/qdTypeSystem/typeDeclare.h>


namespace qd
{
class GuiManager;

// Action's component classId
enum class EActionCompsClassId {
    Shortcuts,
    MOST_COMMON_COMPS,
};

struct UiActionCreator : public qd::NodeCreator
{};


class OperationHistory
{
public:
};


//////////////////////////////////////////////////////////////////////////
class UiAction : public qd::Node
{
    TS_REFLECT_CLASS_BASE(200, qd::UiAction, qd::Node);

public:
    uint32_t mClassId = -1;
    eastl::string m_name;
    eastl::string m_description;
    bool m_bActive = true;

public:
    UiAction() = default;
    virtual ~UiAction() = default;


    virtual void onActionCreate(qd::UiActionCreator* cp) {
        onNodeCreated(cp);
    }

    virtual void destroy() {
    }

    void doActionBase();

    virtual EFlow applyActionMsgProc(action::msg::Base* msg) {
        return _applyMsgProcDefImp(msg);
    }

    void addShortcut(int sid);

    virtual bool hasMtd(int id) const { return false; /*supportMtd[id];*/ }

    virtual void doAction(OperationHistory& history = OperationHistory())
    {
        action::msg::DoAction msg;
        applyActionMsgProc(&msg);
    }
    virtual void undoOperation(OperationHistory& history)
    {
    }

    bool isActive() const { return m_bActive; }
    void setActive(bool Active) { m_bActive = Active; }


protected:
    EFlow _applyMsgProcDefImp(action::msg::Base* pBaseMtd);

};  // class UiAction
//////////////////////////////////////////////////////////////////////////




namespace details {
using ActionClassRegistry = ClassInfoRegistry_<UiAction>;
extern uint32_t qdbActionAutoClassId;

template <class TClass>
struct AutoRegistrator {
    AutoRegistrator(uint32_t class_id) {
        EASTL_ASSERT(class_id != 0);
        ActionClassRegistry::MetaInfo metaInfo;
        metaInfo.classId = class_id != 0 ? class_id : ++qdbActionAutoClassId;
        metaInfo.createCallback = (void*)&createClassCb;
        metaInfo.rtti = &typeid(TClass);
        metaInfo.registerClass();
    }

    static UiAction* createClassCb(const ActionClassRegistry::MetaInfo& meta, UiActionCreator* cp) {
        TClass* pInst = new TClass();
        pInst->mClassId = meta.classId;
        pInst->onCreate(cp);
        pInst->TClass::setup();
        return pInst;
    }
};  // struct AutoRegistrator
//////////////////////////////////////////////////////////////////////////

#define QDB_ACTION_REGISTER(TClass, classId) \
    static action::details::AutoRegistrator<TClass> EA_PREPROCESSOR_JOIN(_rgact_no_, __COUNTER__)((uint32_t)classId);

};  // namespace details
//////////////////////////////////////////////////////////////////////////



}; // namespace qd
