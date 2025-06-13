#pragma once
#include <stdint.h>



#define SCID(name) qd::fnv1aHash(#name)


namespace qd {

// Constants for the FNV-1a hash algorithm (32-bit version)
constexpr uint32_t FNV_OFFSET_BASIS_32 = 0x811c9dc5u; // Initial hash value
constexpr uint32_t FNV_PRIME_32 = 0x01000193u; // Prime multiplier for FNV


// Function to compute the FNV-1a hash for a string
constexpr uint32_t fnv1aHash(const char* str, uint32_t len, uint32_t hash = FNV_OFFSET_BASIS_32)
{
    return (len == 0) ? hash : fnv1aHash(str + 1, len - 1, (hash ^ static_cast<uint8_t>(*str)) * FNV_PRIME_32);
}


constexpr uint32_t fnv1aHash(const char* str)
{
    return fnv1aHash(str, (uint32_t)__builtin_strlen(str));
}


template<typename T>
uint32_t fnv1aHash_(const T* dat, uint32_t size, uint32_t hash = FNV_OFFSET_BASIS_32)
{
    return fnv1aHash((const char*)dat, size, hash);
}


}; // namespace qd
