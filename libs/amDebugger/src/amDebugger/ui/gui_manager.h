#pragma once
#include <amDebugger/debugger.h>
#include <amDebugger/ui/ui_view.h>
#include <EASTL/vector.h>
#include <qd/base/base.h>
#include <qd/node/node.h>
#include <qd/ui/uiNode.h>

FORWARD_DECLARATION_2(qd, UiView);
FORWARD_DECLARATION_3(qd, operation, Operation);


namespace qd {
class ShortcutsMgr;
class UiOperationMgr;


//------------------------------------------------------------------------
class GuiManager : public qd::UiNode
{
    TS_REFLECT_CLASS(qd::GuiManager, qd::UiNode);
    eastl::vector<ref_ptr<UiView>> m_pWindows;

public:
    Debugger* m_pDbg = nullptr;
    UiOperationMgr* m_pOperationMgr = nullptr;
    ShortcutsMgr* m_pShortcutMgr = nullptr;

public:
    GuiManager(Debugger* dbg);
    virtual void onNodeCreated(NodeCreator* mk) override;

    virtual ~GuiManager();

    void drawImGuiMainFrame();

    void destroy();

    Debugger* getDbg() const { return m_pDbg; }

    template<class T>
    inline T* getWnd_() const
    {
        const uint32_t idx = T::CLASS_ID;
        UiView* curView = m_pWindows[idx];
        return static_cast<T*>(curView);
    }

    void addView(UiView* view);

    qd::UiOperationMgr* getOperationMgr() const { return m_pOperationMgr; }
    qd::ShortcutsMgr* getShortcuts() const { return m_pShortcutMgr; }

private:
    void createAllUiWndows();
    void _drawMainMenuBar();
    void _drawToolBar();

}; // class GUIManager

}; // namespace qd
