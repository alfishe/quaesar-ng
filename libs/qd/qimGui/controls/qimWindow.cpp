#include "qimWindow.h"


namespace qim {


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
    im = {};

    bool m_bVisible = true; // TODO

    if (bModal)
    {
        ImGui::OpenPopup(m_title.c_str());
        im.retVis = ImGui::BeginPopupModal(m_title.c_str(), &m_bVisible, flg);
    }
    else
    {
        im.retVis = ImGui::Begin(m_title.c_str(), &m_bVisible, flg);
    }

    if (!im.retVis)
        return false;
    return true;
}


void Window::onSectChildEndImp()
{
    // Close wnd
    if (isModal())
    {
        if (im.retVis)
            ImGui::EndPopup();
    }
    else
        ImGui::End(); // Begin + End - should calls forever
}


}; // namespace qim
