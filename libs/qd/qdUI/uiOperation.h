#pragma once
#include "qd/qdBase/base.h"
#include "qd/qdBase/classInfoReg.h"
#include "qd/qdBase/types.h"
#include "qd/qdCore/nodeBase.h"
#include "qd/qdDebug/assert.h"
#include "qd/qdSTL/string.h"
#include "qd/qdSTL/fixed_vector.h"
#include "qd/qdTypeSystem/typeDeclare.h"
#include "qd/qdUI/uiOperationManager.h"


namespace qd
{
class GuiManager;

// Operation's component classId
enum class EOperationCompsClassId {
    Shortcuts,
    MOST_COMMON_COMPS,
};

struct UiOperationCreator : public qd::NodeCreator
{};


class OperationHistory
{
public:
};


//////////////////////////////////////////////////////////////////////////
class UiOperation : public qd::Node
{
    TS_REFLECT_CLASS_BASE(200, qd::UiOperation, qd::Node);

public:
    uint32_t mClassId = -1;
    eastl::string m_name;
    eastl::string m_description;
    bool m_bActive = true;

public:
    UiOperation() = default;
    virtual ~UiOperation() = default;


    virtual void onOperationCreate(qd::UiOperationCreator* cp) {
        onNodeCreated(cp);
    }

    virtual void destroy() {
    }

    void doOperationBase();

    virtual EFlow applyOperationMsgProc(operation::msg::Base* msg) {
        return _applyMsgProcDefImp(msg);
    }

    void addShortcut(int sid);

    virtual bool hasMtd(int id) const { return false; /*supportMtd[id];*/ }

    virtual void doOperation(OperationHistory& history = OperationHistory())
    {
        operation::msg::DoOperation msg;
        applyOperationMsgProc(&msg);
    }
    virtual void undoOperation(OperationHistory& history)
    {
    }

    bool isActive() const { return m_bActive; }
    void setActive(bool Active) { m_bActive = Active; }


protected:
    EFlow _applyMsgProcDefImp(operation::msg::Base* pBaseMtd);

};  // class UiOperation
//////////////////////////////////////////////////////////////////////////




namespace details {
using OperationClassRegistry = ClassInfoRegistry_<UiOperation>;
extern uint32_t qdbOperationAutoClassId;

template <class TClass>
struct AutoRegistrator {
    AutoRegistrator(uint32_t class_id) {
        EASTL_ASSERT(class_id != 0);
        OperationClassRegistry::MetaInfo metaInfo;
        metaInfo.classId = class_id != 0 ? class_id : ++qdbOperationAutoClassId;
        metaInfo.createCallback = (void*)&createClassCb;
        metaInfo.rtti = &typeid(TClass);
        metaInfo.registerClass();
    }

    static UiOperation* createClassCb(const OperationClassRegistry::MetaInfo& meta, UiOperationCreator* cp) {
        TClass* pInst = new TClass();
        pInst->mClassId = meta.classId;
        pInst->onCreate(cp);
        pInst->TClass::setup();
        return pInst;
    }
};  // struct AutoRegistrator
//////////////////////////////////////////////////////////////////////////

#define QDB_OPERATION_REGISTER(TClass, classId) \
    static operation::details::AutoRegistrator<TClass> EA_PREPROCESSOR_JOIN(_rgact_no_, __COUNTER__)((uint32_t)classId);

};  // namespace details
//////////////////////////////////////////////////////////////////////////



}; // namespace qd
