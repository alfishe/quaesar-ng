#include "uiView.h"
#include <amDebugger/ui/debuggerDesktop.h>
#include <qd/typeSystem/typeInfo.h>


namespace amD {


void* AmDbgWindow::getOpEnvPtr(const qd::TypeInfo& classType) const
{
    return ui->getOpEnvPtr(classType);
}


Debugger* AmDbgWindow::getDbg() const
{
    return ui->getDbg();
}


IVm::VM* AmDbgWindow::getVm() const
{
    return getDbg()->getVm();
}


void window::ImGuiDemoWindow::drawImp() {
    ImGui::ShowDemoWindow(&m_bVisible);
}


void _onUiWindowCreated(const qd::TypeInfo &meta, UiViewCreateCtx *cp, amD::AmDbgWindow * newInst)
{
//     if (auto typeIdAttr = meta.getAttribute_<qd::tsAttr::CustomClassId32>())
//         newInst->mClassId = typeIdAttr->getId32();
    newInst->onCreate(cp);
}


};  // namespace amD
