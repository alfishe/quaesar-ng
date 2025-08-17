#include "window.h"
#include "imgui/imgui.h"
#include "qd/qimGui/controls/qimWindow.h"
#include "qd/qimGui/qimProperty.h"


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
            vis = (qd::Tribool)ImGui::BeginPopupModal(m_title.c_str(), &m_bVisible, flg);
        }
        else
            vis = (qd::Tribool)ImGui::Begin(m_title.c_str(), &m_bVisible, flg);

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
