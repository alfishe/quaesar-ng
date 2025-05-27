#pragma once
#include "TypeInfo.h"

//-------------------------------------------------------------------------
// Type Reflection
//-------------------------------------------------------------------------

namespace qd {
class TypeRegistry;
class PropertyInfo;

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
    virtual qd::TypeId getTypeId() const = 0;
};

// Default instance constructor
//-------------------------------------------------------------------------
// In some cases, you might need a custom constructor for the default instance
// If you define a ctor with this argument in your reflected type it will use that ctor instead of the default one
// e.g. Foo::Foo( DefaultInstanceCtor_t ) { ... }

enum DefaultInstanceCtor_t {
    DefaultInstanceCtor
};

// Helper methods
//-------------------------------------------------------------------------

template<typename T>
bool IsOfType(IReflectedType const* pType)
{
    if (pType == nullptr)
    {
        return false;
    }

    return pType->GetTypeInfo()->IsDerivedFrom(T::GetStaticTypeID());
}

// This is a assumed safe cast, it will validate the cast only in dev builds. Doesnt accept null arguments
template<typename T>
T* Cast(IReflectedType* pType)
{
    EE_ASSERT(pType != nullptr);
    EE_ASSERT(pType->GetTypeInfo()->IsDerivedFrom(T::GetStaticTypeID()));
    return reinterpret_cast<T*>(pType);
}

// This is a assumed safe cast, it will validate the cast only in dev builds. Doesnt accept null arguments
template<typename T>
T const* Cast(IReflectedType const* pType)
{
    EE_ASSERT(pType != nullptr);
    EE_ASSERT(pType->GetTypeInfo()->IsDerivedFrom(T::GetStaticTypeID()));
    return reinterpret_cast<T const*>(pType);
}

// This will try to cast to the specified type but can fail. Also accepts null arguments
template<typename T>
T* TryCast(IReflectedType* pType)
{
    if (pType != nullptr && pType->GetTypeInfo()->IsDerivedFrom(T::GetStaticTypeID()))
    {
        return reinterpret_cast<T*>(pType);
    }

    return nullptr;
}

// This will try to cast to the specified type but can fail. Also accepts null arguments
template<typename T>
T const* TryCast(IReflectedType const* pType)
{
    if (pType != nullptr && pType->GetTypeInfo()->IsDerivedFrom(T::GetStaticTypeID()))
    {
        return reinterpret_cast<T const*>(pType);
    }

    return nullptr;
}

//-------------------------------------------------------------------------

inline void validateStaticTypeInfoPtr(qd::TypeInfo const* pPtr)
{
    assert(pPtr && "Invalid TypeInfo Ptr");
}
} // namespace qd


//-------------------------------------------------------------------------
// Reflection Macros
//-------------------------------------------------------------------------

#define QD_REFLECT_TYPE(TypeName)                            \
    friend qd::TypeInfo;                                     \
    template<typename T>                                     \
    friend class qd::TypeInfo_;                              \
                                                             \
public:                                                      \
    static const qd::TypeInfo* s_pTypeInfo;                  \
    static qd::TypeId getStaticTypeId()                      \
    {                                                        \
        validateStaticTypeInfoPtr(s_pTypeInfo);              \
        return TypeName::s_pTypeInfo->m_ID;                  \
    }                                                        \
    virtual const qd::TypeInfo* getTypeInfo() const override \
    {                                                        \
        validateStaticTypeInfoPtr(TypeName::s_pTypeInfo);    \
        return TypeName::s_pTypeInfo;                        \
    }                                                        \
    virtual qd::TypeId getTypeId() const override            \
    {                                                        \
        validateStaticTypeInfoPtr(TypeName::s_pTypeInfo);    \
        return TypeName::s_pTypeInfo->m_ID;                  \
    }\
