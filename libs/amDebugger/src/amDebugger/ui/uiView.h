#pragma once
#include "amDebugger/ui/uiDefs.h"
#include "qd/base/classInfoReg.h"
#include "qd/base/color.h"
#include "qd/imGui/imGui.h"
#include "qd/typeSystem/attributesCommon.h"
#include "qd/qui/controls/window.h"
#include "qd/qui/uiOperation.h"


FORWARD_DECLARATION_2(IVm, VM);


namespace amD {

class DebuggerDesktop;
class Debugger;


struct UiViewCreateCtx {
    DebuggerDesktop* gui;
    bool visible = true;

    UiViewCreateCtx(DebuggerDesktop* _ui)
        : gui(_ui)
    {}
}; // struct UiViewCreateCtx



#define QDB_CLASS_ID(wnd_id) \
public:                      \
    static const uint32_t CLASS_ID = (uint32_t)wnd_id;


//////////////////////////////////////////////////////////////////////////
//
// Base class of all ui
//

class AmDbgWindow : public qd::UiWindow, public qd::IOperationEnvironment
{
    TS_BEGIN_REFLECT_CLASS_BASE(50, amD::AmDbgWindow, qd::UiWindow);
    TS_END();

public:
    DebuggerDesktop* ui = nullptr;

public:
    AmDbgWindow() = default;

    virtual void onCreate(UiViewCreateCtx* cp)
    {
        setVisible(cp->visible);
        ui = cp->gui;
    }

    virtual void drawImp() override
    {
        TSuper::drawImp();
    }
    Debugger* getDbg() const;
    DebuggerDesktop* getUi() {
        return ui;
    }
    IVm::VM* getVm() const;

}; // class AmDbgWindow
//////////////////////////////////////////////////////////////////////////



#define QDB_WINDOW_REGISTER(enumId, ClassName, BaseClass)                      \
    TS_BEGIN_REFLECT_CLASS(ClassName, BaseClass);                              \
    TS_ATTRIBUTE(qd::tsAttr::CustomClassId32(enumId));                         \
    TS_ATTRIBUTE(qd::tsAttr::CreateClassCb(&amD::createWindowCb_<ClassName>)); \
    TS_END()


void _onUiWindowCreated(const qd::TypeInfo& meta, UiViewCreateCtx* cp, AmDbgWindow* newInst);

template<class TClass>
amD::AmDbgWindow* createWindowCb_(const qd::TypeInfo& meta, UiViewCreateCtx* cp)
{
    TClass* pNewInst = new TClass();
    _onUiWindowCreated(meta, cp, pNewInst);
    return pNewInst;
}


namespace window {

class ImGuiDemoWindow : public amD::AmDbgWindow
{

public:
    virtual void onCreate(UiViewCreateCtx* cp) override
    {
        AmDbgWindow::onCreate(cp);
        m_title = "ImGui Demo";
        setVisible(false);
    }

    virtual void drawImp() override;

}; // class
//////////////////////////////////////////////////////////////////////////

}; // namespace window
//////////////////////////////////////////////////////////////////////////



}; // namespace amD
