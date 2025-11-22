#pragma once
#include "qd/app/moduleBase.h"
#include "qd/base/base.h"
#include "qd/stl/fixed_vector.h"
#include "qd/stl/ref_ptr.h"
#include "qd/stl/string.h"
#include "qd/stl/unique_ptr.h"
#include "qd/stl/vector_map.h"
#include <EASTL/fixed_function.h>


/* Declares struct that register class module creation */
#define QD_MODULE_REGISTRATION(ClassName) \
    inline static qd::ModuleRegistrator_<ClassName> PASTE(gModuleImp_, __COUNTER__)(/**/ STRINGIFY(ClassName));
#define QD_MODULE_REGISTRATION_NOCREATE(ClassName) \
    inline static qd::ModuleRegistratorNoCreate_<ClassName> PASTE(gModuleImp_, __COUNTER__)(/**/ STRINGIFY(ClassName));


namespace qd {


//////////////////////////////////////////////////////////////////////////
class ModuleInfo
{
    typedef ModuleInfo TThis;
    friend class ModuleManager;

public:
    const qd::TypeInfo* m_ModuleId;
    IModuleInterface* m_pInstance;
    eastl::fixed_function<8, IModuleInterface*(const qd::ModuleCreateParams&)> m_CreateFunc;
    typedef eastl::fixed_function<8, IModuleInterface*(const qd::ModuleCreateParams&)> TCreateFunc;
    uint32_t m_nInstanceRef;
    qd::string m_ModuleName;

public:
    ModuleInfo(const qd::TypeInfo& pModuleInfo)
        : m_ModuleId(&pModuleInfo)
        , m_pInstance(nullptr)
        , m_nInstanceRef(0) {}

    IModuleInterface* getInstance() const { return m_pInstance; }
    void setInstance(IModuleInterface* Instance);

    uint32_t retainInstance();
    uint32_t releaseInstance();
    void merge(const ModuleInfo& r);

    IModuleInterface* makeInstance(bool bRegisterSingleton, const qd::ModuleCreateParams& createParam);
    const qd::TypeInfo& getModuleId() const { return *m_ModuleId; }

    // MAKES MODULE INSTANCE
    template<class TModuleClass>
    TModuleClass* makeInstance_(bool bRegisterSingleton, const qd::ModuleCreateParams& mc);

}; // class ModuleInfo
//////////////////////////////////////////////////////////////////////////



template<class TModule>
struct ModuleRegistrator_ {

    typedef ModuleRegistrator_<TModule> TThis;

public:
    inline ModuleRegistrator_(const char* class_name); // CONSTRUCTOR TO REGISTER THIS STRUCT IN #ModuleManager
    static qd::IModuleInterface* createModuleFunc(const qd::ModuleCreateParams& mc);

}; // struct ModuleRegistrator_
//////////////////////////////////////////////////////////////////////////


template<class TModule>
struct ModuleRegistratorNoCreate_ {
    typedef ModuleRegistratorNoCreate_<TModule> TThis;
    inline ModuleRegistratorNoCreate_(const char* pClassName); // CONSTRUCTOR TO REGISTER THIS STRUCT IN #ModuleManager

    static qd::IModuleInterface* createModuleFunc(const qd::ModuleCreateParams&) { return nullptr; }

}; // struct ModuleRegistrator_
//////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////
// #ModuleManager
class ModuleManager
{
    typedef ModuleManager TThis;

    struct InfoItem {
        const qd::TypeInfo* m_pType = nullptr;
        qd::unique_ptr<ModuleInfo> m_pModuleInfo;
    };
    qd::vector<ModuleManager::InfoItem> m_pModuleInfoMap;

public:
    typedef qd::vector<ModuleManager::InfoItem> TModuleInfoMap;

private:
    static TThis* m_pSingleInstance;
    static bool m_bSingleDestroyed;

public:
    qd::ModuleCreateParams m_defaultCreateParam;

public:
    static ModuleManager* I();

    static inline ModuleManager* get() { return TThis::I(); }

    static void _destroySingleton() {
        SAFE_DELETE(TThis::m_pSingleInstance);
        TThis::m_bSingleDestroyed = true;
    }

public:
    ModuleManager(void);

    void cleanUp();

    virtual ~ModuleManager(void);


    const ModuleManager::TModuleInfoMap& getModuleInfoMap() const { return m_pModuleInfoMap; }


