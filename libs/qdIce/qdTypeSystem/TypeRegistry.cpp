#include "TypeRegistry.h"
#include "EASTL/fixed_vector.h"
#include "EASTL/sort.h"
#include "EASTL/vector.h"
#include "ReflectedType.h"
#include "TypeInfo.h"
#include <qdIce/qdDebug/assert.h>


//-------------------------------------------------------------------------

namespace qd {
template<>
class TypeInfo_<IReflectedType> final : public TypeInfo
{
public:
    static void RegisterType(TypeRegistry& typeRegistry)
    {
        IReflectedType::s_pTypeInfo = new TypeInfo_<IReflectedType>();
        typeRegistry.RegisterType(IReflectedType::s_pTypeInfo);
    }

    static void UnregisterType(TypeRegistry& typeRegistry)
    {
        typeRegistry.UnregisterType(IReflectedType::s_pTypeInfo);
        delete IReflectedType::s_pTypeInfo;
    }

public:
    TypeInfo_()
    {
        m_ID = TypeId("qd::IReflectedType");
        m_size = sizeof(IReflectedType);
        m_alignment = alignof(IReflectedType);
    }
};
} // namespace qd

//-------------------------------------------------------------------------

namespace qd {
TypeRegistry::~TypeRegistry()
{
    assert(m_registeredTypes.empty());
}

void TypeRegistry::RegisterInternalTypes()
{
    TypeInfo_<IReflectedType>::RegisterType(*this);
}

void TypeRegistry::UnregisterInternalTypes()
{
    TypeInfo_<IReflectedType>::UnregisterType(*this);
}

//-------------------------------------------------------------------------
// Type Info
//-------------------------------------------------------------------------

const TypeInfo* TypeRegistry::RegisterType(TypeInfo const* pTypeInfo)
{
    assert(pTypeInfo != nullptr);
    assert(pTypeInfo->m_ID.IsValid());
    assert(m_registeredTypes.find(pTypeInfo->m_ID) == m_registeredTypes.end());
    m_registeredTypes.insert(eastl::pair<TypeId, TypeInfo const*>(pTypeInfo->m_ID, pTypeInfo));
    return m_registeredTypes[pTypeInfo->m_ID];
}

void TypeRegistry::UnregisterType(const TypeInfo* pTypeInfo)
{
    assert(pTypeInfo != nullptr);
    assert(pTypeInfo->m_ID.IsValid());
    auto iter = m_registeredTypes.find(pTypeInfo->m_ID);
    assert(iter != m_registeredTypes.end());
    assert(iter->second == pTypeInfo);
    m_registeredTypes.erase(iter);
}

TypeInfo const* TypeRegistry::GetTypeInfo(TypeId typeID) const
{
    assert(typeID.IsValid());
    auto iter = m_registeredTypes.find(typeID);
    if (iter != m_registeredTypes.end())
        return iter->second;
    else
        return nullptr;
}


bool TypeRegistry::IsTypeDerivedFrom(TypeId typeID, TypeId parentTypeID) const
{
    assert(typeID.IsValid() && parentTypeID.IsValid());

    auto pTypeInfo = GetTypeInfo(typeID);
    assert(pTypeInfo != nullptr);

    return pTypeInfo->IsDerivedFrom(parentTypeID);
}


TVector<TypeInfo const*> TypeRegistry::GetAllTypes(bool includeAbstractTypes, bool sortAlphabetically) const
{
    TVector<TypeInfo const*> types;

    for (auto const& typeInfoPair : m_registeredTypes)
    {
        if (!includeAbstractTypes && typeInfoPair.second->IsAbstractType())
            continue;
        types.emplace_back(typeInfoPair.second);
    }

    if (sortAlphabetically)
    {
        auto sortPredicate = [](TypeInfo const* const& pTypeInfoA, TypeInfo const* const& pTypeInfoB) {
            return strcmp(pTypeInfoA->m_ID.c_str(), pTypeInfoB->m_ID.c_str());
        };

        eastl::sort(types.begin(), types.end(), sortPredicate);
    }

    return types;
}


TVector<TypeInfo const*> TypeRegistry::GetAllDerivedTypes(TypeId parentTypeID, bool includeParentTypeInResults,
    bool includeAbstractTypes, bool sortAlphabetically) const
{
    TVector<TypeInfo const*> matchingTypes;

    for (auto const& typeInfoPair : m_registeredTypes)
    {
        if (!includeParentTypeInResults && typeInfoPair.first == parentTypeID)
            continue;

        if (!includeAbstractTypes && typeInfoPair.second->IsAbstractType())
            continue;

        if (typeInfoPair.second->IsDerivedFrom(parentTypeID))
            matchingTypes.emplace_back(typeInfoPair.second);
    }

    if (sortAlphabetically)
    {
        auto sortPredicate = [](TypeInfo const* const& pTypeInfoA, TypeInfo const* const& pTypeInfoB) {
            return strcmp(pTypeInfoA->m_ID.c_str(), pTypeInfoB->m_ID.c_str());
        };

        eastl::sort(matchingTypes.begin(), matchingTypes.end(), sortPredicate);
    }

    return matchingTypes;
}


TInlineVector<qd::TypeId, 5> TypeRegistry::GetAllCastableTypes(IReflectedType const* pType) const
{
    assert(pType != nullptr);
    TInlineVector<qd::TypeId, 5> parentTypeIDs;
    auto pParentTypeInfo = pType->getTypeInfo()->m_pParentTypeInfo;
    while (pParentTypeInfo != nullptr)
    {
        parentTypeIDs.emplace_back(pParentTypeInfo->m_ID);
        pParentTypeInfo = pParentTypeInfo->m_pParentTypeInfo;
    }
    return parentTypeIDs;
}

bool TypeRegistry::AreTypesInTheSameHierarchy(TypeId typeA, TypeId typeB) const
{
    auto pTypeInfoA = GetTypeInfo(typeA);
    auto pTypeInfoB = GetTypeInfo(typeB);
    return AreTypesInTheSameHierarchy(pTypeInfoA, pTypeInfoB);
}

bool TypeRegistry::AreTypesInTheSameHierarchy(TypeInfo const* pTypeInfoA, TypeInfo const* pTypeInfoB) const
{
    if (pTypeInfoA->IsDerivedFrom(pTypeInfoB->m_ID))
        return true;

    if (pTypeInfoB->IsDerivedFrom(pTypeInfoA->m_ID))
        return true;

    return false;
}

}; // namespace qd
