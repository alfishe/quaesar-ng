#pragma once
#include "qd/base/base.h"
#include "qd/base/IRandom.h"


namespace qd {

// PCG-XSH-RR (32-bit output, 64-bit state)
// Best statistical quality of the bunch. Passes all TestU01 tests.
// Good for: when you need reliable, high-quality randomness.
struct RandomPCG : public BaseRandExt_<RandomPCG> {
    uint64_t state;
    uint64_t inc; // stream selector (must be odd)

    explicit RandomPCG(uint64_t seed = 0x853c49e6748fea9bull, uint64_t seq = 0xda3e39cb94b95bdbull) {
        inc = (seq << 1u) | 1u;
        state = 0;
        nextUInt();
        state += seed;
        nextUInt();
    }

    uint32_t nextUInt() {
        const uint64_t old = state;
        state = old * 6364136223846793005ull + inc;
        const uint32_t xorshifted = static_cast<uint32_t>(((old >> 18u) ^ old) >> 27u);
        const uint32_t rot = static_cast<uint32_t>(old >> 59u);
        return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
    }

    float nextFloat() { return (nextUInt() >> 8) * (1.0f / 16777216.0f); }

    int nextIRange(int lo, int hi) { return lo + static_cast<int>(nextUInt() % static_cast<uint32_t>(hi - lo + 1)); }
};
//////////////////////////////////////////////////////////////////////////



// LCG — Linear Congruential Generator (Numerical Recipes)
// Fast, tiny state (4 bytes). Low quality — low bits have short period.
// Good for: visual noise, particle jitter, non-critical randomness.
struct RandomLCG : public BaseRandExt_<RandomLCG> {
    uint32_t state;

    explicit RandomLCG(uint32_t seed = 1) : state(seed) {}

    uint32_t nextUInt() {
        state = state * 1664525u + 1013904223u;
        return state;
    }

    // [0, 1)
    float nextFloat() {
        return (nextUInt() >> 8) * (1.0f / 16777216.0f);
    }

    // [lo, hi]
    int nextIRange(int lo, int hi) {
        return lo + static_cast<int>(nextUInt() % static_cast<uint32_t>(hi - lo + 1));
    }
};
//////////////////////////////////////////////////////////////////////////


// Xorshift32 (Marsaglia)
// Fast, tiny state (4 bytes). Better quality than LCG.
// Good for: general-purpose demoscene randomness, procedural content.
struct RandomXorshift32 : public BaseRandExt_<RandomXorshift32> {
    uint32_t state;

    explicit RandomXorshift32(uint32_t seed = 1) : state(seed ? seed : 1) {}

    uint32_t nextUInt() {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }

    float nextFloat() {
        return (nextUInt() >> 8) * (1.0f / 16777216.0f);
    }

    int nextIRange(int lo, int hi) {
        return lo + static_cast<int>(nextUInt() % static_cast<uint32_t>(hi - lo + 1));
    }
};


// Xoroshiro64** — 8 bytes state, excellent quality for 32-bit output.
// Good for: anything needing solid randomness with small footprint.
struct RandomXoroshiro64ss : public BaseRandExt_<RandomXoroshiro64ss> {
    uint32_t s0, s1;

    explicit RandomXoroshiro64ss(uint32_t seed = 1) {
        // splitmix32 seeding to decorrelate initial state
        seed += 0x9e3779b9u;
        seed ^= seed >> 16; seed *= 0x85ebca6bu;
        seed ^= seed >> 13; seed *= 0xc2b2ae35u;
        seed ^= seed >> 16;
        s0 = seed;
        seed += 0x9e3779b9u;
        seed ^= seed >> 16; seed *= 0x85ebca6bu;
        seed ^= seed >> 13; seed *= 0xc2b2ae35u;
        seed ^= seed >> 16;
        s1 = seed;
        if (s0 == 0 && s1 == 0) s0 = 1;
    }

    static uint32_t rotl(uint32_t x, int k) { return (x << k) | (x >> (32 - k)); }

    uint32_t nextUInt() {
        const uint32_t r = rotl(s0 * 0x9E3779BBu, 5) * 5u;
        const uint32_t t = s1 ^ s0;
        s0 = rotl(s0, 26) ^ t ^ (t << 9);
        s1 = rotl(t, 13);
        return r;
    }

    float nextFloat() {
        return (nextUInt() >> 8) * (1.0f / 16777216.0f);
    }

    int nextIRange(int lo, int hi) {
        return lo + static_cast<int>(nextUInt() % static_cast<uint32_t>(hi - lo + 1));
    }
};





// Hash-based stateless random (no state, pure function)
// Good for: deterministic noise by coordinate/index (textures, terrain, shader-like usage).
struct RandomHash {
    static uint32_t hash(uint32_t x) {
        x = ((x >> 16) ^ x) * 0x45d9f3bu;
        x = ((x >> 16) ^ x) * 0x45d9f3bu;
        x = (x >> 16) ^ x;
        return x;
    }

    static uint32_t hash2d(uint32_t x, uint32_t y) {
        return hash(x + hash(y));
    }

    static float toFloat(uint32_t h) {
        return (h >> 8) * (1.0f / 16777216.0f);
    }
};


}; // namespace qd
