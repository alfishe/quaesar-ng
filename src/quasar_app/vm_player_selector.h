#pragma once
#include "qd/base/base.h"
#include "qd/stl/vector.h"
#include "qsr_application.h"


namespace qsr {

class VmPlayersSelector {
    struct ProviderItem {
        BaseVmServerAppPart* pServerApp = nullptr;
        qd::string id;
        qd::string title;
    };
    qd::vector<ProviderItem> m_vmServerAppParts;
    int m_nCurServerFactory = 0;

public:

    void init()
    {
    }

    int activateVmPlayerByIdStr(QuaesarApplication* pApp, const char* vmProviderId);

    qsr::IVmClientPlayer* getVmPlayer(int hIdx) {
        const ProviderItem* provItem = &m_vmServerAppParts[hIdx];
        qsr::IVmClientPlayer* pVmIO = nullptr;
        pVmIO = provItem->pServerApp->getVmPlayer();
        return pVmIO;
    }
};  // class VmPlayersSelector
//////////////////////////////////////////////////////////////////////////


struct ServerAppPartCreateCtx {
    qsr::QuaesarApplication* app = nullptr;
    ref_ptr<qsr::BaseVmServerAppPart> outPartPtr = {};
};

//------------------------------------------------------------------------
class IAppPartServerProviderFactory {
public:
    std::string id = {};
    std::string guiName = {};
    virtual void setup() = 0;
    virtual bool createServerAppPart(qsr::ServerAppPartCreateCtx& prm) = 0;
    virtual ref_ptr<amD::IVmDbgServiceBridge> createVmDebuggerConnection() {
        return nullptr;
    }

    virtual ~IAppPartServerProviderFactory() = default;
};


namespace plugin_api {
struct RegOnLoadAppPartServerFactory {
    RegOnLoadAppPartServerFactory(IAppPartServerProviderFactory* factory);
};
};  //namespace plugin_api


};  // namespace qsr
