#include "qd/app/applicationPart.h"
#include "qd/qui/uiOperation.h"
#include "quasar_app/qsr_application.h"


FORWARD_DECLARATION_2(qsr, IVmServerThread);
FORWARD_DECLARATION_2(IVm, VM);
FORWARD_DECLARATION_2(amD, IVmConnectionBuilder);
class VAmServerThread;


namespace qsr {

//------------------------------------------------------------------------
// It's a server app-part that runs the VAMIGA emulator in a separate thread
// and can processes requests to it the Main thread.
//
class VAmServerAppPart : public qsr::BaseVmServerAppPart {
    TS_BEGIN_REFLECT_CLASS(VAmServerAppPart, qsr::BaseVmServerAppPart);
    TS_ATTRIBUTE(qd::tsAttr::Name("VAMIGA Server"));
    TS_END();

private:
    VAmServerThread* m_pVAmThread = nullptr;
    ref_ptr<amD::IVmConnectionBuilder> m_pConnBuilder;

public:
    VAmServerAppPart();
    virtual ~VAmServerAppPart() override;

    virtual void onPartCreate(qd::ApplicationPart::OnCreate_t& prm) override;
    virtual void destroyImp() override;

    IVm::VM* getVm() const;
    virtual qsr::IVmServerThread *getServerThread() override;

};  // class VAmServerAppPart


};  // namespace qsr
