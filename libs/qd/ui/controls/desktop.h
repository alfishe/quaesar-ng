#pragma once
#include "qd/stl/vector.h"
#include "qd/base/base.h"
#include "qd/node/node.h"
#include "qd/typeSystem/typeDeclare.h"
#include "qd/ui/uiNode.h"


FORWARD_DECLARATION_2(qd, Application);


namespace qd {

	class UiDesktop : public qd::UiNode {
        TS_REFLECT_CLASS(qd::UiDesktop, qd::UiNode);

    protected:
        qd::vector<ref_ptr<qd::UiNode>> m_pWindows;
	    qd::Application* mApp = nullptr;
        qd::UiNode* m_pModalDlg = nullptr;

	public:
        virtual ~UiDesktop();

        void destroy();

        void addView(qd::UiNode* view);


        void showModal(qd::UiNode* pWnd);

	};  // class UiDesktop


};  // namespace qd
