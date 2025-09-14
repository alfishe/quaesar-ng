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
    qd::Application* m_pApp = nullptr;
    wref_ptr<qd::UiWindow> m_pModalDlg;

public:
    virtual ~UiDesktop() override;

    virtual void destroy() override;
    void addWindowNode(qd::UiNode* view);
    void showModal(qd::UiWindow* pWnd);

}; // class UiDesktop
//////////////////////////////////////////////////////////////////////////


}; // namespace qd
