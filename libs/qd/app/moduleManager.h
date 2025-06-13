#pragma once
#include "EASTL/fixed_map.h"
#include "EASTL/fixed_vector.h"
#include "qd/base/base.h"
#include "qd/base/ref_ptr.h"
#include "qd/stl/string.h"
#include <EASTL/fixed_function.h>
#include <EASTL/unique_ptr.h>
#include <qd/app/moduleBase.h>


/* Declares struct that register class module creation */
#define QD_MODULE_REGISTRATION(ClassName) \
    static qd::ModuleRegistrator_<ClassName> PASTE(gModuleImp_, __COUNTER__)(/**/ STRINGIFY(ClassName));
#define QD_MODULE_REGISTRATION_NOCREATE(ClassName) \
    static qd::ModuleRegistratorNoCreate_<ClassName> PASTE(gModuleImp_, __COUNTER__)(/**/ STRINGIFY(ClassName));


namespace qd {

#if (0)

class CDummyModule : public IModuleInterface
{
public:
    static qd::ECgModuleID getModuleTypeId() { return qd::ECgModuleID::DUMMY; }

private:
public:
    CDummyModule(qd::CModuleCreateParams* p) {}

}; // class
static ModuleRegistrator_<CDummyModule> s;

#endif // 0




//////////////////////////////////////////////////////////////////////////
class ModuleInfo
{
    typedef ModuleInfo TThis;
    friend class ModuleManager;

public:
    ECgModuleID m_ModuleID;
    ref_ptr<IModuleInterface> m_pInstance;
    eastl::fixed_function<8, IModuleInterface*(qd::ModuleCreateParams*)> m_CreateFunc;
    typedef eastl::fixed_function<8, IModuleInterface*(qd::ModuleCreateParams*)> TCreateFunc;
    uint32_t m_nInstanceRef;
    qd::string m_ModuleName;

public:
    ModuleInfo(ECgModuleID ModuleID)
        : m_ModuleID(ModuleID)
        , m_pInstance(nullptr)
        , m_nInstanceRef(0)
    {}


    qd::ECgModuleID getModuleId() const { return m_ModuleID; }

    void setModuleId(qd::ECgModuleID ModuleID) { m_ModuleID = ModuleID; }

    const ref_ptr<IModuleInterface>& getInstance() const { return m_pInstance; }

    void setInstance(ref_ptr<IModuleInterface> Instance);

    uint32_t retainInstance();
    uint32_t releaseInstance();
    void merge(const ModuleInfo& r);

    ref_ptr<IModuleInterface> makeInstance(bool bRegisterSingleton, qd::ModuleCreateParams* pCreateParam = nullptr);


    IModuleInterface* getOrCreateInstance()
    {
        if (m_pInstance)
            return m_pInstance;
        return makeInstance(true);
    }

    // MAKES MODULE INSTANCE
    template<class TModuleClass>
    TModuleClass* makeInstance_(bool bRegisterSingleton, qd::ModuleCreateParams* mc = nullptr);


}; // class ModuleInfo
//////////////////////////////////////////////////////////////////////////



template<class TModule>
struct ModuleRegistrator_ {
    typedef ModuleRegistrator_<TModule> TThis;
    inline ModuleRegistrator_(const char* pClassName); // CONSTRUCTOR TO REGISTER THIS STRUCT IN #ModuleManager

    static qd::IModuleInterface* createModuleFunc(qd::ModuleCreateParams* pCP);

}; // struct ModuleRegistrator_
//////////////////////////////////////////////////////////////////////////


template<class TModule>
struct ModuleRegistratorNoCreate_ {
    typedef ModuleRegistratorNoCreate_<TModule> TThis;
    inline ModuleRegistratorNoCreate_(const char* pClassName); // CONSTRUCTOR TO REGISTER THIS STRUCT IN #ModuleManager

    static qd::IModuleInterface* createModuleFunc(qd::ModuleCreateParams* pCP) { return nullptr; }

}; // struct ModuleRegistrator_
//////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////
// #ModuleManager
class ModuleManager
{
    typedef ModuleManager TThis;
    static constexpr uint32_t g_nMAX_MODULES = ECgModuleID::_FAST_ACCESS_ + 16;

    eastl::fixed_map< uint32_t, eastl::unique_ptr<ModuleInfo>, g_nMAX_MODULES, false > m_pModuleInfoMap;

public:
    typedef eastl::fixed_map< uint32_t, eastl::unique_ptr<ModuleInfo>, g_nMAX_MODULES, false > TModuleInfoMap;

    eastl::fixed_vector< ModuleInfo*, ECgModuleID::_FAST_ACCESS_, false > m_pFastModules;

private:
    static TThis* m_pSingleInstance;

private:
    static bool m_bSingleDestroyed;

public:
    static ModuleManager* I();

