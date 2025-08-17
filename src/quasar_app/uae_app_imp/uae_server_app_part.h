#include "qd/app/appPart.h"
#include "qd/qui/uiOperation.h"

class UaeServerThread;
FORWARD_DECLARATION_2(IVm, VM);

namespace qsr {

class UaeServerAppPart : public qd::AppPart, public qd::IOperationEnvironment {
    TS_BEGIN_REFLECT_CLASS(UaeServerAppPart, qd::AppPart);
    TS_ATTRIBUTE(qd::tsAttr::Name("UAE Server"));
    TS_END();

private:
    ref_ptr<UaeServerThread> m_pUaeThread;

public:
    UaeServerAppPart();
    virtual ~UaeServerAppPart() override;

    virtual void onPartCreate(qd::AppPart::OnCreate_t& prm) override;
    virtual void destroyImp() override;

    IVm::VM* getVm() const;

    UaeServerThread* getUaeThread() const;
};  // class UaeServerAppPart


};  // namespace qsr
