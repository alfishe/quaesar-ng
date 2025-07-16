#pragma once
#include "qd/qui/uiNode.h"
#include <EASTL/functional.h>


namespace qd {

class UiLambda : public qd::UiNode
{
    TS_REFLECT_CLASS(qd::UiLambda, qd::UiNode);

    eastl::function<void()> m_Callback;

public:

    void setup(eastl::function<void()> callback) { m_Callback = callback; }


    virtual void drawContentImp() override
    {
        m_Callback();
    }


}; // class
//////////////////////////////////////////////////////////////////////////

};
