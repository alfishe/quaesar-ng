#pragma once
#include "qd/qui/uiOperation.h"
#include "amDebugger/vm/memory.h"
#include "amDebugger/vm/absVM.h"
#include "EASTL/fixed_set.h"
#include "dbgConnection.h"


namespace amD {

class DebuggerApp;
class IDbgConnection;

constexpr static int BREAKPOINTS_MAX = 20;


class Breakpoint
{
public:
    AddrRef addr1 = {};
    AddrRef addr2 = {};
    bool enabled = false;
    EReg reg;
}; // class Breakpoint
//////////////////////////////////////////////////////////////////////////


class BreakpointsSortedList
{
    eastl::fixed_vector<Breakpoint, amD::BREAKPOINTS_MAX, false> mBreakpoints;

    struct OneAddrBp {
        AddrRef addr;
        int bpIdx;
        bool operator< (const OneAddrBp& rh) const { return addr < rh.addr; }
    };
    eastl::fixed_set<OneAddrBp, amD::BREAKPOINTS_MAX, false> mOneAddrBps;

public:
    void init();
    const amD::Breakpoint* getBpByAddr(AddrRef addr, EReg reg) const;
}; // BreakpointsSortedList
//////////////////////////////////////////////////////////////////////////



// Debugger client
//
class Debugger : public qd::RefCounted, public qd::IOperationEnvironment
{
    int m_nTraceWaitScanLines = 1;
    DebuggerApp* m_pDbgApp = nullptr;
    ref_ptr<IDbgConnection> m_pConnection;

public:
    AbsVM::VM* vm = nullptr;

public:
    Debugger(DebuggerApp* _app, ref_ptr<IDbgConnection> pCon)
        : m_pDbgApp(_app)
        , m_pConnection(pCon)
    {
    }
    virtual ~Debugger() = default;

    void init();
    AbsVM::VM* getVm() const { return vm; }
    amD::DebuggerApp* getDbgApp() const { return m_pDbgApp; }

    virtual void* getOpEnvPtr(const qd::TypeInfo& classType) const override;
    virtual qd::EFlow applyOperationMsg(qd::operation::args::Base* args) override;

    amD::IDbgConnection* getConnection() const { return m_pConnection.get(); }

    void sendOperationServerOnly(qd::operation::args::Base* args)
    {
    }
    void sendOperationBoth(qd::operation::args::Base* args)
    {
        sendOperationServerOnly(args);
    }

    void applyOperationLocal(qd::operation::args::Base* args)
    {

    }

    void execConsoleCmd(qd::string&& cmd);

    int getWaitScanLines() const { return m_nTraceWaitScanLines; }
    void setWaitScanLines(int waitScanLines) { m_nTraceWaitScanLines = waitScanLines; }

    BreakpointsSortedList getBreakpointsSorted() const
    {
        BreakpointsSortedList bp;
        bp.init();
        return bp;
    }

    bool isDebugActivated() const
    {
        return vm->emu->m_debugMode == DebuggerMode_Break;
    }
    void setDebugMode(EDebuggerMode debug_mode);


}; // class Debugger


}; // namespace amD
