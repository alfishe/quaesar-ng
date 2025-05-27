#pragma once
#include <qdIce/qdBase/string.h>
#include <qdIce/qdBase/stringId.h>


namespace qd {

class TypeId
{
    StringID m_ID;

public:
    TypeId() {}
    TypeId(const string& type)
        : m_ID(type)
    {}
    TypeId(char const* pType)
        : m_ID(pType)
    {}
    TypeId(const StringID& ID)
        : m_ID(ID)
    {}
    TypeId(uint32_t ID)
        : m_ID(ID)
    {}

    bool IsValid() const { return m_ID.IsValid(); }

    explicit operator uint64_t () const { return m_ID.ToUint(); }
    uint32_t ToUint() const { return m_ID.ToUint(); }
    StringID ToStringID() const { return m_ID; }
    char const* c_str() const { return m_ID.c_str(); }

    bool operator== (TypeId const& rhs) const { return m_ID == rhs.m_ID; }
    bool operator!= (TypeId const& rhs) const { return m_ID != rhs.m_ID; }

    bool operator== (StringID const& rhs) const { return m_ID == rhs; }
    bool operator!= (StringID const& rhs) const { return m_ID != rhs; }
}; // class TypeId

} // namespace qd


//-------------------------------------------------------------------------

namespace eastl {
template<typename T>
struct hash;

template<>
struct hash<qd::TypeId> {
    size_t operator() (qd::TypeId const& ID) const { return ID.ToUint(); }
};
} // namespace eastl
