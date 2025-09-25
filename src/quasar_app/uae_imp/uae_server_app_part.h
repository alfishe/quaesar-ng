#include "qd/app/applicationPart.h"
#include "qd/qui/uiOperation.h"
#include "qsr_app_interfaces.h"

class UaeServerThread;
FORWARD_DECLARATION_2(IVm, VM);
FORWARD_DECLARATION_2(amD, IVmConnectionBuilder);

namespace qsr {

//------------------------------------------------------------------------
// It's a server app-part that runs the UAE emulator in a separate thread
// and can processes requests to it the Main thread.
//
class UaeServerAppPart : public qd::ApplicationPart {
    TS_BEGIN_REFLECT_CLASS(UaeServerAppPart, qd::ApplicationPart);
    TS_ATTRIBUTE(qd::tsAttr::Name("UAE Server"));
    TS_END();

private:
    UaeServerThread* m_pUaeThread = nullptr;
    ref_ptr<amD::IVmConnectionBuilder> m_pConnBuilder;

public:
    UaeServerAppPart();
    virtual ~UaeServerAppPart() override;

    virtual void onPartCreate(qd::ApplicationPart::OnCreate_t& prm) override;
    virtual void destroyImp() override;

    IVm::VM* getVm() const;

    qsr::IVmServerThread* getUaeThread() const;

};  // class UaeServerAppPart


};  // namespace qsr
