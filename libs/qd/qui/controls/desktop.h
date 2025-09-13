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
    qd::Application* m_pApp = nullptr;
    qd::UiWindow* m_pModalDlg = nullptr;

public:
    virtual ~UiDesktop() override;

    virtual void destroy() override;
    void addWindowNode(qd::UiNode* view);
    void showModal(qd::UiWindow* pWnd);

    template<class TWnd>
    TWnd* findChildByType_() const
    {
        const qd::TypeInfo& wndType = qd::typeof_<TWnd>();
        for (const ref_ptr<qd::UiNode>& pWnd : m_pWindows)
        {
            if (!pWnd)
                continue;
            const qd::TypeInfo& classType = pWnd->getTypeInfo();
            if (classType.isDerivedFrom(wndType))
                return static_cast<TWnd*>(pWnd.get());
        }
        return nullptr;
    }

    template<class T>
    inline T* getWnd_() const
    {
        const uint32_t idx = T::CLASS_ID;
        qd::UiNode* curView = m_pWindows[idx];
        return static_cast<T*>(curView);
    }


}; // class UiDesktop


}; // namespace qd
