#include "window.h"
#include "imgui/imgui.h"


namespace qd {

void UiWindow::draw()
{
    uint32_t flg = ImGuiWindowFlags_NoScrollbar;
    const bool bModal = isModal();
    bool vis;

    assert(!m_title.empty());

    if (bModal)
    {
        ImGui::OpenPopup(m_title.c_str());
        vis = ImGui::BeginPopupModal(m_title.c_str(), &m_bVisible, flg);
    }
    else
        vis = ImGui::Begin(m_title.c_str(), &m_bVisible, flg);

    if (vis)
    {
        drawContentImp();

        if (bModal)
            ImGui::EndPopup();
        else
            ImGui::End();
    }
}

}; // namespace qd
