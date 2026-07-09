#include "vm_player_selector.h"
#include "qd/app/appPartsMgr.h"

#include <strings.h>  // strcasecmp


namespace qsr {

namespace plugin_api {
class AppPartServerFactoryListMgr {
    QD_SINGLETON_DECLARE(AppPartServerFactoryListMgr);

public:
    qtd::vector<std::unique_ptr<IAppPartServerProviderFactory>> m_appPartServerFactoryList;

public:
    AppPartServerFactoryListMgr() = default;
    ~AppPartServerFactoryListMgr() {
        m_appPartServerFactoryList.clear();
    }

    void addFactory(IAppPartServerProviderFactory* factory) {
        factory->setup();
        m_appPartServerFactoryList.push_back(std::unique_ptr<IAppPartServerProviderFactory>(factory));
    }

    // Case-insensitive factory lookup so "vAmiga", "VAMIGA", "vamiga" all match.
    IAppPartServerProviderFactory* findFactoryByIdStr(const char* id) const {
        for (const auto& factory : m_appPartServerFactoryList) {
            if (factory->id.size() == strlen(id) &&
                strcasecmp(factory->id.c_str(), id) == 0)
                return factory.get();
        }
        return nullptr;
    }

    // Read-only access to all registered factories (for UI enumeration).
    const qtd::vector<std::unique_ptr<IAppPartServerProviderFactory>>& getFactories() const {
        return m_appPartServerFactoryList;
    }

};  // class AppPartServerFactoryListMgr


RegOnLoadAppPartServerFactory::RegOnLoadAppPartServerFactory(IAppPartServerProviderFactory* factory) {
    AppPartServerFactoryListMgr::get().addFactory(factory);
}
};  //namespace plugin_api
    //////////////////////////////////////////////////////////////////////////


int VmPlayersSelector::activateVmPlayerByIdStr(QuaesarApplication* pApp, const char* vmProviderId) {
    //vmProviderId = "uae";
    //vmProviderId = "vamiga";

    const plugin_api::AppPartServerFactoryListMgr& appPartsPlugins = plugin_api::AppPartServerFactoryListMgr::get();
    IAppPartServerProviderFactory* pFactory = appPartsPlugins.findFactoryByIdStr(vmProviderId);
    assert(pFactory);
    if (!pFactory)
        return -1;

    ServerAppPartCreateCtx ctx;
    ctx.app = pApp;
    if (pFactory->createServerAppPart(ctx)) {
        qsr::BaseVmServerAppPart* pPart = ctx.outPartPtr;

        m_vmServerAppParts.emplace_back();
        ProviderItem& item = m_vmServerAppParts.back();
        item.pServerApp = pPart;
        item.title = pFactory->guiName.c_str();
        item.id = pFactory->id.c_str();

        qd::ApplicationPart::OnCreate_t prm;
        prm.app = ctx.app;
        prm.name = pFactory->id.c_str();
        pPart->onPartCreate(prm);
        pApp->getAppParts()->addPart(ctx.outPartPtr);

        return (int)m_vmServerAppParts.size() - 1;
    }
    return -1;
}


//------------------------------------------------------------------------
// EngineId conversion helpers
//------------------------------------------------------------------------
const char* engineIdToStr(EngineId id) {
    switch (id) {
        case EngineId::WinUae: return "uae";
        case EngineId::VAmiga: return "vamiga";
        default:               return nullptr;
    }
}

EngineId engineIdFromStr(const char* str) {
    if (!str || !*str)
        return EngineId::Unknown;
    for (const auto& factory : plugin_api::AppPartServerFactoryListMgr::get().m_appPartServerFactoryList) {
        if (factory->id.size() == strlen(str) &&
            strcasecmp(factory->id.c_str(), str) == 0) {
            if (factory->id == "uae")    return EngineId::WinUae;
            if (factory->id == "vamiga") return EngineId::VAmiga;
        }
    }
    return EngineId::Unknown;
}


};  // namespace qsr
