#pragma once
#include <EASTL/vector.h>
#include <amDebugger/debugger.h>
#include <amDebugger/ui/ui_view.h>
#include <qd/qdBase/base.h>
#include <qd/qdCore/nodeBase.h>
#include <qd/qdUI/uiNode.h>

FORWARD_DECLARATION_2(qd, UiView);
FORWARD_DECLARATION_3(qd, operation, Operation);

namespace qd {
class ShortcutsMgr;
class UiOperationMgr;

//------------------------------------------------------------------------
class GuiManager : public qd::UiNode {
  TS_REFLECT_CLASS(qd::GuiManager, qd::UiNode);
  eastl::vector<ref_ptr<UiView>> windows;

public:
  Debugger* m_pDbg = nullptr;
  UiOperationMgr* m_pOperationMgr = nullptr;
  ShortcutsMgr* m_pShortcutMgr = nullptr;

 public:
  GuiManager(Debugger* dbg);
  virtual void onNodeCreated(NodeCreator *mk) override;

  virtual ~GuiManager();

  void drawImGuiMainFrame();

  void _drawToolBar();
  void destroy();

  Debugger* getDbg() const { return m_pDbg; }

  template <class T>
  inline T* getWnd_() const {
    const uint32_t idx = T::CLASS_ID;
    UiView* curView = windows[idx];
    return static_cast<T*>(curView);
  }

  void addView(UiView* view);

  qd::UiOperationMgr* getOperationMgr() const { return m_pOperationMgr; }
  qd::ShortcutsMgr* getShortcuts() const { return m_pShortcutMgr; }

 private:
  void createAllUiWndows();

};  // class GUIManager

};  // namespace qd
