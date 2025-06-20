#pragma once
#include "qd/qimGui/qimGui.h"




namespace qim {
class UiInputBeh;


struct InputScalar : public qim::CtrlElement {
    QIM_ELEMENT_CLASS(qim::InputScalar, qim::CtrlElement, qim::UiInputBeh);


    void setup(const char* text)
    { //
    }

    virtual bool isTextChanged() const
    {
        return false;
    }

    //     virtual void onBegin(qim::Context* ctx) override;
    //     virtual void onEnd(qim::Context* ctx) override;

}; // struct
//////////////////////////////////////////////////////////////////////////


struct InputInt : public qim::InputScalar {
    QIM_ELEMENT_CLASS(qim::InputInt, qim::InputScalar, qim::UiInputBeh);

    using StepInt = Props::StepInt;

    struct Imm {
        const char* m_label = nullptr;
        int* m_pVal = nullptr;
    } im;

    void setup(const char* text, int* p_val)
    {
        im.m_label = text;
        im.m_pVal = p_val;
    }

    virtual void onBeginImp(qim::Context* ctx) override {}

    virtual void onEndImp(qim::Context* ctx) override
    {
        auto& stepPrm = propAdd_<StepInt>();
        ImGui::InputInt(im.m_label, im.m_pVal, stepPrm.m_step, stepPrm.m_stepFast);
    }

}; // struct
//////////////////////////////////////////////////////////////////////////



class UiInputBeh : public qim::BehaviorElem
{
    TS_BEGIN_REFLECT_CLASS(qim::UiInputBeh, qim::BehaviorElem);
    TS_ATTRIBUTE(qd::tsAttr::CreateClassCb(&qim::createElemBehCb_<TRefClass>));
    TS_END();


public:
    Element* createElementData(const qd::TypeInfo& type) override { return new qim::InputInt(); }

}; // class UiInputBeh
//////////////////////////////////////////////////////////////////////////



}; // namespace qim
