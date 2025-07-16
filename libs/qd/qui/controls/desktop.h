#pragma once
#include "qd/qui/uiNode.h"
#include "qd/qui/controls/window.h"


FORWARD_DECLARATION_2(qd, Application);


namespace qd {
class UiWindow;


class UiDesktop : public qd::UiNode
{
    TS_REFLECT_CLASS(qd::UiDesktop, qd::UiNode);

protected:
    qd::vector<ref_ptr<qd::UiNode>> m_pWindows;
    qd::Application* mApp = nullptr;
    qd::UiWindow* m_pModalDlg = nullptr;

public:
    virtual ~UiDesktop();

    void destroy();
    void addView(qd::UiNode* view);
    void showModal(qd::UiWindow* pWnd);

}; // class UiDesktop


}; // namespace qd
