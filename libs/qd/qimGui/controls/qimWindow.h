#pragma once
#include "qd/qimGui/qimGui.h"



namespace qim {
class WindowBeh;


struct Window : public qim::CtrlElement {
    QIM_ELEMENT_CLASS(qim::Window, qim::CtrlElement, qim::WindowBeh);
    using Size = qim::Props::Size;
    bool m_isModal = false;
    qd::string m_title;

    struct Im
    {
        bool retVis = false;
    } im;

public:

    void setup(const char* text)
    { //
        m_title = text;
    }

    bool isModal() const { return m_isModal; }
    void setModal(bool Modal) { m_isModal = Modal; }

    virtual bool onSectChildBeginImp() override;

    virtual void onSectChildEndImp() override;

}; // struct
//////////////////////////////////////////////////////////////////////////



class WindowBeh : public qim::BehaviorElem
{
    TS_BEGIN_REFLECT_CLASS(qim::WindowBeh, qim::BehaviorElem);
    TS_ATTRIBUTE(qd::tsAttr::CreateClassCb(&qim::createElemBehCb_<TRefClass>));
    TS_END();


public:
    Behavior* createElementData(const qd::TypeInfo& type) override { return new qim::Window(); }

}; // class UiInputBeh
//////////////////////////////////////////////////////////////////////////


}; // namespace qim
