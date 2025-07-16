#pragma once
#include <amDebugger/debugger.h>
#include <amDebugger/ui/ui_view.h>
#include "qd/qui/controls/desktop.h"
#include "qd/stl/vector.h"
#include "qd/base/base.h"
#include "qd/node/node.h"

FORWARD_DECLARATION_2(amD, UiView);
FORWARD_DECLARATION_2(qd, ShortcutsMgr);
FORWARD_DECLARATION_2(qd, UiOperationMgr);
FORWARD_DECLARATION_3(qd, operation, Operation);


namespace amD {

//------------------------------------------------------------------------
class DbgGuiDesktop : public qd::UiDesktop
{
    TS_REFLECT_CLASS(amD::DbgGuiDesktop, qd::UiDesktop);

public:
    amD::Debugger* m_pDbg = nullptr;
    qd::UiOperationMgr* m_pOperationMgr = nullptr;
    qd::ShortcutsMgr* m_pShortcutMgr = nullptr;

public:
    DbgGuiDesktop(Debugger* dbg);
    virtual void onNodeCreated(qd::UiNodeCreator* mk) override;

    virtual ~DbgGuiDesktop();

    void drawImGuiMainFrame();

    virtual void destroy() override;

    Debugger* getDbg() const { return m_pDbg; }

    template<class T>
    inline T* getWnd_() const
    {
        const uint32_t idx = T::CLASS_ID;
        UiView* curView = m_pWindows[idx];
        return static_cast<T*>(curView);
    }

    qd::UiOperationMgr* getOperationMgr() const { return m_pOperationMgr; }
    qd::ShortcutsMgr* getShortcuts() const { return m_pShortcutMgr; }

private:
    void createAllUiWndows();
    void _drawMainMenuBar();
    void _drawToolBar();

}; // class GUIManager

}; // namespace amD
