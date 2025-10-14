#include "qd/app/applicationPart.h"
#include "qd/qui/uiOperation.h"
#include "qsr_app_interfaces.h"
#include "qsr_application.h"

class UaeServerThread;
FORWARD_DECLARATION_2(IVm, VM);
FORWARD_DECLARATION_2(amD, IVmConnectionBuilder);


namespace qsr {

//------------------------------------------------------------------------
// It's a server app-part that runs the UAE emulator in a separate thread
// and can processes requests to it the Main thread.
//
class UaeServerAppPart : public qsr::BaseVmServerAppPart {
    TS_BEGIN_REFLECT_CLASS(UaeServerAppPart, qsr::BaseVmServerAppPart);
    TS_ATTRIBUTE(qd::tsAttr::Name("UAE Server"));
    TS_END();

private:
    UaeServerThread* m_pUaeThread = nullptr;
    //ref_ptr<amD::IVmConnectionBuilder> m_pConnBuilder;
    int m_vmActive = -1;

public:
    UaeServerAppPart();
    virtual ~UaeServerAppPart() override;

    virtual void onPartCreate(qd::ApplicationPart::OnCreate_t& prm) override;

    void createUaeThread();
    virtual void destroyImp() override;

    IVm::VM* getVm() const;

    virtual qsr::IVmServerThread* getServerThread() override;

    virtual void update(float, float) override;

    void setVmActive(int v) {
        m_vmActive = v;
    }
};  // class UaeServerAppPart


};  // namespace qsr
