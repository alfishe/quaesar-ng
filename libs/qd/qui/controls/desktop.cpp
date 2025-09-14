#include "desktop.h"


qd::UiDesktop::~UiDesktop()
{
    assert(m_pChilds.empty());
}


void qd::UiDesktop::destroy()
{
    m_pModalDlg = nullptr;

    TSuper::destroy();
}


void qd::UiDesktop::addWindowNode(qd::UiNode* view)
{
    addChild(view);
}


void qd::UiDesktop::showModal(qd::UiWindow* pWnd)
{
    ASSERT_AND_DO(pWnd, return, "No wnd");

    pWnd->setModal(true);
    pWnd->setVisible(true);

    m_pModalDlg = pWnd;
}
