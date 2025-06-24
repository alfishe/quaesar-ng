#pragma once
#include "EASTL/fixed_function.h"
#include "qd/qimGui/qimGui.h"


FORWARD_DECLARATION_2(qd, UiOperation);


namespace qim {
class UiMenuBeh;



struct UiMenu : public qim::CtrlElement {
    QIM_ELEMENT_CLASS(qim::UiMenu, qim::CtrlElement, qim::UiMenuBeh);

    qd::string m_text;
    bool m_isOpen = false;

    void setup(const char* text)
    {
        m_text = text;
        assert(!m_text.empty());
    }

    virtual void onBeginImp(qim::Context* ctx) override;
    virtual void onEndImp(qim::Context* ctx) override;

    bool isOpen() const { return m_isOpen; }
    void setText(const char* text) { m_text = text; }
    const char* getText() const { return m_text.c_str(); }


}; // struct UiMenu
//////////////////////////////////////////////////////////////////////////



struct UiMenuItem : public qim::CtrlElement {
    QIM_ELEMENT_CLASS(qim::UiMenuItem, qim::CtrlElement, qim::UiMenuBeh);
private:
    //eastl::fixed_function<16, void()> m_onClickCb;
    qd::string m_text;

public:
    void setup(const char* text)
    {
        m_text = text;
        assert(!m_text.empty());
    }

    virtual void onEndImp(qim::Context* ctx) override;

    void setText(const char* text) { m_text = text; }
    const char* getText() const { return m_text.c_str(); }

}; // struct UiMenuItem
//////////////////////////////////////////////////////////////////////////



class UiMenuBeh : public qim::BehaviorElem
{
    TS_BEGIN_REFLECT_CLASS(qim::UiMenuBeh, qim::BehaviorElem);
    TS_ATTRIBUTE(qd::tsAttr::CreateClassCb(&qim::createElemBehCb_<TRefClass>));
    TS_END();


public:
    Element* createElementData(const qd::TypeInfo& type) override;

}; // class UiMenuBeh
//////////////////////////////////////////////////////////////////////////


class UiMenuOperationBeh;



struct UiMenuOperation : public qim::CtrlElement {
    QIM_ELEMENT_CLASS(qim::UiMenuOperation, qim::CtrlElement, qim::UiMenuOperationBeh);

    void setup(const char* operation_class_name);
    virtual void onEndImp(qim::Context* ctx) override;

private:
    qd::UiOperation* m_pOperation = nullptr;
    const qd::TypeInfo* m_pOperationType = nullptr;

}; // struct UiMenuOperation



class UiMenuOperationBeh : public qim::BehaviorElem
{
    TS_BEGIN_REFLECT_CLASS(qim::UiMenuOperationBeh, qim::BehaviorElem);
    TS_ATTRIBUTE(qd::tsAttr::CreateClassCb(&qim::createElemBehCb_<TRefClass>));
    TS_END();

public:
    Element* createElementData(const qd::TypeInfo& type) override { return new UiMenuOperation(); }

}; // class UiMenuOperationBeh


}; // namespace qim
