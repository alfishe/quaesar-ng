#include "amDebugger/debugger.h"
#include "amDebugger/debuggerOps.h"
#include "amDebugger/vm/vmInterface.h"
#include <SDL.h>


namespace amD {

DbgProjOptinons g_opt = {};


Debugger::Debugger(DebuggerApp* _app)
    : m_pDbgApp(_app)
{
}

Debugger::~Debugger() = default;




IVm::VM* Debugger::getVm() const
{
    return m_pVm.get();
}


qd::EFlow Debugger::applyOperationMsgProcImp(qd::operation::BaseOpArgs* args)
{
    // VM may be null when the debugger is opened before the emulator instance is created
    // (or after it has been torn down), or not yet ready (sub-modules not wired up).
    // Forwarding to a non-ready VM would crash menu/toolbar handlers that dereference
    // sub-module pointers (cpu, mem, custom, emu, etc.).
    if (!m_pVm || !m_pVm->isReady())
        return qd::EFlow::NO_RESULT;
    qd::EFlow r = m_pVm->applyOperationMsgProcImp(args);
    return r;
}


qd::EFlow Debugger::setupDefaultOperationArgsImp(qd::operation::BaseOpArgs* args) const
{
    switch (args->getCid())
    {
    case amD::operation::DebugWaitScanLines::CID:
    {
        auto* a = args->cast_<amD::operation::DebugWaitScanLines>();
        a->waitScanLines = g_opt.traceWaitScanLines;
        return EFlow::DONE;
    }
    break;
    default:
        break;
    }
    return qd::EFlow::NO_RESULT;
}


void Debugger::fetchVmState()
{
    if (m_pVm && m_pVm->isReady())
        m_pVm->fetchStateFromEmu();
}



void Debugger::setDbgServiceBridge(ref_ptr<IVmDbgServiceBridge> pCon)
{
    if (m_pConnection == pCon)
        return;
    m_pConnection = pCon;
    if (m_pConnection) {
        m_pVm = m_pConnection->getClientVm();
        m_pOsIntro = std::make_unique<os::OsIntrospector>(m_pVm.get());
    } else {
        m_pVm = nullptr;
        m_pOsIntro.reset();
    }
}


void Debugger::execConsoleCmd(qtd::string&& cmd)
{
    if (!m_pVm || !m_pVm->isReady())
        return;
    amD::operation::ExecConsoleCmd exec;
    exec.cmd = std::move(cmd);
    applyOperationMsgProcImp(&exec);
}



bool Debugger::isDebugActivated() const
{
    return (m_pVm && m_pVm->isReady()) ? m_pVm->getVmDebugMode() == EVmDebugMode::Break : false;
}


void Debugger::setDebugMode(EVmDebugMode debug_mode)
{
    // m_pVm is a UI-only mirror VM (see DummyVmDbgServiceBridge) used to track
    // debug mode for menu/toolbar enable state. Its concrete type shares the
    // same process-global engine state as the real running VM (UAE has no
    // per-instance state), so calling the virtual setVmDebugMode() here would
    // re-trigger real engine side effects (activate_debugger_new_pc(), console
    // commands, ...) from the UI thread, racing with the queued operation that
    // already does this correctly on the emulator thread. Bypass the virtual
    // dispatch and only update the mirrored enum.
    if (m_pVm)
        m_pVm->IVm::VM::setVmDebugMode(debug_mode);
}



void BreakpointsSortedList::init(IVm::VM* vm)
{
    mBreakpoints.clear();
    if (vm && vm->isReady())
        vm->emu->initBreakPoints(*this);
}


}; // namespace amD
