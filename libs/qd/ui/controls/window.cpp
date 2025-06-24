#include "window.h"
#include "imgui/imgui.h"
#include "qd/qimGui/controls/qimWindow.h"
#include "qd/qimGui/qimProperty.h"


namespace qd {

void UiWindow::drawImp()
{
    assert(!m_title.empty());

    // start draw QImGui window
    QCTRL(qim::Window, pImWindow, m_title.c_str())
    {
        if (m_size.isValid())
            pImWindow->propAdd_<qim::Window::Size>().set(m_size);

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

}; // namespace qd
