#include "uae_actions.h"
#include "amDebugger/commonOperations.h"


namespace qd::operation {


int DebugDmaOption::getCurDebugDmaMode() {
    return ::debug_dma;
}


void DebugDmaOption::changeDebugDmaMode(int nMode) {
    eastl::string buf(eastl::string::CtorSprintf(), "v -%d", nMode + 1);
    getDbg()->applyImmediateConsoleCmd(eastl::move(buf));
}


SDL_Window* UaeWndAlwaysOnTop::getEmulatorMainWindow() {
    return app->mUaeWindow;
}


};  // namespace qd::operation
