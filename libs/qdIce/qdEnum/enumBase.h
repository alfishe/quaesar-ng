#pragma once
#include <qdIce/qdBase/base.h>


//////////////////////////////////////////////////////////////////////////
// EnumNamespace = "Namespace::ToEnum::"

#define ENUM_DECLARE_BASE(EnumNamespace, EnumNameStruct, eEnumType, _DefaultValue)       \
private:                                                                                 \
    typedef EnumNamespace EnumNameStruct EThis; /* CONCAT NAME */                        \
protected:                                                                               \
    typedef /*qd::VEnum_<eEnumType>*/ eEnumType TEnum;                                   \
    /*private: typedef qd::VEnum_<eEnumType> VEnum;*/                                    \
public:                                                                                  \
    eEnumType mV; /* member */                                                           \
    inline EnumNameStruct() : mV(static_cast<TEnum>(_DefaultValue)) {                    \
    } /* CONSTRUCTOR */                                                                  \
    template <typename V>                                                                \
    inline EnumNameStruct(V Value) : mV(static_cast<TEnum>(Value)) {                     \
    }                                                                                    \
    inline EnumNameStruct(const EnumNameStruct& r) : mV(r.mV) {                          \
    }                                                                                    \
    inline EnumNameStruct& operator=(const EnumNameStruct& r) throw() {                  \
        mV = r.mV;                                                                       \
        return *this;                                                                    \
    }                                                                                    \
    /*inline EnumNameStruct& operator = (const VEnum& r)  { mV = r.mV; return *this; }*/ \
    template <typename V>                                                                \
    inline constexpr EnumNameStruct& operator=(const V& Value) throw() {                 \
        mV = static_cast<TEnum>(Value);                                                  \
        return *this;                                                                    \
    }                                                                                    \
    inline constexpr operator TEnum() const {                                            \
        return mV;                                                                       \
    }                                                                                    \
    inline EnumNameStruct& operator++() /* ++ prefix */ throw() {                        \
        mV = (TEnum)(mV + 1); /* -V1016 */                                               \
        return *this;                                                                    \
    }                                                                                    \
    inline EnumNameStruct& operator--() throw() {                                        \
        mV = (TEnum)(mV - 1); /* -V1016 */                                               \
        return *this;                                                                    \
    } /* -- prefix */                                                                    \
    inline EnumNameStruct operator++(int) /* postfix ++ */ throw() {                     \
        EnumNameStruct t(mV);                                                            \
        mV = (TEnum)(mV + 1); /* -V1016 */                                               \
        return t;                                                                        \
    }                                                                                    \
    inline EnumNameStruct operator--(int) /* postfix -- */ throw() {                     \
        EnumNameStruct t(mV);                                                            \
        mV = (TEnum)(mV - 1); /* -V1016 */                                               \
        return t;                                                                        \
    }                                                                                    \
    template <typename V>                                                                \
    inline bool operator==(const V& Value) throw() {                                     \
        return (mV == (TEnum)Value);                                                     \
    }                                                                                    \
    template <typename V>                                                                \
    inline bool operator!=(const V& Value) throw() {                                     \
        return !(mV == (TEnum)Value);                                                    \
    }                                                                                    \
    template <typename V>                                                                \
    inline volatile EnumNameStruct& operator=(const V& Value) volatile throw() {         \
        mV = (TEnum)Value;                                                               \
        return *this;                                                                    \
    }
//////////////////////////////////////////////////////////////////////////


// #define ENUM_DECLARE_BASE( EnumNamespace, EnumNameStruct, eEnumType, _DefaultValue ) \
// 	_ENUM_DECLARE_BASE_0( CON(EnumNamespace,::), EnumNameStruct, eEnumType, _DefaultValue )\



//////////////////////////////////////////////////////////////////////////
#define ENUM_DECLARE_FLAGS                                          \
    template <typename V>                                           \
    inline EThis& operator+=(V Value) {                             \
        mV = TEnum((uint32_t)mV | (uint32_t)Value);                 \
        return *this;                                               \
    }                                                               \
    template <typename V>                                           \
    inline EThis& operator-=(V Value) {                             \
        mV = TEnum((uint32_t)mV & ~(uint32_t)Value);                \
        return *this;                                               \
    }                                                               \
    template <typename V>                                           \
    inline EThis& operator|=(V Value) {                             \
        return operator+=(Value);                                   \
    }                                                               \
    template <typename V>                                           \
    inline EThis& operator&=(V Value) {                             \
        assert(Value);                                              \
        mV = TEnum((uint32_t)mV & (uint32_t)Value);                 \
        return *this;                                               \
    }                                                               \
    template <typename V>                                           \
    inline EThis operator-(V Value) const {                         \
        EThis t((uint32_t)mV & ~(uint32_t)Value);                   \
        return t;                                                   \
    }                                                               \
    template <typename V>                                           \
    inline EThis operator|(V Value) const {                         \
        EThis t((uint32_t)mV | (uint32_t)(Value));                  \
        return t;                                                   \
    }                                                               \
    template <typename V>                                           \
    inline EThis operator+(V Value) const {                         \
        EThis t((uint32_t)mV | (uint32_t)(Value));                  \
        return t;                                                   \
    }                                                               \
    template <typename V>                                           \
    inline EThis operator&(V Value) const {                         \
        EThis t((uint32_t)mV&(uint32_t)(Value));                    \
        return t;                                                   \
    }                                                               \
    template <typename V>                                           \
    inline bool is(V Value) const {                                 \
        return ((uint32_t)mV & (uint32_t)Value) == (uint32_t)Value; \
    }                                                               \
    template <typename V>                                           \
    inline bool hasAll(V Value) const {                             \
        return ((uint32_t)mV & (uint32_t)Value) == (uint32_t)Value; \
    }                                                               \
    template <typename V>                                           \
    inline bool hasAny(V Value) const {                             \
        return ((uint32_t)mV & (uint32_t)Value) != 0;               \
    }                                                               \
    template <typename V>                                           \
    inline bool has(V Value) const {                                \
        return hasAny(Value);                                       \
    }                                                               \
    template <typename V>                                           \
    inline EThis& set(V Value) {                                    \
        return operator|=(Value);                                   \
    }                                                               \
    template <typename V>                                           \
    inline EThis& set(V flg, bool bVal) {                           \
        return bVal ? set(flg) : del(flg);                          \
    }                                                               \
    template <typename V>                                           \
    inline EThis& del(V Value) {                                    \
        return operator-=(Value);                                   \
    }                                                               \
    inline bool isEmpty() const {                                   \
        return mV == (TEnum)0;                                      \
    }
