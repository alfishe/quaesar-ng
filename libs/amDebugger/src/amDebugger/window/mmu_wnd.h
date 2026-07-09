#pragma once
#include "amDebugger/ui/uiView.h"
#include "amDebugger/vm/vmInterface.h"

namespace amD::window {

class MmuWnd : public amD::AmDbgWindow {
    QDB_WINDOW_REGISTER(WndId::MmuWnd, amD::window::MmuWnd, amD::AmDbgWindow);

public:
    virtual void onCreate(UiViewCreateCtx* cp) override;
    virtual void drawContentImp() override;

private:
    double m_lastFetchTime = -1.0;
    int m_cachedCpuModel = 0;
    bool m_cachedMmuEnabled = false;
    bool m_showIdentityPages = false;
    ::IVm::Cpu::MmuStats m_cachedStats;
    qtd::vector<::IVm::Cpu::MmuPage> m_cachedPages;
    qtd::vector<::IVm::Cpu::MmuPage> m_filteredPages;
};

} // namespace amD::window
