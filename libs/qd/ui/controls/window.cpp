#include "window.h"
#include "imgui/imgui.h"
#include "qd/qimGui/controls/qimWindow.h"
#include "qd/qimGui/qimProperty.h"


namespace qd {

void UiWindow::draw()
{
    assert(!m_title.empty());

    QCTRL(qim::Window, pWnd, m_title.c_str())
    {
         if (m_size.isValid())
             pWnd->propAdd_<qim::Window::Size>().set(m_size);

        pWnd->setModal(isModal());

        if (pWnd->sectChildBegin())
        {
            // DRAW CHILD
            drawContentImp();
            pWnd->sectChildEnd();
        }
        setVisible(pWnd->isVisible());
    }
}

}; // namespace qd
