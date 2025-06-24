#include "ui_view.h"
#include <amDebugger/ui/dbgGuiDesktop.h>
#include <qd/typeSystem/typeInfo.h>


namespace amD {

Debugger* AmDbgWindow::getDbg() const
{
    return ui->getDbg();
}


namespace window {

void ImGuiDemoWindow::drawImp() {
    ImGui::ShowDemoWindow(&m_bVisible);
}

};  // namespace window


void _onUiWindowCreated(const qd::TypeInfo &meta, UiViewCreateCtx *cp, amD::AmDbgWindow * newInst)
{
//     if (auto typeIdAttr = meta.getAttribute_<qd::tsAttr::CustomClassId32>())
//         newInst->mClassId = typeIdAttr->getId32();
    newInst->onCreate(cp);
}


};  // namespace amD
