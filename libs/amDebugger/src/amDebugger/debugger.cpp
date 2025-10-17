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
    if (m_pVm)
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


void Debugger::execConsoleCmd(qd::string&& cmd)
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
    m_pVm->setVmDebugMode(debug_mode);
}



void BreakpointsSortedList::init(IVm::VM* vm)
{
    mBreakpoints.clear();
    vm->emu->initBreakPoints(*this);
}


}; // namespace amD
