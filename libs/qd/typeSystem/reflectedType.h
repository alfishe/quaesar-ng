#pragma once
#include "qd/typeSystem/typeInfoBuilder.h"
#include "qd/mem/fnvHash.h"


//-------------------------------------------------------------------------
// Type Reflection
//-------------------------------------------------------------------------

namespace qd {
class TypeRegistry;
class PropertyInfo;
class TypeInfo;
class TypeId;






// Base type for reflection
//-------------------------------------------------------------------------
// Interface to enforce virtual destructors and type-info overrides

class IReflectedType
{
public:
    inline static qd::TypeInfo const* s_pTypeInfo = nullptr;

public:
    IReflectedType() = default;
    IReflectedType(IReflectedType const&) = default;
    virtual ~IReflectedType() = default;

    IReflectedType& operator= (IReflectedType const& rhs) = default;

    virtual const qd::TypeInfo* getTypeInfo() const = 0;
    // virtual const qd::TypeId& getTypeId() const = 0;
}; // class IReflectedType
//////////////////////////////////////////////////////////////////////////



// Helper methods
//-------------------------------------------------------------------------

template<typename T>
bool IsOfType(IReflectedType const* pType)
{
    if (pType == nullptr)
    {
        return false;
    }

    return pType->GetTypeInfo()->isDerivedFrom_(T::GetStaticTypeID());
}

// This is a assumed safe cast, it will validate the cast only in dev builds. Doesnt accept null arguments
template<typename T>
T* Cast(IReflectedType* pType)
{
    assert(pType != nullptr);
    assert(pType->GetTypeInfo()->IsDerivedFrom(T::GetStaticTypeID()));
    return reinterpret_cast<T*>(pType);
}

// This is a assumed safe cast, it will validate the cast only in dev builds. Doesnt accept null arguments
template<typename T>
T const* Cast(IReflectedType const* pType)
{
    assert(pType != nullptr);
    assert(pType->GetTypeInfo()->IsDerivedFrom(T::GetStaticTypeID()));
    return reinterpret_cast<T const*>(pType);
}

// This will try to cast to the specified type but can fail. Also accepts null arguments
template<typename T>
T* TryCast(IReflectedType* pType)
{
    if (pType != nullptr && pType->GetTypeInfo()->isDerivedFrom_(T::GetStaticTypeID()))
    {
        return reinterpret_cast<T*>(pType);
    }

    return nullptr;
}

// This will try to cast to the specified type but can fail. Also accepts null arguments
template<typename T>
T const* TryCast(IReflectedType const* pType)
{
    if (pType != nullptr && pType->GetTypeInfo()->isDerivedFrom_(T::GetStaticTypeID()))
    {
        return reinterpret_cast<T const*>(pType);
    }

    return nullptr;
}


}; // namespace qd
//////////////////////////////////////////////////////////////////////////

