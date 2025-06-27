#pragma once
#include <amDebugger/vm/memory.h>
#include "qd/math/point2.h"
#include <amDebugger/ui/ui_view.h>

namespace amD {
namespace window {

class MemoryGraphWnd : public AmDbgWindow {
    QDB_WINDOW_REGISTER(WndId::MemoryGraph, amD::window::MemoryGraphWnd, amD::AmDbgWindow);


    ImTextureID mTextureId = 0;
    qd::Size mTextureSize = {-1, -1};
    qd::Size mNewTextureSize = {640, 320};
    float mLastTextureCreateTime = FLT_MIN;
    int mCurBank = MemBank::CHIP;
    int mBankOffset = 0x0;
    int mTextureMod = 0;
    int mStartDragBankOffset = 0;


public:
    virtual void onCreate(UiViewCreateCtx* cp) override {
        AmDbgWindow::onCreate(cp);
        m_title = "Memory graph";
    }

    virtual void drawContentImp() override;

};  // class MemoryGraphWnd

};  // namespace window
};  // namespace amD
