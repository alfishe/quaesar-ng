#pragma once
#include "qd/math/point2.h"
#include "qd/stl/vector.h"
#include "qd/qui/uiNode.h"



namespace qd {

class UiWindow : public qd::UiNode
{
    TS_BEGIN_REFLECT_CLASS_BASE(100, qd::UiWindow, qd::UiNode);
    TS_END();

    qd::string m_title;
    qd::Size m_size = {-1, -1};
    bool m_bModal = false;

public:
    UiWindow() = default;
    virtual ~UiWindow() = default;

    bool isModal() const { return m_bModal; }
    void setModal(bool Modal) { m_bModal = Modal; }

    virtual qd::string getText() const override { return m_title; }
    virtual void setText(const char* pText) { m_title = pText; }
    virtual void drawImp() override;

    qd::Size getSize() const {
        return m_size;
    }
    void setSize(const qd::Size& Size) {
        m_size = Size;
    }
}; // class UiWindow


}; // namespace qd