    static inline ModuleManager* get() { return TThis::I(); }

    static void _destroySingleton()
    {
        SAFE_DELETE(TThis::m_pSingleInstance);
        TThis::m_bSingleDestroyed = true;
    }

public:
    ModuleManager(void);

    void cleanUp();


    virtual ~ModuleManager(void);


    const ModuleManager::TModuleInfoMap& getModuleInfoMap() const { return m_pModuleInfoMap; }


    ModuleInfo* overrideModule(ECgModuleID ModuleId, const ModuleInfo::TCreateFunc& pCreateFunc);


    // Default create function - concreate module
    template<class TModuleClass>
    inline ModuleInfo* overrideModule_(const ModuleInfo::TCreateFunc& pCreateFunc = CCallbackNull());


    ModuleInfo* registerModule(const ModuleInfo& i);


    ModuleInfo* findModuleInfo(ECgModuleID ModuleId) const;

    ModuleInfo* getOrCreateModuleInfo(ECgModuleID ModuleId);

    void setModuleInstance(ECgModuleID ModuleId, ref_ptr<IModuleInterface> pInstance);

    template<class TModuleClass>
    inline void setModuleInstance_(ref_ptr<TModuleClass> pInstance)
    {
        ECgModuleID ModuleId = TModuleClass::getModuleTypeId();
        setModuleInstance(ModuleId, pInstance);
    }


    qd::IModuleInterface* getOrCreateModule(ECgModuleID ModuleId);

    qd::IModuleInterface* createModuleInstance(ECgModuleID ModuleId, bool bSingletonInstance,
        qd::ModuleCreateParams* pCreateParam = nullptr);


    // JUST CREATES MODULE INSTANCE WITHOUT STORE IT IN MODULE_INFO
    template<class TModuleClass>
    inline ref_ptr<TModuleClass> makeInstance_(qd::ModuleCreateParams* mc = nullptr)
    {
        ECgModuleID ModuleId = TModuleClass::getModuleTypeId();
        qd::ref_ptr<TModuleClass> pInstance = createModuleInstance(ModuleId, /*regInstance:*/ false, mc);
        return pInstance;
    }


    IModuleInterface* findModuleInstance(ECgModuleID ModuleId)
    {
        ModuleInfo* pModuleInfo = findModuleInfo(ModuleId);
        if (!pModuleInfo)
            return nullptr;
        IModuleInterface* pCurModule = pModuleInfo->getInstance();
        return pCurModule;
    }

    template<class TModuleClass>
    TModuleClass* findInstacnce_()
    {
        ECgModuleID ModuleId = TModuleClass::getModuleTypeId();
        ptr<TModuleClass> pInstance = findModuleInstance(ModuleId);
        return pInstance;
    }

    // RETAINS MODULE
    IModuleInterface* loadModule(ECgModuleID ModuleId, qd::ModuleCreateParams* pCreateParam = nullptr);

    template<class TModuleClass>
    inline TModuleClass* loadModule_(qd::ModuleCreateParams* pCreateParam = nullptr)
    {
        ECgModuleID ModuleId = TModuleClass::getModuleTypeId();
        ptr<TModuleClass> pInstance = loadModule(ModuleId, pCreateParam);
        return pInstance;
    }

    // Modules are unloaded in reverse order to when their StartupModule() FINISHES.
    // The practical implication of this is that if module A depends on another module B,
    // and A loads B during A's StartupModule, that B will actually get Unloaded after A during shutdown.
    // This allows A's ShutdownModule() call to still reference module B.
    // RELEASE MODULE
    void unloadModule(ECgModuleID ModuleId, bool bIsShutdown = false);

    template<class TModuleClass>
    inline void unloadModule_()
    {
        ECgModuleID ModuleId = TModuleClass::getModuleTypeId();
        unloadModule(ModuleId);
    }

    IModuleInterface* getModuleInstance(ECgModuleID ModuleId, bool bMakeInst = false);


    template<class TModule>
    inline TModule* getModuleInstance_()
    {
        ptr<TModule> pExistInst = getModuleInstance(TModule::getModuleTypeId(), false);
        return pExistInst;
    }


    template<class TModule>
    TModule* getModuleInstanceOrCreate_()
    {
        ptr<TModule> pExistInst = getModuleInstance(TModule::getModuleTypeId(), /*MakeInst*/ false);
        if (pExistInst)
            return pExistInst;
        pExistInst = createModuleInstance(TModule::getModuleTypeId(), /*keepInstance:*/ true);
        return pExistInst;
    }


