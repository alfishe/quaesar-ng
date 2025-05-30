#pragma once
#include <qdIce/qdBase/classInfoReg.h>
#include <amDebugger/ui_defs.h>
#include <qdIce/qdBase/color.h>
#include <qdIce/qdImGui/imgui_eastl.h>
#include <qdIce/qdCore/nodeBase.h>
#include <qdIce/qdTypeSystem/attributesCommon.h>




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



#define QDB_CLASS_ID(wnd_id)                           \
public:                                                \
    static const uint32_t CLASS_ID = (uint32_t)wnd_id; \
                                                       \

//////////////////////////////////////////////////////////////////////////
//
// Base class of all ui
//
class UiView : public qd::Node {
    TS_BEGIN_REFLECT_CLASS_BASE(10000, qd::UiView, qd::Node);
    TS_END();

public:
    qd::string mTitle;
    bool mVisible = true;
    GuiManager* ui = nullptr;
    uint32_t mClassId = 0;

public:
    UiView() = default;

    virtual void onCreate(UiViewCreateCtx* cp) {
        mVisible = cp->visible;
        ui = cp->gui;
    }

protected:
    virtual void updateBeforeDraw() {
    }
    virtual void drawContent() {
    }

public:
    virtual ~UiView() {
    }

    virtual void destroy() {
    }

    virtual void draw() {
        drawContent();
    }

    Debugger* getDbg() const;

};  // class UiView
//////////////////////////////////////////////////////////////////////////


class UiWindow : public UiView {
    TS_BEGIN_REFLECT_CLASS_BASE(50, qd::UiWindow, qd::UiView);
    TS_END();

public:

    // QDB_CLASS_ID();
    UiWindow() = default;

    virtual void onCreate(UiViewCreateCtx* cp) override {
        UiView::onCreate(cp);
    }
    virtual void draw() override;

};  // class UiWindow
//////////////////////////////////////////////////////////////////////////



#define QDB_WINDOW_REGISTER(enumId, ClassName, BaseClass)             \
    TS_BEGIN_REFLECT_CLASS(ClassName, BaseClass);                     \
    TS_ATTRIBUTE(qd::CustomTypeId32Attr(enumId));                     \
    TS_ATTRIBUTE(qd::CreateClassCbAttr(&createWindowCb_<ClassName>)); \
    TS_END() \



void _onUiWindowCreated(const qd::TypeInfo &meta, UiViewCreateCtx *cp, UiWindow* newInst);

template<class TClass>
static UiView* createWindowCb_(const qd::TypeInfo& meta, UiViewCreateCtx* cp)
{
    TClass* pNewInst = new TClass();
    _onUiWindowCreated(meta, cp, pNewInst);
    return pNewInst;
}


namespace window {

class ImGuiDemoWindow : public qd::UiWindow {

public:
    virtual void onCreate(UiViewCreateCtx* cp) override {
        UiWindow::onCreate(cp);
        mTitle = "ImGui Demo";
        mVisible = false;
    }

    virtual void draw() override;

};  // class
//////////////////////////////////////////////////////////////////////////

};  // namespace window
//////////////////////////////////////////////////////////////////////////






};  // namespace qd
