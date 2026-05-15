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
        SDL_Log("Debugger: VM bound, vm=%p, isReady=%d", (void*)m_pVm.get(), 
               m_pVm ? (int)m_pVm->isReady() : -1);
    } else {
        m_pVm = nullptr;
        SDL_Log("Debugger: VM connection cleared");
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
    // No-op when no VM is bound or sub-modules are not yet wired up.
    if (!m_pVm || !m_pVm->isReady())
        return;
    m_pVm->setVmDebugMode(debug_mode);
}



void BreakpointsSortedList::init(IVm::VM* vm)
{
    mBreakpoints.clear();
    if (vm && vm->isReady())
        vm->emu->initBreakPoints(*this);
}


}; // namespace amD
