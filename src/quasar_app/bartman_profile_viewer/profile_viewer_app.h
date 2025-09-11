#pragma once
#include "qd/app/applicationPart.h"
#include "qd/typeSystem/typeDeclare.h"


class BarmanProfileViewerAppPart : public qd::ApplicationPart {
    TS_BEGIN_REFLECT_CLASS(BarmanProfileViewerAppPart, qd::ApplicationPart);
    TS_ATTRIBUTE(qd::tsAttr::Name("Bartman's profile viewer"));
    TS_END();

public:
public:
    virtual void onPartCreate(ApplicationPart::OnCreate_t& prm) override {
    }

};  // BarmanProfileViewerAppPart
