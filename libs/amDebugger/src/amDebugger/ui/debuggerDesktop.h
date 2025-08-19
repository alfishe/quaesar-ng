#pragma once
#include <amDebugger/debuggerApp.h>
#include <amDebugger/ui/uiView.h>
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
class DebuggerDesktop : public qd::UiDesktop, public qd::IOperationEnvironment
{
    TS_REFLECT_CLASS(amD::DebuggerDesktop, qd::UiDesktop);

public:
    amD::DebuggerApp* m_pDbgApp = nullptr;
    amD::Debugger* m_pDbg = nullptr;
    qd::UiOperationMgr* m_pOperationMgr = nullptr;
    qd::ShortcutsMgr* m_pShortcutMgr = nullptr;

public:
    DebuggerDesktop(amD::DebuggerApp* pDbgApp, Debugger* dbg);
    virtual void onNodeCreated(qd::UiNodeCreator* mk) override;

    virtual ~DebuggerDesktop();

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

    virtual void* getOpEnvPtr(const qd::TypeInfo& classType) const override;

    virtual IOperationEnvironment* getOpEnvParent() const override;
    virtual qd::EFlow applyOperationMsgProc(qd::operation::args::Base* args) override;

private:
    void createAllUiWndows();
    void _drawMainMenuBar();
    void _drawToolBar();

}; // class GUIManager

}; // namespace amD
