#include "qd/app/moduleManager.h"
#include "moduleBase.h"
#include "qd/app/application.h"
#include "qd/debug/assert.h"
#include "qd/debug/exception.h"
#include "qd/mem/ptrMath.h"
#include "qd/stl/unique_ptr.h"
#include "qd/typeSystem/typeInfo.h"


namespace qd {

bool ModuleManager::m_bSingleDestroyed = false;
ModuleManager* ModuleManager::m_pSingleInstance = 0;


ModuleInfo* ModuleManager::registerModule(const ModuleInfo& moduleInfo)
{
    const qd::TypeInfo& moduleId = moduleInfo.getModuleId();

    ModuleInfo* pModule = findModuleInfo(moduleId);
    if (!pModule)
    {
        qd::unique_ptr<ModuleInfo> pNewModule = qd::make_unique<ModuleInfo>(moduleId);
        pModule = pNewModule.get();
        ModuleManager::InfoItem item;
        item.m_pType = &moduleId;
        item.m_pModuleInfo = std::move(pNewModule);
        m_pModuleInfoMap.push_back(std::move(item));
    }
    pModule->merge(moduleInfo);

    return pModule;
}


ModuleInfo* ModuleManager::findModuleInfo(const qd::TypeInfo& moduleId) const
{

    for (auto& it : m_pModuleInfoMap)
    {
        if (moduleId.isDerivedFrom(*it.m_pType))
            return it.m_pModuleInfo.get();
    }
    return nullptr;
}


ModuleInfo* ModuleManager::getOrCreateModuleInfo(const qd::TypeInfo& moduleId)
{
    ModuleInfo* pModuleInfo = findModuleInfo(moduleId);
    if (!pModuleInfo)
    {
        pModuleInfo = registerModule(ModuleInfo(moduleId));
    }
    return pModuleInfo;
}


void ModuleManager::setModuleInstance(const qd::TypeInfo& ModuleId, IModuleInterface* pInstance)
{
    ModuleInfo* pModuleInfo = getOrCreateModuleInfo(ModuleId);
    pModuleInfo->setInstance(pInstance);
    return;
}


qd::IModuleInterface* ModuleManager::createModuleInstance(const qd::TypeInfo& ModuleId, bool bSingletonInstance,
    qd::ModuleCreateParams* pCreateParam)
{
    ModuleInfo* pModuleInfo = getOrCreateModuleInfo(ModuleId);
    if (!pModuleInfo)
        throw Exception("Module Manger can't Create module: '%s' - module not registered",
            ModuleId.getFullName().c_str());

    if (!pCreateParam)
        pCreateParam = &m_defaultCreateParam;
    pCreateParam->moduleMgr = this;
    if (!pCreateParam->app)
        pCreateParam->app = qd::Application::get();

    IModuleInterface* pInstance;
    pInstance = pModuleInfo->makeInstance(bSingletonInstance, *pCreateParam);
    return pInstance;
}


qd::IModuleInterface* ModuleManager::loadModule(const qd::TypeInfo& moduleId, qd::ModuleCreateParams* pCreateParam)
{
    ModuleInfo* pModuleInfo = getOrCreateModuleInfo(moduleId);
    if (!pModuleInfo)
        throw Exception("Module Manager: Can't Destroy Module. Module : %u Not Declare", (uint32_t)0);

    pModuleInfo->retainInstance();

    if (pModuleInfo->getInstance())
        return pModuleInfo->getInstance();

    if (!pCreateParam)
        pCreateParam = &m_defaultCreateParam;
    pCreateParam->moduleMgr = this;
    if (!pCreateParam->app)
        pCreateParam->app = qd::Application::get();
    return pModuleInfo->makeInstance(true, *pCreateParam);
}


void ModuleManager::unloadModule(const qd::TypeInfo& ModuleId, bool bIsShutdown)
{
    ModuleInfo* pModuleInfo = findModuleInfo(ModuleId);
    if (!pModuleInfo)
        throw Exception("Module Manager: Can't Destroy Module. Module : %u Not Declare", (uint32_t)0);
    pModuleInfo->releaseInstance();
}


qd::IModuleInterface* ModuleManager::getModuleInstance(const qd::TypeInfo& moduleId, bool bMakeInst /*= true */)
{
    ModuleInfo* pModuleInfo = findModuleInfo(moduleId);
    if (!pModuleInfo)
        throw Exception(EException::NOT_FOUND, "Module:'%s' not registered", CC(moduleId.getFullName()));
    IModuleInterface* pExistInst = pModuleInfo->getInstance();
    if (pExistInst || !bMakeInst)
        return pExistInst;
    return this->createModuleInstance(moduleId, true, nullptr);
}


bool ModuleManager::isModuleRegistered(const qd::TypeInfo& moduleId) const
{
    ModuleInfo* pModuleInfo = findModuleInfo(moduleId);
    if (pModuleInfo != nullptr)
        return true;
    return false;
}


bool ModuleManager::isModuleLoaded(const qd::TypeInfo& ModuleId) const
{
    ModuleInfo* pModuleInfo = findModuleInfo(ModuleId);
    if (pModuleInfo != nullptr)
    {
        if (pModuleInfo->getInstance())
            return true;
    }
    return false;
}


qd::IModuleInterface* ModuleManager::getOrCreateModule(const qd::TypeInfo& ModuleId)
{
    ModuleInfo* pModuleInfo = findModuleInfo(ModuleId);
    if (!pModuleInfo)
        throw Exception("Module Manger can't Create module: '%s' - module not registered", CC(ModuleId.getFullName()));

    IModuleInterface* pInstance = pModuleInfo->getInstance();
    if (!pInstance)
        pInstance = pModuleInfo->makeInstance(true, m_defaultCreateParam);
    return pInstance;
}


qd::ModuleManager* ModuleManager::I()
{
    // PHOENIX SINGLETON
    if (!TThis::m_pSingleInstance && !TThis::m_bSingleDestroyed)
    {
        // qd::String::gInitStaticData(); // First create qd::string(W) + Destroy Singleton
        m_pSingleInstance = new TThis();
        ::atexit(&TThis::_destroySingleton);
        m_bSingleDestroyed = false;
    }
    return TThis::m_pSingleInstance;
}


ModuleManager::ModuleManager(void) /*: m_pFastModules(ECgModuleID::_FAST_ACCESS_)*/
{
    c_def(0);
}


void ModuleManager::cleanUp()
{
    // Modules are unloaded in reverse order to when their StartupModule() FINISHES.
    // The practical implication of this is that if module A depends on another module B,
    // and A loads B during A's StartupModule, that B will actually get Unloaded after A during shutdown.
    // This allows A's ShutdownModule() call to still reference module B.

    for (auto It = m_pModuleInfoMap.begin(); It != m_pModuleInfoMap.end(); ++It)
    {
        ModuleInfo* pModuleInfo = It->m_pModuleInfo.get();
        if (!pModuleInfo)
            continue;
        if (!pModuleInfo->m_pInstance)
        {
            assert(pModuleInfo->m_nInstanceRef == 0);
            continue;
        }
        pModuleInfo->releaseInstance(); // ONE TIME RELEASE INSTANCE DESTROYS

        if (pModuleInfo->m_pInstance)
            c_def(0);
        // 			pModuleInfo->m_pInstance = nullptr;
        // 			ModuleManager::_ShutDownInstance( pInstance );
        // 			ModuleManager::_DestroyInstance( pInstance );
        c_def(0);
    }
}


ModuleManager::~ModuleManager(void)
{
    // Modules are unloaded in reverse order to when their StartupModule() FINISHES.
    // The practical implication of this is that if module A depends on another module B,
    // and A loads B during A's StartupModule, that B will actually get Unloaded after A during shutdown.
    // This allows A's ShutdownModule() call to still reference module B.

    // DESTROY MODULE INFO
    while (!m_pModuleInfoMap.empty())
    {
        auto It = m_pModuleInfoMap.rbegin();
        qd::unique_ptr<ModuleInfo> pModuleInfo = eastl::move(It->m_pModuleInfo);
        m_pModuleInfoMap.erase(It);
        if (pModuleInfo)
        {
            // assert2( !pModuleInfo->m_pInstance, "Not deleted module \"%s\" found", CC(pModuleInfo->m_ModuleName) );
            if (pModuleInfo->m_pInstance)
            {
                // FORCE DESTROY
                ModuleManager::_shutDownInstance(pModuleInfo->m_pInstance);
                ModuleManager::_destroyInstance(pModuleInfo->m_pInstance);
                pModuleInfo->m_pInstance = nullptr;
            }
            // delete pModuleInfo;
            pModuleInfo.release();
        }
    }
}


uint32_t ModuleManager::destroyModule(const qd::TypeInfo& moduleId)
{
    ModuleInfo* pModuleInfo = findModuleInfo(moduleId);
    if (!pModuleInfo)
    {
        // throw Exception( "Module Manager: Can't Destroy Module. Module : %u Not Declare", (uint)moduleId );
        return 0;
    }
    return pModuleInfo->releaseInstance();
}


void ModuleManager::destroyModule(const qd::TypeInfo& moduleId, IModuleInterface* pInstance)
{
    destroyModule(moduleId);
    if (pInstance)
    {
        ModuleManager::_shutDownInstance(pInstance);
        ModuleManager::_destroyInstance(pInstance);
    }
}


ModuleInfo* ModuleManager::overrideModule(const qd::TypeInfo& moduleId, const ModuleInfo::TCreateFunc& pCreateFunc)
{
    ModuleInfo* pModuleInfo = findModuleInfo(moduleId);
    if (!pModuleInfo)
    {
        ModuleInfo m(moduleId);
        pModuleInfo = registerModule(m);
    }
    pModuleInfo->m_CreateFunc = pCreateFunc; // CUSTOM CREATE FUNC
    return pModuleInfo;
}


void ModuleInfo::setInstance(IModuleInterface* pInstance)
{
    if (m_pInstance == pInstance)
        return;
    // 		if ( !pInstance ) {
    // 			ReleaseInstance();
    // 			return;
    // 		}
    if (m_pInstance)
    {
        assert(0 && "Instance already Set");
        return;
    }
    // 		++ m_nInstanceRef; // REGISTER
    // 		assert(m_nInstanceRef == 1);
    m_pInstance = pInstance;
}


uint32_t ModuleInfo::retainInstance()
{
    ModuleInfo* pModuleInfo = this;
    assert(pModuleInfo);
    ++pModuleInfo->m_nInstanceRef;
    return pModuleInfo->m_nInstanceRef;
}


uint32_t ModuleInfo::releaseInstance()
{
    ModuleInfo* pModuleInfo = this;
    assert(pModuleInfo);
    if (pModuleInfo->m_nInstanceRef == 0)
    {
        // pModuleInfo->SetInstance(nullptr);
        pModuleInfo->m_pInstance = nullptr;
        return 0;
    }

    if (--pModuleInfo->m_nInstanceRef == 0)
    {
        IModuleInterface* pInstance = pModuleInfo->getInstance();
        if (pInstance)
        {
            ModuleManager::_shutDownInstance(pInstance);
            ModuleManager::_destroyInstance(pInstance);
            pModuleInfo->m_pInstance = nullptr;
            // pModuleInfo->SetInstance(nullptr);
        }
        return 0;
    }
    return pModuleInfo->m_nInstanceRef;
}


IModuleInterface* ModuleInfo::makeInstance(bool bRegisterInstance, const ModuleCreateParams& createParam)
{
    TThis* pModuleInfo = this;
    if (!pModuleInfo->m_CreateFunc)
        return nullptr;

    IModuleInterface* pNewInstance = pModuleInfo->m_CreateFunc(createParam); // MAKE INSTANCE
    // pNewInstance->m_CGModuleTypeID = pModuleInfo->getModuleId();
    if (bRegisterInstance)
    {
        pModuleInfo->setInstance(pNewInstance);
    }
    ModuleManager::_onStatrupInstance(pNewInstance, createParam);
    return pNewInstance;
}


void ModuleInfo::merge(const ModuleInfo& r)
{
    assert(!r.m_pInstance);
    assert(m_ModuleId == r.m_ModuleId);

    if (m_ModuleName.empty())
        m_ModuleName = r.m_ModuleName;

    m_CreateFunc = r.m_CreateFunc;
}


void IModuleInterface::onModuleMessageProc(qd::moduleMsg::BaseMsg& in_msg)
{
    switch (in_msg.id)
    {
    case qd::moduleMsg::RENDER_IMGUI_DEBUG_INFO_TREE::ID:
        // qd::Modules::ImGuiCG::ImGuiDrawForModule(this);
        break;
    default:
        break;
    }
}


}; // namespace qd
