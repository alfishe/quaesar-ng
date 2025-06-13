#pragma once
#include "amDebugger/ui_defs.h"
#include "qd/base/classInfoReg.h"
#include "qd/base/color.h"
#include "qd/ImGui/imgui_eastl.h"
#include "qd/node/node.h"
#include "qd/typeSystem/attributesCommon.h"
#include "qd/ui/uiNode.h"




namespace qd {

class GuiManager;
class Debugger;


struct UiViewCreateCtx {
    GuiManager* gui;
    bool visible = true;

    UiViewCreateCtx(GuiManager* _ui)
        : gui(_ui)
    {}
}; // struct CreateUiViewParams



#define QDB_CLASS_ID(wnd_id) \
public:                      \
    static const uint32_t CLASS_ID = (uint32_t)wnd_id;


//////////////////////////////////////////////////////////////////////////
//
// Base class of all ui
//
class UiView : public qd::UiNode
{
    TS_BEGIN_REFLECT_CLASS_BASE(100, qd::UiView, qd::UiNode);
    TS_END();

public:
    qd::string m_title;
    GuiManager* ui = nullptr;
    uint32_t mClassId = 0;

public:
    UiView() = default;
    virtual ~UiView() {}

    virtual void onCreate(UiViewCreateCtx* cp)
    {
        setVisible(cp->visible);
        ui = cp->gui;
    }

    Debugger* getDbg() const;

}; // class UiView
//////////////////////////////////////////////////////////////////////////


class UiWindow : public UiView
{
    TS_BEGIN_REFLECT_CLASS_BASE(50, qd::UiWindow, qd::UiView);
    TS_END();

public:
    // QDB_CLASS_ID();
    UiWindow() = default;

    virtual void onCreate(UiViewCreateCtx* cp) override { UiView::onCreate(cp); }
    virtual void draw() override;

}; // class UiWindow
//////////////////////////////////////////////////////////////////////////



#define QDB_WINDOW_REGISTER(enumId, ClassName, BaseClass)                \
    TS_BEGIN_REFLECT_CLASS(ClassName, BaseClass);                        \
    TS_ATTRIBUTE(qd::tsAttr::CustomClassId32(enumId));                   \
    TS_ATTRIBUTE(qd::tsAttr::CreateClassCb(&createWindowCb_<ClassName>)); \
    TS_END()



void _onUiWindowCreated(const qd::TypeInfo& meta, UiViewCreateCtx* cp, UiWindow* newInst);

template<class TClass>
static UiView* createWindowCb_(const qd::TypeInfo& meta, UiViewCreateCtx* cp)
{
    TClass* pNewInst = new TClass();
    _onUiWindowCreated(meta, cp, pNewInst);
    return pNewInst;
}


namespace window {

class ImGuiDemoWindow : public qd::UiWindow
{

public:
    virtual void onCreate(UiViewCreateCtx* cp) override
    {
        UiWindow::onCreate(cp);
        m_title = "ImGui Demo";
        setVisible(false);
    }

    virtual void draw() override;

}; // class
//////////////////////////////////////////////////////////////////////////

}; // namespace window
//////////////////////////////////////////////////////////////////////////






}; // namespace qd
