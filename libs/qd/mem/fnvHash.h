#pragma once
#include <cstdint>
#include "qd/base/compiler.h"


#define SCID(name) qd::fnv1aHash(#name)

namespace qd {

// Constants for the FNV-1a hash algorithm (32-bit version)
constexpr uint32_t FNV_OFFSET_BASIS_32 = 0x811c9dc5u; // Initial hash value
constexpr uint32_t FNV_PRIME_32 = 0x01000193u; // Prime multiplier for FNV


// Function to compute the FNV-1a hash for a string
constexpr uint32_t fnv1aHash2(const char* str, size_t len, uint32_t hash = FNV_OFFSET_BASIS_32) {
    // clang-format off
    QD_PUSH_CLANG_WARNING("-Wunsafe-buffer-usage");
    return (len == 0) ? hash : fnv1aHash2(str + 1, len - 1, (hash ^ static_cast<uint8_t>(*str)) * FNV_PRIME_32);
    QD_POP_CLANG_WARNING()
    // clang-format on
}
constexpr uint32_t fnv1aHash(const char* str) {
    return fnv1aHash2(str, (uint32_t)__builtin_strlen(str));
}

template<typename T>
uint32_t fnv1aHash_(const T* dat, size_t size, uint32_t hash = FNV_OFFSET_BASIS_32) {
    return fnv1aHash2((const char*)dat, size, hash);
}

// Alternative iterative version of FNV-1a hash for better performance
constexpr uint32_t fnv1aHashFor(const char* s, size_t n) {
    uint32_t h = 0x811c9dc5u;
    for (size_t i = 0; i < n; i++) {
        h ^= (uint8_t)s[i];
        h *= 0x01000193u;
    }
    return h;
}

constexpr uint64_t fnv1aHashFor64(const char* s, size_t n) {
    uint64_t h = 14695981039346656037ULL;
    for (size_t i = 0; i < n; i++) {
        h ^= (uint8_t)s[i];
        h *= 1099511628211ULL;
    }
    return h;
}

}; // namespace qd
