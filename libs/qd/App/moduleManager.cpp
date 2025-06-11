#include <qd/App/appliction.h>
#include <qd/App/moduleManager.h>
#include <qd/Base/unique_ptr.h>
#include <qd/Debug/assert.h>
#include <qd/Debug/exception.h>


namespace qd {
bool ModuleManager::m_bSingleDestroyed = false;
ModuleManager* ModuleManager::m_pSingleInstance = 0;


CModuleInfo* ModuleManager::registerModule(const CModuleInfo& Mod) {
    ECgModuleID nModuleId = Mod.getModuleId();

    if (nModuleId == ECgModuleID::UNKNOWN)
        throw qd::Exception("Module ID is Unknown");

    CModuleInfo* pModule = findModuleInfo(nModuleId);
    if (!pModule) {
        unique_ptr<CModuleInfo> pNewModule = make_unique<CModuleInfo>(nModuleId);
        pModule = pNewModule.get();
        m_pModuleInfoMap[nModuleId] = std::move(pNewModule);

        if ((size_t)nModuleId <= ECgModuleID::_FAST_ACCESS_) {
            m_pFastModules[nModuleId] = pModule;
        }
    }
    // COPY
    pModule->merge(Mod);

    return pModule;
}


CModuleInfo* ModuleManager::findModuleInfo(ECgModuleID ModuleId) const {
    if ((uint32_t)ModuleId < ECgModuleID::_FAST_ACCESS_) {
        CModuleInfo* pModuleInfo = m_pFastModules[ModuleId];
        return pModuleInfo;
    }
    TModuleInfoMap::const_iterator It = m_pModuleInfoMap.find(ModuleId);
    if (It == m_pModuleInfoMap.end())
        return nullptr;
    return It->second.get();
}


CModuleInfo* ModuleManager::getOrCreateModuleInfo(ECgModuleID ModuleId) {
    CModuleInfo* pModuleInfo = findModuleInfo(ModuleId);
    if (!pModuleInfo) {
        pModuleInfo = registerModule(CModuleInfo(ModuleId));
    }
    return pModuleInfo;
}


void ModuleManager::setModuleInstance(ECgModuleID ModuleId, ref_ptr<IModuleInterface> pInstance) {
    CModuleInfo* pModuleInfo = getOrCreateModuleInfo(ModuleId);
    pModuleInfo->setInstance(pInstance);
    return;
}


qd::IModuleInterface* ModuleManager::createModuleInstance(ECgModuleID ModuleId, bool bSingletonInstance,
                                                           qd::ModuleCreateParams* pCreateParam) {
    CModuleInfo* pModuleInfo = getOrCreateModuleInfo(ModuleId);
    if (!pModuleInfo)
        throw Exception("Module Manger can't Create module: %u - module not registered", (uint32_t)ModuleId);

    qd::ModuleCreateParams tmpC;
    if (!pCreateParam)
        pCreateParam = &tmpC;
    pCreateParam->moduleMgr = this;
    if (!pCreateParam->app)
        pCreateParam->app = qd::g_pApp;

    IModuleInterface* pInstance;
    pInstance = pModuleInfo->makeInstance(bSingletonInstance, pCreateParam);
    return pInstance;
}


qd::IModuleInterface* ModuleManager::loadModule(ECgModuleID ModuleId,
                                                 qd::ModuleCreateParams* pCreateParam /*= nullptr*/) {
    CModuleInfo* pModuleInfo = getOrCreateModuleInfo(ModuleId);
    if (!pModuleInfo)
        throw Exception("Module Manager: Can't Destroy Module. Module : %u Not Declare", (uint32_t)ModuleId);

    pModuleInfo->retainInstance();

    if (pModuleInfo->getInstance())
        return pModuleInfo->getInstance();

    return pModuleInfo->makeInstance(true, pCreateParam);
}


void ModuleManager::unloadModule(ECgModuleID ModuleId, bool bIsShutdown) {
    CModuleInfo* pModuleInfo = findModuleInfo(ModuleId);
    if (!pModuleInfo)
        throw Exception("Module Manager: Can't Destroy Module. Module : %u Not Declare", (uint32_t)ModuleId);
    pModuleInfo->releaseInstance();
}


qd::IModuleInterface* ModuleManager::getModuleInstance(ECgModuleID ModuleId, bool bMakeInst /*= true */) {
    CModuleInfo* pModuleInfo = findModuleInfo(ModuleId);
    if (!pModuleInfo)
        throw Exception(EException::NOT_FOUND, "Module:'%u' not registered", (uint32_t)ModuleId);
    IModuleInterface* pExistInst = pModuleInfo->getInstance();
    if (pExistInst || !bMakeInst)
        return pExistInst;
    return this->createModuleInstance(ModuleId, true, nullptr);
}


qd::IModuleInterface* ModuleManager::getOrCreateModule(ECgModuleID ModuleId) {
    CModuleInfo* pModuleInfo = findModuleInfo(ModuleId);
    if (!pModuleInfo)
        throw Exception("Module Manger can't Create module: %u - module not registered", (uint32_t)ModuleId);

    IModuleInterface* pInstance = pModuleInfo->getOrCreateInstance();
    return pInstance;
}


qd::ModuleManager* ModuleManager::I() {
    // PHOENIX SINGLETON
    if (!TThis::m_pSingleInstance && !TThis::m_bSingleDestroyed) {
        //qd::String::gInitStaticData(); // First create CString(W) + Destroy Singleton
        m_pSingleInstance = new TThis();
        ::atexit(&TThis::_destroySingleton);
        m_bSingleDestroyed = false;
    }
    return TThis::m_pSingleInstance;
}


ModuleManager::ModuleManager(void) : m_pFastModules(ECgModuleID::_FAST_ACCESS_) {
    c_def(0);
}


void ModuleManager::cleanUp() {
    // Modules are unloaded in reverse order to when their StartupModule() FINISHES.
    // The practical implication of this is that if module A depends on another module B,
    // and A loads B during A's StartupModule, that B will actually get Unloaded after A during shutdown.
    // This allows A's ShutdownModule() call to still reference module B.

    for (TModuleInfoMap::iterator It = m_pModuleInfoMap.begin(); It != m_pModuleInfoMap.end(); ++It) {
        CModuleInfo* pModuleInfo = It->second.get();
        if (!pModuleInfo)
            continue;
        if (!pModuleInfo->m_pInstance) {
            assert(pModuleInfo->m_nInstanceRef == 0);
            continue;
        }
        pModuleInfo->releaseInstance();  // ONE TIME RELEASE INSTANCE DESTROYS

        if (pModuleInfo->m_pInstance)
            c_def(0);
        // 			pModuleInfo->m_pInstance = nullptr;
        // 			ModuleManager::_ShutDownInstance( pInstance );
        // 			ModuleManager::_DestroyInstance( pInstance );
        c_def(0);
    }
}


ModuleManager::~ModuleManager(void) {
    // Modules are unloaded in reverse order to when their StartupModule() FINISHES.
    // The practical implication of this is that if module A depends on another module B,
    // and A loads B during A's StartupModule, that B will actually get Unloaded after A during shutdown.
    // This allows A's ShutdownModule() call to still reference module B.

    // DESTROY MODULE INFO
    while (!m_pModuleInfoMap.empty()) {
        auto It = m_pModuleInfoMap.rbegin();
        eastl::unique_ptr<CModuleInfo> pModuleInfo = eastl::move(It->second);
        m_pModuleInfoMap.erase(It);
        if (pModuleInfo) {
            //assert2( !pModuleInfo->m_pInstance, "Not deleted module \"%s\" found", CC(pModuleInfo->m_ModuleName) );
            if (pModuleInfo->m_pInstance) {
                // FORCE DESTROY
                ModuleManager::_shutDownInstance(pModuleInfo->m_pInstance);
                ModuleManager::_destroyInstance(pModuleInfo->m_pInstance);
                pModuleInfo->m_pInstance = nullptr;
            }
            //delete pModuleInfo;
            pModuleInfo.release();
        }
    }
}


uint32_t ModuleManager::destroyModule(qd::ECgModuleID ModuleId) {
    CModuleInfo* pModuleInfo = findModuleInfo(ModuleId);
    if (!pModuleInfo) {
        //throw CException( "Module Manager: Can't Destroy Module. Module : %u Not Declare", (uint)ModuleId );
        return 0;
    }
    return pModuleInfo->releaseInstance();
}


CModuleInfo* ModuleManager::overrideModule(ECgModuleID ModuleId, const CModuleInfo::TCreateFunc& pCreateFunc) {
    CModuleInfo* pModuleInfo = findModuleInfo(ModuleId);
    if (!pModuleInfo) {
        CModuleInfo m(ModuleId);
        pModuleInfo = registerModule(m);
    }
    pModuleInfo->m_CreateFunc = pCreateFunc;  // CUSTOM CREATE FUNC
    return pModuleInfo;
}


void CModuleInfo::setInstance(ref_ptr<IModuleInterface> pInstance) {
    if (m_pInstance == pInstance)
        return;
    // 		if ( !pInstance ) {
    // 			ReleaseInstance();
    // 			return;
    // 		}
    if (m_pInstance) {
        assert(0 && "Instance already Set");
        return;
    }
    // 		++ m_nInstanceRef; // REGISTER
    // 		assert(m_nInstanceRef == 1);
    m_pInstance = pInstance;
}


uint32_t CModuleInfo::retainInstance() {
    CModuleInfo* pModuleInfo = this;
    assert(pModuleInfo);
    ++pModuleInfo->m_nInstanceRef;
    return pModuleInfo->m_nInstanceRef;
}


uint32_t CModuleInfo::releaseInstance() {
    CModuleInfo* pModuleInfo = this;
    assert(pModuleInfo);
    if (pModuleInfo->m_nInstanceRef == 0) {
        //pModuleInfo->SetInstance(nullptr);
        pModuleInfo->m_pInstance = nullptr;
        return 0;
    }

    if (--pModuleInfo->m_nInstanceRef == 0) {
        ref_ptr<IModuleInterface> pInstance = pModuleInfo->getInstance();
        if (pInstance) {
            ModuleManager::_shutDownInstance(pInstance);
            ModuleManager::_destroyInstance(pInstance);
            pModuleInfo->m_pInstance = nullptr;
            //pModuleInfo->SetInstance(nullptr);
        }
        return 0;
    }
    return pModuleInfo->m_nInstanceRef;
}


ref_ptr<IModuleInterface> CModuleInfo::makeInstance(bool bRegisterInstance,
                                                    ModuleCreateParams* pCreateParam /*= nullptr*/) {
    TThis* pModuleInfo = this;
    if (!pModuleInfo->m_CreateFunc)
        return nullptr;

    ref_ptr<IModuleInterface> pNewInstance = pModuleInfo->m_CreateFunc(pCreateParam);  // MAKE INSTANCE
    pNewInstance->m_CGModuleTypeID = pModuleInfo->getModuleId();
    if (bRegisterInstance) {
        pModuleInfo->setInstance(pNewInstance);
    }
    ModuleManager::_onStatrupInstance(pNewInstance, pCreateParam);
    return pNewInstance;
}


void CModuleInfo::merge(const CModuleInfo& r) {
    assert(!r.m_pInstance);
    assert(m_ModuleID == r.m_ModuleID);

    if (m_ModuleName.empty())
        m_ModuleName = r.m_ModuleName;

    m_CreateFunc = r.m_CreateFunc;
}


void IModuleInterface::onModuleMessageProc(qd::Enm::EModuleMsg::Msg_t MsgId, void* pMsgData /*= nullptr */) {
    switch (MsgId) {
        case Enm::EModuleMsg::RENDER_IMGUI_DEBUG_INFO_TREE:
#if defined(CGMOD_IMGUI)
        {
            qd::Modules::ImGuiCG::ImGuiDrawForModule(this);
        }
#endif  // CGMOD_IMGUI
        break;
        default:
            break;
    }
}


};  // namespace qd
