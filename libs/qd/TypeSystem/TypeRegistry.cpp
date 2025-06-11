#include "TypeRegistry.h"
#include "EASTL/fixed_vector.h"
#include "EASTL/sort.h"
#include "EASTL/vector.h"
#include "ReflectedType.h"
#include "TypeInfo.h"
#include <qd/Debug/assert.h>
#include <qd/Mem/fnvHash.h>
#include "qd/Debug/exception.h"
#include "typeInfoBuilder.h"


//-------------------------------------------------------------------------

namespace qd {


TypeRegistry::SharedData::~SharedData()
{
    for (TypeInfoMap::iterator Iter = m_TypeMap.begin(); Iter != m_TypeMap.end(); ++Iter)
    {
        TypeInfo* pTypeInfo = Iter->second;
        delete pTypeInfo;
    }
    m_TypeMap.clear();
}

//------------------------------------------------------------------------


TypeRegistry::TypeRegistry() {}


TypeRegistry::~TypeRegistry()
{
    delete m_pSharedData;
}


qd::TypeRegistry* TypeRegistry::get()
{
    static TypeRegistry instance;
    return &instance;
}


const TypeInfo& TypeRegistry::getTypeInfo(const StdTypeId& ti, bool bReplaceIfDefined /*= false*/)
{
    TypeInfoMap& typeMap = getSharedData()->m_TypeMap;
    TypeInfoMap::iterator Iter = typeMap.find(ti.getTypePtr());
    if (Iter != typeMap.end())
        return *Iter->second;
    return _createUnNamedTypeInfoByStdType(ti);
}


void TypeRegistry::bindNamedTypeInfo(const TypeInfo& ti)
{
    assert(ti.isDefined());
    SharedData* pSharedData = getSharedData();

    assert(& getTypeInfo(ti.getStdTypeId()) == &ti && "Type not registered yet");

    const string& name = ti.getFullName();
    THash32 nameHash = hash_type_info_name(name.c_str(), (uint32_t)name.size());
    auto it = pSharedData->m_TypeByFullName.find(nameHash);

    if (it != pSharedData->m_TypeByFullName.end())
    {
        if (it->second->getStdTypeId() == ti.getStdTypeId())
            return;
        G_THROW_OR_DO(Exception("Duplicate type name found: " + name), return);
    };
    pSharedData->m_TypeByFullName.insert(eastl::make_pair(nameHash, &ti));
}


const TypeInfo& TypeRegistry::_createUnNamedTypeInfoByStdType(const StdTypeId& ti)
{
    SharedData* pSharedData = getSharedData();
    TypeInfoMap& typeMap = pSharedData->m_TypeMap;
    assert(typeMap.find(ti.getTypePtr()) == typeMap.end());
    //     if (Iter != typeMap.end())
    //     {
    //         TypeInfo* pOldType = Iter->second;
    //         assert(pOldType->getStdTypeId() == ti);
    //         return pOldType;
    //     }

    TypeInfo* pType = new TypeInfo(ti);
    typeMap[ti.getTypePtr()] = pType;
    return *pType;
}


void TypeRegistry::_createSharedData() const
{
    TypeRegistry* pThis = const_cast<TypeRegistry*>(this);
    assert(pThis == TypeRegistry::get());
    pThis->m_pSharedData = new SharedData();

    TypeRegistry::SharedData* pData = pThis->m_pSharedData;

    TypeInfoBuilder voidBldr(makeStdTypeId_<void>(), pThis);
    voidBldr.declareType("void");
}



TypeRegistry::SharedData* TypeRegistry::getSharedData() const
{
    if (!m_pSharedData)
        _createSharedData();
    return m_pSharedData;
}

const TypeInfoMap& TypeRegistry::getTypesMap()
{
    return getSharedData()->m_TypeMap;
}


eastl::vector<const TypeInfo*> TypeRegistry::findAllDerivedFromTypes(const TypeInfo& rBaseType, bool bIncludeBaseInList)
{
    // TODO: too slow to iterate all types in the system
    // It's better to store index of all Inherited types to separate types
    eastl::vector<const TypeInfo*> result;

    if (bIncludeBaseInList)
        result.push_back(&rBaseType);

    const TypeInfoMap& Types = this->getTypesMap();
    for (TypeInfoMap::const_iterator it = Types.begin(); it != Types.end(); ++it)
    { // iterates all
        const TypeInfo* pCurType = it->second;
        if (pCurType->isDerivedFrom(rBaseType) && pCurType != &rBaseType)
            result.push_back(pCurType);
    }
    return eastl::move(result);
}


const qd::TypeInfo* TypeRegistry::findTypeByName(const char* type_name) const
{
    const SharedData::TTypeByFullNameMap& TypeMap = getSharedData()->m_TypeByFullName;
    THash32 nameHash = hash_type_info_name(type_name);
    auto iter = TypeMap.find(nameHash);
    if (iter == TypeMap.end())
        return nullptr;
    return iter->second;
}


const qd::TypeInfo& TypeRegistry::getTypeByName(const char* pName) const
{
    const TypeInfo* pResType = findTypeByName(pName);
    if (!pResType)
        G_THROW_OR_DO(Exception(EException::NOT_FOUND, "ERROR: Reflected type:'%s' - not declared!", CC(pName)),
            return *m_pVoidType);
    return *pResType;
}


const qd::TypeInfo& getTypeInfo(const StdTypeId& ti)
{
    TypeRegistry* pRegistry = TypeRegistry::get();
    const qd::TypeInfo& pTypeInfo = pRegistry->getTypeInfo(ti);
    return pTypeInfo;
}


}; // namespace qd
