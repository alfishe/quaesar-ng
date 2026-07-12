#pragma once
#include "amDebugger/dbgConnection.h"
#include "qd/base/base.h"
#include "qd/stl/vector.h"
#include "qsr_application.h"


namespace qsr {

//------------------------------------------------------------------------
// Known emulation engines.  The factory registry is string-based (for plugin
// extensibility) but internally we use this enum for type safety.
//------------------------------------------------------------------------
enum class EngineId : uint8_t {
    WinUae,
    VAmiga,
    Unknown,  // returned by engineIdFromStr when no known id matches
};

// String <-> EngineId conversions.  engineIdFromStr is case-insensitive.
const char* engineIdToStr(EngineId id);
EngineId engineIdFromStr(const char* str);


class VmPlayersSelector {
    struct ProviderItem {
        BaseVmServerAppPart* pServerApp = nullptr;
        qtd::string id;
        qtd::string title;
    };
    qtd::vector<ProviderItem> m_vmServerAppParts;

public:
    void init() {
    }

    int activateVmPlayerByIdStr(QuaesarApplication* pApp, const char* vmProviderId);


    qsr::IVmClientPlayer* getVmPlayer(int hIdx) {
        const ProviderItem* provItem = &m_vmServerAppParts[hIdx];
        qsr::IVmClientPlayer* pVmIO = nullptr;
        pVmIO = provItem->pServerApp->getVmPlayer();
        return pVmIO;
    }

    ref_ptr<amD::IVmDbgServiceBridge> createVmDebuggerConnection(int hIdx);
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
