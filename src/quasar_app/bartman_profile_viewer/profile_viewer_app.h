#pragma once
#include "qd/app/appPart.h"
#include "qd/typeSystem/typeDeclare.h"


class BarmanProfileViewerAppPart : public qd::AppPart {
    TS_BEGIN_REFLECT_CLASS(BarmanProfileViewerAppPart, qd::AppPart);
    TS_ATTRIBUTE(qd::tsAttr::Name("Bartman's profile viewer"));
    TS_END();

public:
public:
    virtual void onPartCreate(AppPart::OnCreate_t& prm) override {
    }

};  // BarmanProfileViewerAppPart
