#pragma once
#include "qd/base/base.h"
#include "qd/node/node.h"
#include "qd/stl/vector.h"
#include "qd/typeSystem/typeDeclare.h"
#include "qd/ui/uiNode.h"



namespace qd {

class UiWindow : public qd::UiNode
{
    TS_BEGIN_REFLECT_CLASS_BASE(100, qd::UiWindow, qd::UiNode);
    TS_END();

    qd::string m_title;

public:
    UiWindow() = default;
    virtual ~UiWindow() = default;

    virtual qd::string getText() const override { return m_title; }
    virtual void setText(const char* pText) { m_title = pText; }
    virtual void draw() override;

}; // class UiWindow


}; // namespace qd
