#pragma once
#include "qd/typeSystem/typeInfoBuilder.h"
#include "qd/mem/fnvHash.h"

#if 0


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
bool isOfType_(IReflectedType const* pType)
{
    if (pType == nullptr)
    {
        return false;
    }

    return pType->getTypeInfo()->isDerivedFrom_(T::getStaticTypeId());
}

// This is a assumed safe cast, it will validate the cast only in dev builds. Doesnt accept null arguments
template<typename T>
T* cast_(IReflectedType* pType)
{
    assert(pType != nullptr);
    assert(pType->getTypeInfo()->IsDerivedFrom(T::GetStaticTypeID()));
    return reinterpret_cast<T*>(pType);
}

// This is a assumed safe cast, it will validate the cast only in dev builds. Doesnt accept null arguments
template<typename T>
T const* cast_(IReflectedType const* pType)
{
    assert(pType != nullptr);
    assert(pType->getTypeInfo()->IsDerivedFrom(T::GetStaticTypeID()));
    return reinterpret_cast<T const*>(pType);
}

// This will try to cast to the specified type but can fail. Also accepts null arguments
template<typename T>
T* tryCast_(IReflectedType* pType)
{
    if (pType != nullptr && pType->getTypeInfo()->isDerivedFrom_(T::GetStaticTypeID()))
    {
        return reinterpret_cast<T*>(pType);
    }

    return nullptr;
}

// This will try to cast to the specified type but can fail. Also accepts null arguments
template<typename T>
T const* tryCast_(IReflectedType const* pType)
{
    if (pType != nullptr && pType->getTypeInfo()->isDerivedFrom_(T::GetStaticTypeID()))
    {
        return reinterpret_cast<T const*>(pType);
    }

    return nullptr;
}

}; // namespace qd
//////////////////////////////////////////////////////////////////////////

#endif //
