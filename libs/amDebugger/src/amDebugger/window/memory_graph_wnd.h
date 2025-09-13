#pragma once
#include <amDebugger/vm/memory.h>
#include "qd/math/point2.h"
#include <amDebugger/ui/uiView.h>
#include "amDebugger/exprValue.h"


namespace amD {
namespace window {

class MemoryGraphWnd : public AmDbgWindow {
    QDB_WINDOW_REGISTER(WndId::MemoryGraph, amD::window::MemoryGraphWnd, amD::AmDbgWindow);

    ImTextureID m_textureId = 0;
    qd::Size m_textureSize = {-1, -1};
    qd::Size m_newTextureSize = {640, 320};
    float m_lastTextureCreateTime = FLT_MIN;
    int m_curBank = MemBank::CHIP;
    int m_bankOffset = 0x0;
    int m_textureMod = 0;
    int mStartDragBankOffset = 0;
    ExprValStr m_exprAddr;


public:
    virtual void onCreate(UiViewCreateCtx* cp) override {
        AmDbgWindow::onCreate(cp);
        m_title = "Memory graph";
    }

    virtual void drawContentImp() override;

};  // class MemoryGraphWnd

};  // namespace window
};  // namespace amD
