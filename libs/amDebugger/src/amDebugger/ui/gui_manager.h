#pragma once
#include <EASTL/vector.h>
#include <amDebugger/debugger.h>
#include <amDebugger/ui/ui_view.h>
#include <qdIce/qdBase/base.h>
#include <qdIce/qdCore/nodeBase.h>
#include <qdIce/qdUI/uiNode.h>

FORWARD_DECLARATION_2(qd, UiView);
FORWARD_DECLARATION_3(qd, action, Action);

namespace qd {
class ShortcutsMgr;
class ActionManager;

//------------------------------------------------------------------------
class GuiManager : public qd::UiNode {
  QD_REFLECT_TYPE(GuiManager);
  eastl::vector<ref_ptr<UiView>> windows;
  Debugger* dbg = nullptr;

  ActionManager* m_pActionMgr = nullptr;
  ShortcutsMgr* m_pShortcutMgr = nullptr;

 public:
  GuiManager(Debugger* dbg);
  virtual ~GuiManager();

  void drawImGuiMainFrame();

  void _drawMainToolBar();
  void _drawDebuggerWindows();
  void destroy();

  Debugger* getDbg() const { return dbg; }

  template <class T>
  inline T* getWnd_() const {
    const uint32_t idx = T::CLASS_ID;
    UiView* curView = windows[idx];
    return static_cast<T*>(curView);
  }

  void addView(UiView* view);

  qd::ActionManager* getActionMgr() const { return m_pActionMgr; }
  qd::ShortcutsMgr* getShortcuts() const { return m_pShortcutMgr; }

 private:
  void _drawMainMenuBar();

};  // class GUIManager

};  // namespace qd
