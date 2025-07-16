#include "window.h"
#include "imgui/imgui.h"
#include "qd/qimGui/controls/qimWindow.h"
#include "qd/qimGui/qimProperty.h"


namespace qd {

void UiWindow::drawImp()
{
    assert(!m_title.empty());

    uint32_t flg = ImGuiWindowFlags_NoScrollbar;
    const bool bModal = isModal();
    bool vis;

    assert(!m_title.empty());

    if (m_size.isSizeValid())
    {
        ImVec2 size((float)m_size.x, (float)m_size.y);
        ImGui::SetNextWindowSize(size);
    }
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
    }

    if (bModal)
        ImGui::EndPopup();
    else
        ImGui::End();

#if 0
    // start draw QImGui window
    QCTRL(qim::Window, pImWindow, m_title.c_str())
    {
        if (m_size.isValid())
        {
            Q_IF(qim::Window::Size, pSize)
            pSize->set(m_size);
        }

        Q_IF(qim::Sect::ChildList, _)
        {
            pImWindow->setVisible(true);
            pImWindow->setModal(isModal());

            if (pImWindow->sectChildBegin())
            {
                // DRAW CHILD
                drawContentImp();
            }
            pImWindow->sectChildEnd();

            if (!pImWindow->isVisible())
                setVisible(false);
        }
    }
#endif //
}

}; // namespace qd
