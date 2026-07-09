#pragma once
#include "amDebugger/ui/uiView.h"
#include "amDebugger/os/os_introspector.h"

namespace amD::window {

class OsModulesWnd : public amD::AmDbgWindow {
    QDB_WINDOW_REGISTER(WndId::OsModules, amD::window::OsModulesWnd, amD::AmDbgWindow);

public:
    virtual void onCreate(UiViewCreateCtx* cp) override;
    virtual void drawContentImp() override;

    std::string m_selectedLibName;
    std::vector<os::LibraryInfo> m_cachedLibs;
    std::vector<os::RomTag> m_cachedTags;
    bool m_hasScanned = false;
    double m_lastScanTime = 0.0;
};

} // namespace amD::window
