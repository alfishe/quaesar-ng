#include "amDebugger/debugger.h"
#include "amDebugger/debuggerOps.h"
#include "amDebugger/vm/vmInterface.h"


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
    // (or after it has been torn down). Forwarding to a null VM would crash menu/toolbar handlers.
    if (!m_pVm)
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
    if (m_pConnection)
        m_pVm = m_pConnection->getClientVm();
    else
        m_pVm = nullptr;
}


void Debugger::execConsoleCmd(qtd::string&& cmd)
{
    amD::operation::ExecConsoleCmd exec;
    exec.cmd = std::move(cmd);
    applyOperationMsgProcImp(&exec);
}



bool Debugger::isDebugActivated() const
{
    return m_pVm ? m_pVm->getVmDebugMode() == EVmDebugMode::Break : false;
}


void Debugger::setDebugMode(EVmDebugMode debug_mode)
{
    // No-op when no VM is bound (dummy connection / emulator not running).
    if (!m_pVm)
        return;
    m_pVm->setVmDebugMode(debug_mode);
}



void BreakpointsSortedList::init(IVm::VM* vm)
{
    mBreakpoints.clear();
    if (vm && vm->emu)
        vm->emu->initBreakPoints(*this);
}


}; // namespace amD
