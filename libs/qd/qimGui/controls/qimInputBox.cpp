#include "qimInputBox.h"


void qim::InputInt::onDrawEndImp(qim::Context* ctx)
{
    StepInt defaultStep;
    auto* stepPrm = propFind_<StepInt>();
    if (!stepPrm)
        stepPrm = &defaultStep;

    int val = 0;
    im.m_valStorage.call(&val, nullptr); // get val

    m_bTextChanged = ImGui::InputInt(im.m_label, &val, stepPrm->m_step, stepPrm->m_stepFast);

    if (m_bTextChanged)
        im.m_valStorage.call(nullptr, &val); // set val via callback

    im = {};
}
