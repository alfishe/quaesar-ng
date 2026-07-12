#pragma once



// returns byte color to float [0.0f - 1.0f]
inline static float byte_to_float_01(uint8_t x) {
    union {
        float f;
        uint32_t i;
    } u;
    u.f = 32768.0f;
    u.i |= x;
    return (u.f - 32768.0f) * (256.0f / 255.0f);
}
