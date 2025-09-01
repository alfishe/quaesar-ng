#include "amDebugger/debugger.h"
#include "amDebugger/debuggerOps.h"
#include "amDebugger/vm/vmInterface.h"


namespace amD {

DbgProjOptinons g_opt = {};


Debugger::Debugger(DebuggerApp* _app, ref_ptr<IDbgConnection> pCon)
    : m_pDbgApp(_app)
    , m_pConnection(pCon)
{
    assert(m_pConnection);
}


void Debugger::init()
{
    m_pVm = m_pConnection->getClientVm();
    assert(m_pVm);
}


qd::EFlow Debugger::applyOperationMsgProcImp(qd::operation::args::Base* args)
{
    qd::EFlow r = m_pVm->applyOperationMsgProcImp(args);
    return r;
}


qd::EFlow Debugger::setupDefaultOperationArgsImp(qd::operation::args::Base* args) const
{
    switch (args->getCid())
    {
    case amD::operation::args::DebugWaitScanLines::CID:
    {
        auto* a = args->cast_<amD::operation::args::DebugWaitScanLines>();
        a->waitScanLines = g_opt.traceWaitScanLines;
        return EFlow::DONE;
    }
    break;
    default:
        break;
    }
    return qd::EFlow::NO_RESULT;
}


void Debugger::execConsoleCmd(qd::string&& cmd)
{
    amD::operation::args::ExecConsoleCmd exec;
    exec.cmd = std::move(cmd);
    applyOperationMsgProcImp(&exec);
}



bool Debugger::isDebugActivated() const
{
    return m_pVm->getVmDebugMode() == EVmDebugMode::Break;
}


void Debugger::setDebugMode(EVmDebugMode debug_mode)
{
    m_pVm->setVmDebugMode(debug_mode);
}


}; // namespace amD
