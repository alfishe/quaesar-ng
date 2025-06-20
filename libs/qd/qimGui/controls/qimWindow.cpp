#include "qimWindow.h"


namespace qim
{


bool Window::onSectChildBeginImp()
{
    auto* pSize = propFind_<qim::Props::Size>();

    if (pSize && pSize->isSizeValid())
    {
        ImVec2 size((float)pSize->m_size.x, (float)pSize->m_size.y);
        ImGui::SetNextWindowSize(size);
    }

    uint32_t flg = ImGuiWindowFlags_NoScrollbar;
    const bool bModal = isModal();
    bool bIsVisible = false;

    if (bModal)
    {
        ImGui::OpenPopup(m_title.c_str());
        bIsVisible = ImGui::BeginPopupModal(m_title.c_str(), &m_bVisible, flg);
    }
    else
    {
        bIsVisible = ImGui::Begin(m_title.c_str(), &m_bVisible, flg);
    }

    if (!bIsVisible)
        return false;
    return true;
}


void Window::onSectChildEndImp()
{
    // Close wnd
    if (isModal())
        ImGui::EndPopup();
    else
        ImGui::End();
}


}; // namespace qim
