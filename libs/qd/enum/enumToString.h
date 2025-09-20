#pragma once
#include "qd/enum/enumBase.h"


// TO STRING WITHOUT CARCHIVE DECLARATION
#define ENUM_DECLARE_TO_STRING_BASE()                                         \
public:                                                                       \
    static inline qd::CEnumString* getEnum()                                  \
    {                                                                         \
        static qd::CEnumString _Enum;                                         \
        return &_Enum;                                                        \
    } /* SINGLETON */                                                         \
public:                                                                       \
    inline const qd::string& toString() const                                 \
    {                                                                         \
        return getEnum()->getStrVal(mV);                                      \
    }                                                                         \
                                                                              \
public:                                                                       \
    template<typename V>                                                      \
    static inline const qd::string& toString(V ID)                            \
    {                                                                         \
        return getEnum()->getStrVal((TEnum)ID);                               \
    }                                                                         \
                                                                              \
public:                                                                       \
    template<typename V>                                                      \
    static inline bool toString(V ID, qd::string& outVal)                     \
    {                                                                         \
        return getEnum()->findStrVal((TEnum)ID, outVal);                      \
    }                                                                         \
                                                                              \
public:                                                                       \
    static inline TEnum fromString(const qd::CStringHash& pName)              \
    {                                                                         \
        return (TEnum)(getEnum()->getIntByStr(pName));                        \
    };                                                                        \
                                                                              \
public:                                                                       \
    template<typename TInt>                                                   \
    static inline bool fromString(const qd::CStringHash& pName, TInt& RetVal) \
    {                                                                         \
        bool bRes = getEnum()->findIntByStr(pName, RetVal);                   \
        return bRes;                                                          \
    };                                                                        \
                                                                              \
public:                                                                       \
    inline bool fromStringSelf(const qd::CStringHash& pName)                  \
    {                                                                         \
        bool bRes = getEnum()->findIntByStr(pName, mV);                       \
        return bRes;                                                          \
    };                                                                        \
                                                                              \
public:                                                                       \
    template<typename V>                                                      \
    static inline bool fromStringReg(const qd::string& Name, V& RetVal)       \
    {                                                                         \
        bool bRes = false;                                                    \
        RetVal = (V)getEnum()->findIntByStrOrRegister(Name, &bRes);           \
        return bRes;                                                          \
    };                                                                        \
                                                                              \
public:                                                                       \
    inline bool fromStringSelfReg(const qd::string& pName)                    \
    {                                                                         \
        bool bRes = false;                                                    \
        mV = (TEnum)getEnum()->findIntByStrOrRegister(pName, &bRes);          \
        return bRes;                                                          \
    };


// WARNING define "ENUM_DECLARE_BASE" declared in "qd/base/base.h"

// ENamespace = "Namespace::ToEnum::"
// ENUM_DECLARE
#define ENUM_DECLARE(ENamespace, EnumNameStruct, eEnumType, _DefaultVal)   \
    ENUM_DECLARE_BASE(ENamespace, EnumNameStruct, eEnumType, _DefaultVal); \
    XENUM_SERIALIZATION(); //
//////////////////////////////////////////////////////////////////////////


// USING
// ENUM_DECLARE_DERIVED(ARoom::, EError, int, 0);

#define ENUM_DECLARE_DERIVED(ENamespace, CEnumStruct_t, eEnumType, _DefaultValue) \
private:                                                                          \
    typedef ENamespace CEnumStruct_t EThis; /* CONCAT NAME */                     \
public:                                                                           \
    inline CEnumStruct_t()                                                        \
    {                                                                             \
        mV = static_cast<TEnum>((_DefaultValue));                                 \
    } /* CONSTRUCTOR */                                                           \
    template<typename V>                                                          \
    inline CEnumStruct_t(const V& Value)                                          \
    {                                                                             \
        mV = static_cast<TEnum>((Value));                                         \
    }                                                                             \
    inline CEnumStruct_t(const CEnumStruct_t& r)                                  \
    {                                                                             \
        mV = r.mV;                                                                \
    }                                                                             \
    inline CEnumStruct_t& operator= (const CEnumStruct_t& r)                      \
    {                                                                             \
        mV = r.mV;                                                                \
        return *this;                                                             \
    }
