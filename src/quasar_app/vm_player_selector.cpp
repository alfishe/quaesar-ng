#include "vm_player_selector.h"
#include "qd/app/appPartsMgr.h"


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

    IAppPartServerProviderFactory* findFactoryByIdStr(const char* id) const {
        for (const auto& factory : m_appPartServerFactoryList) {
            if (factory->id == id)
                return factory.get();
        }
        return nullptr;
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


ref_ptr<amD::IVmDbgServiceBridge> VmPlayersSelector::createVmDebuggerConnection(int hIdx) {
    if (hIdx < 0 || hIdx >= (int)m_vmServerAppParts.size())
        return nullptr;
    const ProviderItem& item = m_vmServerAppParts[hIdx];
    const plugin_api::AppPartServerFactoryListMgr& plugins = plugin_api::AppPartServerFactoryListMgr::get();
    IAppPartServerProviderFactory* pFactory = plugins.findFactoryByIdStr(item.id.c_str());
    if (!pFactory)
        return nullptr;
    return pFactory->createVmDebuggerConnection();
}


};  // namespace qsr
