#pragma once
#include "qd/base/base.h"


//////////////////////////////////////////////////////////////////////////
// EnumNamespace = "Namespace::ToEnum::"
// example: ENUM_DECLARE_BASE(app::shortcut::, EShortcutId, Type, UNDEF);
//
// clang-format off
#define ENUM_DECLARE_BASE(EnumNamespace, EnumNameStruct, eEnumType, _DefaultValue)                \
private:                                                                                          \
    typedef EnumNamespace EnumNameStruct EThis; /* CONCAT NAME */                                 \
protected:                                                                                        \
    typedef eEnumType TEnum;                                                                      \
                                                                                                  \
public:                                                                                           \
    eEnumType mV; /* member */                                                                    \
    constexpr inline EnumNameStruct() : mV(static_cast<TEnum>(_DefaultValue)) {}                  \
    template<typename V>                                                                          \
    constexpr inline EnumNameStruct(V Value) : mV(static_cast<TEnum>(Value)) {}                   \
    constexpr inline EnumNameStruct(const EnumNameStruct& r) : mV(r.mV) {}                        \
    constexpr inline EnumNameStruct& operator= (const EnumNameStruct& r) throw() {                \
        mV = r.mV;                                                                                \
        return *this;                                                                             \
    }                                                                                             \
    template<typename V>                                                                          \
    inline constexpr EnumNameStruct& operator= (const V& Value) throw() {                         \
        mV = static_cast<TEnum>(Value);                                                           \
        return *this;                                                                             \
    }                                                                                             \
    inline constexpr operator TEnum () const throw() { return mV; }                               \
    inline constexpr TEnum get() const throw() { return mV; }                                     \
    inline constexpr EnumNameStruct& operator++ () /* ++ prefix */ throw() {                      \
        mV = (TEnum)(mV + 1); /* -V1016 */                                                        \
        return *this;                                                                             \
    }                                                                                             \
    inline constexpr EnumNameStruct& operator-- () throw() {  /* --prefix */                      \
        mV = (TEnum)(mV - 1); /* -V1016 */                                                        \
        return *this;                                                                             \
    }                                                                                             \
    inline constexpr EnumNameStruct operator++ (int) /* postfix ++ */ throw() {                   \
        EnumNameStruct t(mV);                                                                     \
        mV = (TEnum)(mV + 1); /* -V1016 */                                                        \
        return t;                                                                                 \
    }                                                                                             \
    inline constexpr EnumNameStruct operator-- (int) /* postfix -- */ throw() {                   \
        EnumNameStruct t(mV);                                                                     \
        mV = (TEnum)(mV - 1); /* -V1016 */                                                        \
        return t;                                                                                 \
    }                                                                                             \
    template<typename V>                                                                          \
    inline constexpr bool operator== (const V& Value) throw() { return (mV == (TEnum)Value);}     \
    template<typename V>                                                                          \
    inline constexpr bool operator!= (const V& Value) throw() {                                   \
        return !(mV == (TEnum)Value);                                                             \
    }                                                                                             \
    template<typename V>                                                                          \
    inline constexpr volatile EnumNameStruct& operator= (const V& Value) volatile throw() {       \
        mV = (TEnum)Value;                                                                        \
        return *this;                                                                             \
    }
// clang-format on
//////////////////////////////////////////////////////////////////////////





//////////////////////////////////////////////////////////////////////////
#define ENUM_DECLARE_FLAGS()                                        \
    template<typename V>                                            \
    inline constexpr EThis& operator+= (V Value)                    \
    {                                                               \
        mV = TEnum((uint32_t)mV | (uint32_t)Value);                 \
        return *this;                                               \
    }                                                               \
    template<typename V>                                            \
    inline constexpr EThis& operator-= (V Value)                    \
    {                                                               \
        mV = TEnum((uint32_t)mV & ~(uint32_t)Value);                \
        return *this;                                               \
    }                                                               \
    template<typename V>                                            \
    inline constexpr EThis& operator|= (V Value)                    \
    {                                                               \
        return operator+= (Value);                                  \
    }                                                               \
    template<typename V>                                            \
    inline constexpr EThis& operator&= (V Value)                    \
    {                                                               \
        /*assert(Value);*/                                          \
        mV = TEnum((uint32_t)mV & (uint32_t)Value);                 \
        return *this;                                               \
    }                                                               \
    template<typename V>                                            \
    inline constexpr EThis operator- (V Value) const                \
    {                                                               \
        EThis t((uint32_t)mV & ~(uint32_t)Value);                   \
        return t;                                                   \
    }                                                               \
    template<typename V>                                            \
    inline constexpr EThis operator| (V Value) const                \
    {                                                               \
        EThis t((uint32_t)mV | (uint32_t)(Value));                  \
        return t;                                                   \
    }                                                               \
    template<typename V>                                            \
    inline constexpr EThis operator+ (V Value) const                \
    {                                                               \
        EThis t((uint32_t)mV | (uint32_t)(Value));                  \
        return t;                                                   \
    }                                                               \
    template<typename V>                                            \
    inline constexpr EThis operator& (V Value) const                \
    {                                                               \
        EThis t((uint32_t)mV&(uint32_t)(Value));                    \
        return t;                                                   \
    }                                                               \
    template<typename V>                                            \
    inline constexpr bool is(V Value) const                         \
    {                                                               \
        return ((uint32_t)mV & (uint32_t)Value) == (uint32_t)Value; \
    }                                                               \
    template<typename V>                                            \
    inline constexpr bool hasAll(V Value) const                     \
    {                                                               \
        return ((uint32_t)mV & (uint32_t)Value) == (uint32_t)Value; \
    }                                                               \
    template<typename V>                                            \
    inline constexpr bool hasAny(V Value) const                     \
    {                                                               \
        return ((uint32_t)mV & (uint32_t)Value) != 0;               \
    }                                                               \
    template<typename V>                                            \
    inline constexpr bool has(V Value) const                        \
    {                                                               \
        return hasAny(Value);                                       \
    }                                                               \
    template<typename V>                                            \
    inline constexpr EThis& set(V Value)                            \
    {                                                               \
        return operator|= (Value);                                  \
    }                                                               \
    template<typename V>                                            \
    inline constexpr EThis& set(V flg, bool bVal)                   \
    {                                                               \
        return bVal ? set(flg) : del(flg);                          \
    }                                                               \
    template<typename V>                                            \
    inline constexpr EThis& del(V Value)                            \
    {                                                               \
        return operator-= (Value);                                  \
    }                                                               \
    inline constexpr bool isEmpty() const                           \
    {                                                               \
        return mV == (TEnum)0;                                      \
    }



#define ENUM_TAKE_ITEM_1(name, ...)      name,
#define ENUM_TAKE_ITEM_2(name, str, ...) str,