    bool isModuleRegistered(ECgModuleID ModuleId) const
    {
        ModuleInfo* pModuleInfo = findModuleInfo(ModuleId);
        if (pModuleInfo != nullptr)
        {
            return true;
        }
        return false;
    }

    template<class TModuleClass>
    inline bool isModuleRegistered_() const
    {
        ECgModuleID ModuleId = TModuleClass::getModuleTypeId();
        return isModuleRegistered(ModuleId);
    }


    bool isModuleLoaded(ECgModuleID ModuleId) const
    {
        ModuleInfo* pModuleInfo = findModuleInfo(ModuleId);
        if (pModuleInfo != nullptr)
        {
            if (pModuleInfo->getInstance())
            {
                return true;
            }
        }
        return false;
    }

    template<class TModuleClass>
    inline bool isModuleLoaded_() const
    {
        ECgModuleID ModuleId = TModuleClass::getModuleTypeId();
        return isModuleLoaded(ModuleId);
    }

    uint32_t destroyModule(qd::ECgModuleID ModuleId);




    void destroyModule(qd::ECgModuleID ModuleId, ref_ptr<IModuleInterface> pInstance)
    {
        destroyModule(ModuleId);
        if (pInstance)
        {
            ModuleManager::_shutDownInstance(pInstance);
            ModuleManager::_destroyInstance(pInstance);
        }
    }


    static void _onStatrupInstance(IModuleInterface* pModule, qd::ModuleCreateParams* mc = nullptr)
    {
        if (!pModule->m_ModuleState.m_bModStartuped && !pModule->m_ModuleState.m_bModShutdowned)
        {
            pModule->m_ModuleState.m_bModStartuped = true;
            pModule->onModuleStartup(mc);
        }
    }

    static void _shutDownInstance(IModuleInterface* pModule)
    {
        if (pModule && pModule->m_ModuleState.m_bModStartuped && !pModule->m_ModuleState.m_bModShutdowned)
        {
            pModule->m_ModuleState.m_bModShutdowned = true;
            pModule->onModuleShutdown();
            c_def(0);
        }
    }

    static void _destroyInstance(ref_ptr<IModuleInterface> pInstance)
    {
        if (pInstance && !pInstance->m_ModuleState.m_bModDestroyed)
        {
            pInstance->m_ModuleState.m_bModDestroyed = true;
            pInstance->destroyModule();
            c_def(0);
        }
    }


}; // class ModuleManager
//////////////////////////////////////////////////////////////////////////




namespace Modules {

inline ModuleManager* get()
{
    return ModuleManager::I();
}

template<class TModule>
inline TModule* getModuleInst_()
{
    ModuleManager* pMgr = ModuleManager::get();
    ptr<TModule> pExistInst = pMgr->getModuleInstance(TModule::getModuleTypeId(), false);
    return pExistInst;
}

}; // namespace Modules



template<class TModuleClass>
inline ModuleInfo* ModuleManager::overrideModule_(const ModuleInfo::TCreateFunc& pCreateFunc /*= CCallbackNull() */)
{
    ECgModuleID ModuleId = TModuleClass::getModuleTypeId();
    if (pCreateFunc)
    {
        // CUSTOM CREATE FUNC
        return overrideModule(ModuleId, pCreateFunc);
    }
    else
    {
        // DEFAULT CREATE FUNC
        return overrideModule(ModuleId, BIND_FREE_CB(&ModuleRegistrator_<TModuleClass>::createModuleFunc));
    }
}



template<class TModuleClass>
qd::IModuleInterface* ModuleRegistrator_<TModuleClass>::createModuleFunc(qd::ModuleCreateParams* pCP)
{
    // non static function but with this == null, for emscripten happy
    TModuleClass* pMod = new TModuleClass(pCP);
    return pMod;
}



template<class TModule>
inline ModuleRegistrator_<TModule>::ModuleRegistrator_(const char* pClassName)
{
    ECgModuleID ModuleId = TModule::getModuleTypeId();
    ModuleInfo* pMtd = ModuleManager::I()->overrideModule(ModuleId, BIND_FREE_CB(&TThis::createModuleFunc));
    pMtd->m_ModuleName = pClassName;
}


template<class TModule>
inline ModuleRegistratorNoCreate_<TModule>::ModuleRegistratorNoCreate_(const char* pClassName)
{
    ECgModuleID ModuleId = TModule::getModuleTypeId();
    ModuleInfo* pMtd =
        ModuleManager::I()->overrideModule(ModuleId, BIND_FREE_CB(&TThis::CreateModuleFunc)); // NO CREATE
    pMtd->m_ModuleName = pClassName;
}


template<class TModuleClass>
TModuleClass* ModuleInfo::makeInstance_(bool bRegisterSingleton, qd::ModuleCreateParams* mc /*= nullptr */)
{
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
