#pragma once

#include <typeinfo>
#include <EASTL/hash_map.h>
#include <qdIce/qdTypeSystem/TypeID.h>

//-------------------------------------------------------------------------

namespace qd
{
}

//-------------------------------------------------------------------------

namespace qd
{
class IReflectedType;
class TypeInfo;
class EnumInfo;
class PropertyInfo;
class PropertyPath;
struct ResourceInfo;
struct DataFileInfo;

//-------------------------------------------------------------------------

class TypeRegistry
{
    eastl::hash_map<TypeId, TypeInfo const*> m_registeredTypes;

public:

    TypeRegistry() = default;
    ~TypeRegistry();

    void RegisterInternalTypes();
    void UnregisterInternalTypes();

    //-------------------------------------------------------------------------
    // Type Info
    //-------------------------------------------------------------------------

    const TypeInfo* RegisterType(TypeInfo const* pType);
    void UnregisterType(TypeInfo const* pType);

    // Returns the type information for a given type ID
    TypeInfo const* GetTypeInfo(TypeId typeID) const;

    // Returns the type information for a given type
    template<typename T, typename = std::enable_if_t<std::is_base_of<qd::IReflectedType, T>::value>>
    TypeInfo const* GetTypeInfo() const
    {
        return T::s_pTypeInfo;
    }

    // Returns the resolved property info for a given path
    PropertyInfo const* ResolvePropertyPath(TypeInfo const* pTypeInfo, PropertyPath const& pathID) const;

    // Is this type registered?
    inline bool IsRegisteredType(TypeId typeID) const { return m_registeredTypes.find(typeID) != m_registeredTypes.end(); }

    // Does a given type derive from a given parent type
    bool IsTypeDerivedFrom(TypeId typeID, TypeId parentTypeID) const;

    // Return all known types
    TVector<TypeInfo const*> GetAllTypes(bool includeAbstractTypes = true, bool sortAlphabetically = false) const;

    // Return all types that derived from a specified type
    TVector<TypeInfo const*> GetAllDerivedTypes(TypeId parentTypeID, bool includeParentTypeInResults = false, bool includeAbstractTypes = true, bool sortAlphabetically = false) const;

    // Get all the types that this type is allowed to be cast to
    TInlineVector<TypeId, 5> GetAllCastableTypes(IReflectedType const* pType) const;

    // Are these two types in the same derivation chain (i.e. does either derive from the other )
    bool AreTypesInTheSameHierarchy(TypeId typeA, TypeId typeB) const;

    // Are these two types in the same derivation chain (i.e. does either derive from the other )
    bool AreTypesInTheSameHierarchy(TypeInfo const* pTypeInfoA, TypeInfo const* pTypeInfoB) const;


}; // class TypeRegistry
}; // namespace qd
