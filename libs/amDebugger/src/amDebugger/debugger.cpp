#include "amDebugger/debugger.h"
#include "amDebugger/vm/vmInterface.h"
#include "amDebugger/debuggerOps.h"


namespace amD
{


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


void* Debugger::getOpEnvPtr(const qd::TypeInfo& classType) const
{
    return nullptr;
}


qd::EFlow Debugger::applyOperationMsgProc(qd::operation::args::Base* args)
{
    qd::EFlow r = m_pVm->applyOperationMsgProc(args);
    return r;
}


void Debugger::execConsoleCmd(qd::string&& cmd)
{
    amD::operation::args::ExecConsoleCmd exec;
    exec.cmd = std::move(cmd);
    applyOperationMsgProc(&exec);
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
