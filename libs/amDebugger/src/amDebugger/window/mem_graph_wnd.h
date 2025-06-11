#pragma once
#include <amDebugger/vm/memory.h>
#include <qd/Base/types.h>
#include <amDebugger/ui/ui_view.h>

namespace qd {
namespace window {

class MemoryGraphWnd : public UiWindow {
    QDB_WINDOW_REGISTER(WndId::MemoryGraph, qd::window::MemoryGraphWnd, qd::UiWindow);


    ImTextureID mTextureId = 0;
    Int2 mTextureSize = {-1, -1};
    Int2 mNewTextureSize = {640, 320};
    float mLastTextureCreateTime = FLT_MIN;
    int mCurBank = MemBank::CHIP;
    int mBankOffset = 0x0;
    int mTextureMod = 0;
    int mStartDragBankOffset = 0;


public:
    virtual void onCreate(UiViewCreateCtx* cp) override {
        UiWindow::onCreate(cp);
        m_title = "Memory graph";
    }

    virtual void drawContentImp() override;

};  // class

};  // namespace window
};  // namespace qd
