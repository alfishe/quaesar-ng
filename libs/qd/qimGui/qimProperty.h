#pragma once
#include "qimBase.h"
#include "qd/base/ref_ptr.h"
#include "qd/mem/fnvHash.h"


namespace qim {


class Property : public qd::RefCounted
{
public:
    virtual uint32_t getCID() const = 0;
};


#define QIM_PROPERTY(PropName)                                \
    using TThis = PropName;                                   \
    static constexpr uint32_t CID = qd::fnv1aHash(#PropName); \
    virtual uint32_t getCID() const override                  \
    {                                                         \
        return CID;                                           \
    }



namespace Props {

struct Color : public qim::Property {
    QIM_PROPERTY(Color);
    qd::Color color = qd::Color::WHITE;
    TThis& set(const qd::Color& in)
    {
        color = in;
        return *this;
    }
};

struct Text : public qim::Property {
    QIM_PROPERTY(Text);
    qd::string text;
};

struct Size : public qim::Property {
    QIM_PROPERTY(Size);
    qd::Size m_size = {};
    TThis& set(const qd::Size& in)
    {
        m_size = in;
        return *this;
    }
    bool isSizeValid() const
    {
        return m_size.isValid();
    }
};


struct StepInt : public qim::Property {
    QIM_PROPERTY(StepInt);
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



}; // namespace qim