    ModuleInfo* overrideModule(const qd::TypeInfo& ModuleId, const ModuleInfo::TCreateFunc& createFunc);

    // Default create function
    template<class TModuleClass>
    inline ModuleInfo* overrideModule_(const ModuleInfo::TCreateFunc& pCreateFunc = {});

    ModuleInfo* registerModule(const ModuleInfo& i);


    ModuleInfo* findModuleInfo(const qd::TypeInfo& ModuleId) const;

    ModuleInfo* getOrCreateModuleInfo(const qd::TypeInfo& ModuleId);

    void setModuleInstance(const qd::TypeInfo& ModuleId, IModuleInterface* pInstance);

    template<class TModuleClass>
    inline void setModuleInstance_(TModuleClass* pInstance) {
        const qd::TypeInfo& moduleId = &TModuleClass::getStaticTypeInfo();
        setModuleInstance(moduleId, pInstance);
    }


    qd::IModuleInterface* getOrCreateModule(const qd::TypeInfo& ModuleId);

    qd::IModuleInterface* createModuleInstance(const qd::TypeInfo& ModuleId, bool bSingletonInstance,
        qd::ModuleCreateParams* pCreateParam = nullptr);


    // JUST CREATES MODULE INSTANCE WITHOUT STORE IT IN MODULE_INFO
    template<class TModuleClass>
    inline ref_ptr<TModuleClass> makeInstance_(qd::ModuleCreateParams* mc = nullptr) {
        const qd::TypeInfo& moduleId = &TModuleClass::getStaticTypeInfo();
        ref_ptr<TModuleClass> pInstance = createModuleInstance(moduleId, /*regInstance:*/ false, mc);
        return pInstance;
    }


    IModuleInterface* findModuleInstance(const qd::TypeInfo& ModuleId) {
        ModuleInfo* pModuleInfo = findModuleInfo(ModuleId);
        if (!pModuleInfo)
            return nullptr;
        IModuleInterface* pCurModule = pModuleInfo->getInstance();
        return pCurModule;
    }

    template<class TModuleClass>
    TModuleClass* findInstacnce_() {
        const qd::TypeInfo& ModuleId = TModuleClass::getStaticTypeInfo();
        ptr<TModuleClass> pInstance = findModuleInstance(ModuleId);
        return pInstance;
    }

    // RETAINS MODULE
    IModuleInterface* loadModule(const qd::TypeInfo& ModuleId, qd::ModuleCreateParams* pCreateParam = nullptr);


    template<class TModuleClass>
    inline TModuleClass* loadModule_(qd::ModuleCreateParams* pCreateParam = nullptr) {
        const qd::TypeInfo& ModuleId = TModuleClass::getStaticTypeInfo();
        ptr<TModuleClass> pInstance = loadModule(ModuleId, pCreateParam);
        return pInstance;
    }

    // Modules are unloaded in reverse order to when their StartupModule() FINISHES.
    // The practical implication of this is that if module A depends on another module B,
    // and A loads B during A's StartupModule, that B will actually get Unloaded after A during shutdown.
    // This allows A's ShutdownModule() call to still reference module B.
    // RELEASE MODULE
    void unloadModule(const qd::TypeInfo& ModuleId, bool bIsShutdown = false);

    template<class TModuleClass>
    inline void unloadModule_() {
        const qd::TypeInfo& ModuleId = TModuleClass::getStaticTypeInfo();
        unloadModule(ModuleId);
    }

    IModuleInterface* getModuleInstance(const qd::TypeInfo& ModuleId, bool bMakeInst = false);


    template<class TModule>
    inline TModule* getModuleInst_() {
        TModule* pExistInst = static_cast<TModule*>(getModuleInstance(TModule::getStaticTypeInfo(), false));
        return pExistInst;
    }


    template<class TModule>
    TModule* getModuleInstOrCreate_() {
        TModule* pExistInst = static_cast<TModule*>(getModuleInstance(TModule::getStaticTypeInfo(), /*MakeInst*/ false));
        if (pExistInst)
            return pExistInst;
        pExistInst = static_cast<TModule*>(createModuleInstance(TModule::getStaticTypeInfo(), /*keepInstance:*/ true));
        return pExistInst;
    }


    bool isModuleRegistered(const qd::TypeInfo& moduleId) const;

