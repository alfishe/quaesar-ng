#pragma once
#include <qd/Debug/assert.h>
#include <typeinfo>


namespace qd {



struct StdTypeId {
    const std::type_info* m_pType = nullptr;
    uint32_t m_SizeOf = 0;

public:
    constexpr StdTypeId() = default;
    constexpr StdTypeId(const std::type_info& ti, uint32_t size_of)
        : m_pType(&ti)
        , m_SizeOf(size_of)
    {}

    constexpr StdTypeId(const StdTypeId& r)
        : m_pType(r.m_pType)
        , m_SizeOf(r.m_SizeOf)
    {}
    constexpr StdTypeId& operator= (const StdTypeId&) = default;

    bool isValid() const { return m_pType != 0; }

    const std::type_info* getTypePtr() const { return m_pType; }
    const std::type_info& getType() const
    {
        assert(isValid());
        return *m_pType;
    }
    uint32_t getSizeOf() const { return m_SizeOf; }

    bool operator== (const StdTypeId& r) const { return m_pType == r.m_pType /*&& m_SizeOf == r.m_SizeOf*/; }
    bool operator!= (const StdTypeId& r) const { return !(m_pType == r.m_pType) /*|| m_SizeOf != r.m_SizeOf*/; }

    const char* name() const
    {
        assert(isValid());
        return m_pType->name();
    }
}; // struct StdTypeInfo
//////////////////////////////////////////////////////////////////////////


template<typename T>
inline constexpr StdTypeId makeStdTypeId_()
{
    return StdTypeId(typeid(T), sizeof(T));
}


template<>
inline constexpr StdTypeId makeStdTypeId_<void>()
{
    return StdTypeId(typeid(void), 0);
}


} // namespace qd
