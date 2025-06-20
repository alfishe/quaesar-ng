#include "desktop.h"
#include "qd/ui/uiNode.h"
#include "qd/ui/controls/window.h"


qd::UiDesktop::~UiDesktop()
{
    assert(m_pWindows.empty());
}


void qd::UiDesktop::destroy()
{
    TSuper::destroy();

    while (!m_pWindows.empty())
    {
        ref_ptr<qd::UiNode> curWnd = m_pWindows.back();
        m_pWindows.pop_back();
        curWnd->destroy();
        curWnd.reset();
        // delete curWnd;
    }
}


void qd::UiDesktop::addView(qd::UiNode* view)
{
    addChild(view);

    m_pWindows.push_back(view);
}


void qd::UiDesktop::showModal(qd::UiWindow* pWnd)
{
    ASSERT_AND_DO(pWnd, return, "No wnd");

    pWnd->setModal(true);
    pWnd->setVisible(true);

    m_pModalDlg = pWnd;
}
