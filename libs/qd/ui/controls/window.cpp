#include "window.h"
#include "imgui/imgui.h"


namespace qd {

void UiWindow::draw()
{
    bool vis = ImGui::Begin(m_title.c_str(), &m_bVisible, ImGuiWindowFlags_NoScrollbar);
    if (vis)
        drawContentImp();
    ImGui::End();
}

}; // namespace qd
