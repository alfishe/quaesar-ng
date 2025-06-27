#pragma once
#include "qd/base/ref_ptr.h"
#include "qd/mem/fnvHash.h"
#include "qd/qimGui/qimBase.h"


namespace qim {


namespace Props {

struct Color : public qim::Property {
    QIM_PROPERTY(qim::Props::Color, qim::Property, qim::EVisitStage::VProperty);
    qd::Color color = qd::Color::WHITE;
    TThis& set(const qd::Color& in)
    {
        color = in;
        return *this;
    }
};

struct Text : public qim::Property {
    QIM_PROPERTY(qim::Props::Text, qim::Property, qim::EVisitStage::VProperty);
    qd::string text;
};

struct Size : public qim::Property {
    QIM_PROPERTY(qim::Props::Size, qim::Property, qim::EVisitStage::VProperty);
    qd::Size m_size = {};
    TThis& set(const qd::Size& in)
    {
        m_size = in;
        return *this;
    }
    bool isSizeValid() const { return m_size.isValid(); }
};


struct StepInt : public qim::Property {
    QIM_PROPERTY(qim::Props::StepInt, qim::Property, qim::EVisitStage::VProperty);
    int m_step = 1;
    int m_stepFast = 100;
    TThis& step(int in_val)
    {
        m_step = in_val;
        return *this;
    }
    TThis& stepFast(int in_val)
    {
        m_stepFast = in_val;
        return *this;
    }
};


}; // namespace Props
//////////////////////////////////////////////////////////////////////////


namespace Sect {


struct ChildList : public qim::Section {
    QIM_PROPERTY(qim::Sect::ChildList, qim::Property, qim::EVisitStage::VChild);
};

struct IsClicked : public qim::Section {
    QIM_PROPERTY(qim::Sect::IsClicked, qim::Property, qim::EVisitStage::VEventHandler);

    qd::Tribool m_hasEvent;

    virtual bool isSectEnterAllowedImp(qim::Context* ctx, ElementData* pData) override
    {
        if (m_hasEvent.maybeFalse())
            return false;
        return true;
    }

    void onClick(int mb)
    {
        m_hasEvent = qd::Tribool::True;
    }

};


}; // namespace Sect


}; // namespace qim
