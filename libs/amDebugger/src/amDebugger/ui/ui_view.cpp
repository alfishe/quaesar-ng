#include "ui_view.h"
#include <amDebugger/ui/gui_manager.h>
#include <qdIce/qdTypeSystem/typeInfo.h>


namespace qd {

Debugger* UiView::getDbg() const {
    return ui->getDbg();
}

void UiWindow::draw() {
    bool vis = ImGui::Begin(m_title.c_str(), &m_bVisible, ImGuiWindowFlags_NoScrollbar);
    if (vis)
        drawContentImp();
    ImGui::End();
}

namespace window {

void ImGuiDemoWindow::draw() {
    ImGui::ShowDemoWindow(&m_bVisible);
}

};  // namespace window


void _onUiWindowCreated(const qd::TypeInfo &meta, UiViewCreateCtx *cp, UiWindow *newInst)
{
    if (auto typeIdAttr = meta.getAttribute_<qd::CustomTypeId32Attr>())
        newInst->mClassId = typeIdAttr->getId32();
    newInst->onCreate(cp);
}


};  // namespace qd
