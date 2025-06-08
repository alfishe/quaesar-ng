#pragma once
#include <amDebugger/shortcut/shortcut_list.h>
#include <amDebugger/ui_defs.h>
#include <EASTL/bitset.h>
#include <EASTL/vector_map.h>
#include <qdIce/qdBase/classInfoReg.h>
#include <qdIce/qdBase/types.h>
#include <qdIce/qdTypeSystem/ReflectedType.h>
#include <qdIce/qdUI/actionBase.h>
#include <qdIce/qdUI/actionMsg.h>


namespace qd {
class Debugger;
class GuiManager;

namespace action {
class Action;


//////////////////////////////////////////////////////////////////////////
template<class TClass>
static qd::action::Action* createActionCb_(const qd::TypeInfo& meta, qd::UiActionCreator* cp)
{
    TClass* pNewInst = new TClass();
    pNewInst->onActionCreate(cp);
    pNewInst->setup();
    return pNewInst;
}


#define QDB_REG_ACTION(ClassName)                                     \
    TS_BEGIN_REFLECT_CLASS(ClassName, qd::action::Action);            \
    TS_ATTRIBUTE(qd::CreateClassCbAttr(&createActionCb_<ClassName>)); \
    TS_END();                                                         \
                                                                      \
public:

//////////////////////////////////////////////////////////////////////////


struct AmDebuggerActionCreator : public qd::UiActionCreator {
    GuiManager* gui = nullptr;
    Debugger* dbg = nullptr;
}; // struct AmDebuggerActionCreator
//////////////////////////////////////////////////////////////////////////


class Action : public qd::UiAction
{
    TS_REFLECT_CLASS(qd::action::Action, qd::UiAction);

public:
    GuiManager* gui = nullptr;
    Debugger* dbg = nullptr;

public:
    virtual void onDebuggerActionCreate(qd::action::AmDebuggerActionCreator* cp) {}

    void doActionBase();
    void addShortcut(shortcut::EId sid) { UiAction::addShortcut((int)sid); }
    Debugger* getDbg() const;

    virtual EFlow applyActionMsgProc(action::msg::Base* p_msg) override;

private:
    virtual void onNodeCreated(NodeCreator* cp) override;

}; // class Action
//////////////////////////////////////////////////////////////////////////


}; // namespace action
}; // namespace qd
