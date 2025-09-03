#include "window.h"
#include "imgui/imgui.h"
#include "qd/base/tribool.h"


namespace qd {

void UiWindow::drawImp()
{
    assert(!m_title.empty());

    uint32_t flg = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    const bool bModal = isModal();

    Tribool vis;
    if (m_bVisible)
    {
        m_bFocus = ImGui::IsItemFocused();
        assert(!m_title.empty());
        if (m_size.isSizeValid())
        {
            ImVec2 size((float)m_size.x, (float)m_size.y);
            ImGui::SetNextWindowSize(size);
        }

        if (bModal)
        {
            ImGui::OpenPopup(m_title.c_str());
            vis = (Tribool)ImGui::BeginPopupModal(m_title.c_str(), &m_bVisible, flg);
        }
        else
            vis = (Tribool)ImGui::Begin(m_title.c_str(), &m_bVisible, flg);

        if (!m_bVisible)
            c_def(0);
    }
    else
        c_def(0);

    if (vis.isTrue())
    {
        drawContentImp();
    }

    if (vis.hasBool())
    {
        if (bModal)
        {
            if (vis.isTrue())
                ImGui::EndPopup();
        }
        else
            ImGui::End();
    }
    else
        BPT();

}

}; // namespace qd
