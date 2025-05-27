#pragma once
#include <qdIce/qdBase/base.h>
#include <qdIce/qdUI/actionMsg.h>
#include <qdIce/qdBase/types.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/bitset.h>
#include <EASTL/string.h>
#include <qdIce/qdBase/classInfoReg.h>
#include <qdIce/qdCore/nodeBase.h>


namespace qd
{
class GuiManager;

// Action's component classId
enum class EActionCompsClassId {
    Shortcuts,
    MOST_COMMON_COMPS,
};

struct UiActionCreator {};


namespace action
{
	
struct ActionCreator : public UiActionCreator {};

	
}; // namespace action





//////////////////////////////////////////////////////////////////////////
class UiAction : public qd::Node
{
public:
    uint32_t mClassId = -1;
    eastl::bitset<64> supportMtd;
    eastl::string mName;
    eastl::string mDesc;

public:
    UiAction() = default;


    void onCreate(UiActionCreator* cp) {
        //gui = cp->gui;
        supportMtd.none();
        mName = "NO NAME";
    }

    virtual void destroy() {
    }

    virtual ~UiAction();

    bool hasMtd(int id) const {
        return supportMtd[id];
    }

    virtual void onDrawMainMenuItem(int event, void* = nullptr);

    void doActionBase();

    virtual EFlow applyMsgProc(action::msg::Base* msg) {
        return _applyMsgProcDefImp(msg);
    }

    void addShortcut(int sid);

protected:
    EFlow _applyMsgProcDefImp(action::msg::Base* pBaseMtd);

};  // class Action
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
