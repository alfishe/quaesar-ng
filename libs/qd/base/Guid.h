#pragma once
#include "qd/base/IRandom.h"
#include "qd/debug/assert.h"
#include "qd/stl/algorithm.h"
#include "qd/stl/string.h"
#include <cstdint>
#include <cstdio> // for snprintf


namespace qd {
class IRandom;
class Guid128;
class Guid64;
class Guid32;



//////////////////////////////////////////////////////////////////////////
class Guid128
{
    typedef Guid128 TThis;

public:
    union {
        uint32_t m_ID[4]{};
        struct {
            uint32_t ID0, ID1, ID2, ID3;
        };
        struct {
            uint64_t m_ID64[2];
        };
    };

public:
    Guid128() = default;

    Guid128(const Guid128& rhs) {
        m_ID64[0] = rhs.m_ID64[0];
        m_ID64[1] = rhs.m_ID64[1];
    }

    Guid128(uint32_t id0, uint32_t id1, uint32_t id2, uint32_t id3)
        : ID0(id0)
        , ID1(id1)
        , ID2(id2)
        , ID3(id3) {}

    static constexpr uint32_t hexCharVal(char c) {
        return (c >= '0' && c <= '9') ? (c - '0') : (c >= 'a' && c <= 'f') ? (c - 'a' + 10) : (c >= 'A' && c <= 'F') ? (c - 'A' + 10) : 0;
    }
    static constexpr uint32_t parseHex32(const char* s, int count) {
        uint32_t r = 0;
        for (int i = 0; i < count; ++i)
            r = (r << 4) | hexCharVal(s[i]);
        return r;
    }

    // Guid128("00000000-1111-2222-3333-123456789021")
    constexpr Guid128(const char* str)
        : ID0(parseHex32(str, 8))
        , ID1((parseHex32(str + 9, 4) << 16) | parseHex32(str + 14, 4))
        , ID2((parseHex32(str + 19, 4) << 16) | parseHex32(str + 24, 4))
        , ID3(parseHex32(str + 28, 8)) {}

public:
    static inline TThis makeRandom(qd::IRandom* rnd/*=nullptr*/) {
        TThis id;
        id.randomize(rnd);
        return id;
    }
    static inline constexpr TThis makeFromStr(const char* str) {
        return TThis(str);
    }

    uint64_t getHashCode() const { return m_ID64[0] ^ m_ID64[1]; }

    static TThis Null() { return TThis(); }

    inline bool empty() const { return m_ID64[0] == 0 && m_ID64[1] == 0; }

    inline operator bool () const { return !empty(); }
    constexpr bool operator== (const TThis& rhs) const { return (m_ID64[0] == rhs.m_ID64[0]) && (m_ID64[1] == rhs.m_ID64[1]); }
    constexpr bool operator!= (const TThis& rhs) const { return (m_ID64[0] != rhs.m_ID64[0]) && (m_ID64[1] != rhs.m_ID64[1]); }
    constexpr bool operator< (const TThis& rhs) const { return (m_ID64[0] < rhs.m_ID64[0]) && (m_ID64[1] < rhs.m_ID64[1]); }
    constexpr bool operator> (const TThis& rhs) const { return (m_ID64[0] > rhs.m_ID64[0]) && (m_ID64[1] > rhs.m_ID64[1]); }

    void toString(char* buffer, size_t bufferSize) const {
        snprintf(buffer, bufferSize, "%08X-%04X-%04X-%04X-%08X", ID0, ID1 >> 16, ID1 & 0xFFFF, ID2 >> 16,
            (ID2 & 0xFFFF) << 16 | (ID3 >> 16));
    }

    qtd::string toString() const {
        char buffer[37];
        toString(buffer, sizeof(buffer));
        return buffer;
    }

    void randomize(qd::IRandom* rnd) {
        assert(rnd);
        for (int i = 0; i < 4; ++i)
            m_ID[i] = rnd->getUInt();
    }

}; // struct Guid128
//////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////
class Guid32
{
    typedef Guid32 TThis;

public:
    uint32_t m_id = 0; // mutable

public:
    Guid32() = default;
    constexpr Guid32(const Guid32& rhs)
        : m_id(rhs.m_id) {}
    template<typename TInt>
    constexpr Guid32(const TInt& id)
        : m_id(static_cast<uint32_t>(id)) {}


public:
    TThis& randomize() const;

    // Create Random GUID
    static inline TThis MakeRandom() {
        TThis id;
        id.randomize();
        return id;
    }

    constexpr static TThis Construct(const uint32_t& id) { return TThis(id); }

    constexpr static TThis Null() { return TThis::Construct(0); }

    constexpr bool IsNull() const { return m_id == 0; }

    constexpr bool operator== (const TThis& rhs) const { return m_id == rhs.m_id; }
    constexpr bool operator!= (const TThis& rhs) const { return m_id != rhs.m_id; }
    constexpr bool operator< (const TThis& rhs) const { return m_id < rhs.m_id; }
    constexpr bool operator> (const TThis& rhs) const { return m_id > rhs.m_id; }
    constexpr bool operator<= (const TThis& rhs) const { return m_id <= rhs.m_id; }
    constexpr bool operator>= (const TThis& rhs) const { return m_id >= rhs.m_id; }

    constexpr operator uint32_t () const { return m_id; }

}; // class Guid32
   //////////////////////////////////////////////////////////////////////////




//////////////////////////////////////////////////////////////////////////
class Guid64
{
    typedef Guid64 TThis;

public:
    union {
        uint64_t m_id = 0; // mutable
        struct {
            uint32_t ID0, ID1;
        };
        struct {
            uint32_t FileID, LocalID;
        };
    };

public:
    Guid64() = default;
    constexpr Guid64(const Guid64& rhs)
        : m_id(rhs.m_id) {}
    constexpr Guid64(const uint32_t& id)
        : m_id(id) {}

public:
    TThis& randomize() const;

    // Create Random GUID
    static inline TThis MakeRandom() {
        TThis id;
        id.randomize();
        return id;
    }

    constexpr static TThis Construct(const uint32_t& id) { return TThis(id); }

    constexpr static TThis Null() { return TThis::Construct(0); }

    constexpr bool IsNull() const { return m_id == 0; }

    constexpr bool operator== (const TThis& rhs) const { return m_id == rhs.m_id; }
    constexpr bool operator!= (const TThis& rhs) const { return m_id != rhs.m_id; }
    constexpr bool operator< (const TThis& rhs) const { return m_id < rhs.m_id; }
    constexpr bool operator> (const TThis& rhs) const { return m_id > rhs.m_id; }
    constexpr bool operator<= (const TThis& rhs) const { return m_id <= rhs.m_id; }
    constexpr bool operator>= (const TThis& rhs) const { return m_id >= rhs.m_id; }

    constexpr operator uint64_t () const { return m_id; }

}; // class Guid64
   //////////////////////////////////////////////////////////////////////////



}; // namespace qd
//////////////////////////////////////////////////////////////////////////


#if QTD_IS_EASTL
namespace eastl
#else
namespace std
#endif
{
template<typename T>
struct hash;

template<>
struct hash<qd::Guid128> {
    size_t operator() (qd::Guid128 const& ID) const { return (uint64_t)ID; }
};
}; // namespace eastl
