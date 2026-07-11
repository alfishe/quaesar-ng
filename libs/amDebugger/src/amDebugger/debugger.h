#pragma once
#include "amDebugger/vm/memory.h"
#include "amDebugger/vm/vmInterface.h"
#include "dbgConnection.h"
#include <EASTL/fixed_set.h>
#include "qd/stl/fixed_vector.h"
#include "qd/qui/uiOperation.h"
#include <memory>
#include "amDebugger/os/os_introspector.h"


namespace amD {

class DebuggerApp;
class IVmDbgServiceBridge;

constexpr static int BREAKPOINTS_MAX = 20;


class Breakpoint
{
public:
    AddrRef addr1 = {};
    AddrRef addr2 = {};
    bool enabled = false;
    IVm::EReg reg;
}; // class Breakpoint
//////////////////////////////////////////////////////////////////////////


class BreakpointsSortedList
{
public:
    qtd::fixed_vector<Breakpoint, amD::BREAKPOINTS_MAX, false> mBreakpoints;

    struct OneAddrBp {
        AddrRef addr = 0;
        int bpIdx = 0;
        bool operator< (const OneAddrBp& rh) const { return addr < rh.addr; }
    };
    eastl::fixed_set<OneAddrBp, amD::BREAKPOINTS_MAX, false> mOneAddrBps;

public:
    void init(IVm::VM* vm);
    const amD::Breakpoint* getBpByAddr(AddrRef addr, EReg reg) const;
}; // BreakpointsSortedList
//////////////////////////////////////////////////////////////////////////


struct DbgProjOptinons
{
    int traceWaitScanLines = 1;
};
extern DbgProjOptinons g_opt;


//------------------------------------------------------------------------
// Debugger client engine
//
class Debugger
    : public qd::RefCounted
    , public qd::IOperationEnvironment
{
    DebuggerApp* m_pDbgApp = nullptr;
    ref_ptr<IVmDbgServiceBridge> m_pConnection;
    ref_ptr<IVm::VM> m_pVm = nullptr; // owner
    std::unique_ptr<os::OsIntrospector> m_pOsIntro;

public:
    Debugger(DebuggerApp* _app);
    virtual ~Debugger() override;

    IVm::VM* getVm() const;
    amD::DebuggerApp* getDbgApp() const { return m_pDbgApp; }
    os::OsIntrospector* getOsIntro() const { return m_pOsIntro.get(); }

    void setDbgServiceBridge(ref_ptr<IVmDbgServiceBridge> pCon);

    void execConsoleCmd(qtd::string&& cmd);

    int getWaitScanLines() const { return g_opt.traceWaitScanLines; }
    void setWaitScanLines(int waitScanLines) { g_opt.traceWaitScanLines = waitScanLines; }

    BreakpointsSortedList getBreakpointsSorted() const
    {
        BreakpointsSortedList bp;
        bp.init(m_pVm);
        return bp;
    }

    bool isDebugActivated() const;
    void setDebugMode(EVmDebugMode debug_mode);
    void fetchVmState();

    //------------------------------------------------------------------------
    // Implement Operation Environment
    virtual IOperationEnvironment* getOpEnvParent() const override { return nullptr; }
    virtual qd::EFlow applyOperationMsgProcImp(qd::operation::BaseOpArgs* args) override;
    virtual qd::EFlow setupDefaultOperationArgsImp(qd::operation::BaseOpArgs* args) const override;

}; // class Debugger
//////////////////////////////////////////////////////////////////////////


}; // namespace amD