    template<class TModuleClass>
    inline bool isModuleRegistered_() const {
        const qd::TypeInfo& ModuleId = TModuleClass::getStaticTypeInfo();
        return isModuleRegistered(ModuleId);
    }


    bool isModuleLoaded(const qd::TypeInfo& ModuleId) const;

    template<class TModuleClass>
    inline bool isModuleLoaded_() const {
        const qd::TypeInfo& moduleId = TModuleClass::getStaticTypeInfo();
        return isModuleLoaded(moduleId);
    }

    uint32_t destroyModule(const qd::TypeInfo& moduleId);
    void destroyModule(const qd::TypeInfo& moduleId, IModuleInterface* pInstance);


    static void _onStatrupInstance(IModuleInterface* pModule, const qd::ModuleCreateParams& mc) {
        if (!pModule->m_ModuleState.m_bModStartuped && !pModule->m_ModuleState.m_bModShutdowned) {
            pModule->m_ModuleState.m_bModStartuped = true;
            pModule->onModuleStartup(mc);
        }
    }

    static void _shutDownInstance(IModuleInterface* pModule) {
        if (pModule && pModule->m_ModuleState.m_bModStartuped && !pModule->m_ModuleState.m_bModShutdowned) {
            pModule->m_ModuleState.m_bModShutdowned = true;
            pModule->onModuleShutdown();
            c_def(0);
        }
    }

    static void _destroyInstance(IModuleInterface* pInstance) {
        if (pInstance && !pInstance->m_ModuleState.m_bModDestroyed) {
            pInstance->m_ModuleState.m_bModDestroyed = true;
            pInstance->destroyModule();
            c_def(0);
        }
    }


}; // class ModuleManager
//////////////////////////////////////////////////////////////////////////




namespace Modules {

inline ModuleManager* get() {
    return ModuleManager::I();
}

template<class TModule>
inline TModule* getModuleInst_() {
    ModuleManager* pMgr = ModuleManager::get();
    TModule* pExistInst = pMgr->getModuleInst_<TModule>(false);
    return pExistInst;
}

}; // namespace Modules


template<class TModuleClass>
inline ModuleInfo* ModuleManager::overrideModule_(const ModuleInfo::TCreateFunc& pCreateFunc /*= CCallbackNull() */) {

    const qd::TypeInfo& moduleId = TModuleClass::getModuleTypeId();
    if (pCreateFunc) {
        // CUSTOM CREATE FUNC
        return overrideModule(moduleId, pCreateFunc);
    }
    else {
        // DEFAULT CREATE FUNC
        return overrideModule(moduleId, BIND_FREE_CB(&ModuleRegistrator_<TModuleClass>::createModuleFunc));
    }
}


template<class TModuleClass>
qd::IModuleInterface* ModuleRegistrator_<TModuleClass>::createModuleFunc(const qd::ModuleCreateParams& cp) {
    TModuleClass* pMod = new TModuleClass(cp);
    return pMod;
}


template<class TModule>
inline ModuleRegistrator_<TModule>::ModuleRegistrator_(const char* class_name) {

    const qd::TypeInfo& moduleId = TModule::getStaticTypeInfo();
    ModuleInfo* pMtd = ModuleManager::I()->overrideModule(moduleId, &TThis::createModuleFunc);
    pMtd->m_ModuleName = class_name;
}


template<class TModule>
inline ModuleRegistratorNoCreate_<TModule>::ModuleRegistratorNoCreate_(const char* pClassName) {

    const qd::TypeInfo& moduleId = TModule::getStaticTypeInfo();
    ModuleInfo* pMtd = ModuleManager::I()->overrideModule(moduleId, &TThis::createModuleFunc); // NO CREATE
    pMtd->m_ModuleName = pClassName;
}


template<class TModuleClass>
TModuleClass* ModuleInfo::makeInstance_(bool bRegisterSingleton, const qd::ModuleCreateParams& mc) {

    TThis* pModuleInfo = this;
    ref_ptr<TModuleClass> pNewInstance = new TModuleClass(mc); // MAKE INSTANCE
    pNewInstance->m_CGModuleTypeID = pModuleInfo->getModuleId();
    ModuleManager::_onStatrupInstance(pNewInstance, mc);
    if (bRegisterSingleton)
        pModuleInfo->setInstance(pNewInstance);
    return pNewInstance;
}


}; // namespace qd
//////////////////////////////////////////////////////////////////////////
