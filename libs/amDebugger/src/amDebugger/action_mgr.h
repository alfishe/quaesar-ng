#pragma once
#include <EASTL/bitset.h>
#include <EASTL/vector_map.h>
#include <qdIce/qdBase/classInfoReg.h>
#include <amDebugger/ui_defs.h>
#include <qdIce/qdBase/types.h>
#include <amDebugger/shortcut/shortcut_list.h>
#include <qdIce/qdUI/actionMsg.h>
#include <qdIce/qdUI/actionBase.h>


namespace qd {

class Debugger;
class GuiManager;

namespace action
{

struct AmDebuggerActionCreator : public qd::action::ActionCreator {
  GuiManager *gui = nullptr;
  Debugger *dbg = nullptr;
};


class Action : public qd::UiAction
{
  GuiManager *gui = nullptr;

public:

  virtual void onNodeCreated(NodeMaker *cp) override;

  virtual void onDrawMainMenuItem(UiDrawEvent::Type event, void * = nullptr);
  void doActionBase();


  Debugger *getDbg() const;


}; // class Action

	


};  // namespace action
};  // namespace qd
