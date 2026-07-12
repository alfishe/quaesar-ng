#pragma once
#include "qd/base/baseTypes.h"
#include "qd/math/float.h"
#include "qd/math/mathBase.h"


namespace qd {


class Color
{
public:
    // clang-format off
    enum EColor : uint32_t {
        // AABBGGRR     AABBGGRR
        BLACK     = 0xFF000000ul,
        RED       = 0xFF0000FFul,
        GREEN     = 0xFF00FF00ul,
        BLUE      = 0xFFFF0000ul,
        WHITE     = 0xFFFFFFFFul,
        YELLOW    = 0xFF00FFFFul,
        CYAN      = 0xFFFFFF00ul,
        MAGENTA   = 0xFFFF00FFul,
        GRAY      = 0xFF808080ul,
        GRAY75    = 0xFFC0C0C0ul,
        GRAY25    = 0xFF404040ul,
        BLUE75    = 0xFFC00000ul,
        BLUE50    = 0xFF800000ul,
        BLUE25    = 0xFF400000ul,
        RED75     = 0xFF0000C0ul,
        RED50     = 0xFF000080ul,
        RED25     = 0xFF000040ul,
        RED_DARK  = 0xFF36369Dul,
        GREEN50   = 0xFF008000ul,
        GREEN75   = 0xFF00C000ul,
        GREEN25   = 0xFF004000ul,
        MAGENTA75 = 0xFFC000C0ul,
        MAGENTA50 = 0xFF800080ul,
        MAGENTA25 = 0xFF400040ul,
        YELLOW75 = 0xFF00C0C0ul,
        YELLOW50 = 0xFF008080ul,
        YELLOW25 = 0xFF004040ul,
        YELLOWGREEN = 0xFF32CD9Aul,
        CYAN75      = 0xFFC0C000ul,
        CYAN50      = 0xFF808000ul,
        CYAN25      = 0xFF404000ul,
        ORANGE      = 0xFF00A5FFul,
        ORANGERED   = 0xFF0045FFul,
        ORCHID      = 0xFFD670DAul,
        VIOLET      = 0xFFEE82EEul,
        VIOLETRED   = 0xFF9020D0ul,
        VIOLETBLUE  = 0xFFE22B8Aul,
        TOMATO      = 0xFF4763FFul,
        MIDNIGHTBLUE= 0xFF701919ul,
        INDIGO      = 0xFF82004Bul,
        KHAKI       = 0xFF8CE6F0ul,
        AQUAMARINE  = 0xFFD4FF7Ful,
        ALICEBLUE   = 0xFFFFF8F0ul,
    };

public:
    QD_PUSH_VC_WARNING(4201) // nameless struct/union
    union {
        struct {
            uint8_t r, g, b, a;
        };
        uint32_t u;
        uint32_t mColor;
        Color::EColor mEColor;
    };
    QD_POP_VC_WARNING()

    using TThis = Color;

    inline Color()
        : mColor((uint32_t)Color::WHITE)
    {}

    inline Color(uint32_t d)
        : mColor(d)
    {}

    inline Color(Color::EColor d)
        : mColor((uint32_t)d)
    {}

    inline Color(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a = 255u) { set(_r, _g, _b, _a); }

    ~Color() = default;

    inline uint32_t getU32() const { return mColor; }

    inline void set(uint32_t Color) { mColor = Color; }
    inline void set(Color::EColor Clr) { mColor = (uint32_t)Clr; }

    inline constexpr void set(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a = 255u) {
        r = _r; g = _g; b = _b; a = _a;
    }
    inline void setF(float _r, float _g, float _b, float _a = 1.0f) {
        setRedF(_r); setGreenF(_g); setBlueF(_b); setAlphaF(_a);
    }
    uint32_t getColor32() const { return mColor; }
    void setColor32(uint32_t Color) { set(Color); }

    static inline TThis makeFromF(float _r, float _g, float _b, float _a = 1.0f) {
        TThis c; c.setF(_r, _g, _b, _a);
        return c;
    }

    inline uint8_t getR() const { return r; }
    inline uint8_t getG() const { return g; }
    inline uint8_t getB() const { return b; }
    inline uint8_t getA() const { return a; }

    inline TThis& setR(uint8_t _r) { r = _r; return *this; }
    inline TThis& setG(uint8_t _g) { g = _g; return *this; }
    inline TThis& setB(uint8_t _b) { b = _b; return *this; }
    inline TThis& setA(uint8_t _a) { a = _a; return *this; }

    inline float getRF() const { return _byte_to_float_01(r); }
    inline float getGF() const { return _byte_to_float_01(g); }
    inline float getBF() const { return _byte_to_float_01(b); }
    inline float getAlphaF() const { return _byte_to_float_01(a); }

    inline TThis& setRedF(float _r)   { r = _clamp8(_r * 255.f); return *this; }
    inline TThis& setGreenF(float _g) { g = _clamp8(_g * 255.f); return *this; }
    inline TThis& setBlueF(float _b)  { b = _clamp8(_b * 255.f); return *this; }
    inline TThis& setAlphaF(float _a) { a = _clamp8(_a * 255.f); return *this; }

    inline void setWhite() { r = g = b = a = 255u; }
    inline void setBlack() { r = g = b = 0; a = 255u; }
    void setWhiteAlpha(uint8_t _a = 255) { r = g = b = 255; a = _a; }
    void setBlackAlpha(uint8_t _a = 255) { r = g = b = 0; a = _a; }

    static TThis makeWhite(uint8_t _a = 255) { TThis c; c.setWhiteAlpha(_a); return c; }
    static TThis makeBlack(uint8_t _a = 255) { TThis c; c.setBlackAlpha(_a); return c; }

    // Operators.
    template<typename TColorType> inline bool operator == (const TColorType& C) const { return mColor == (TThis)C; }
    template<typename TColorType> inline bool operator != (const TColorType& C) const { return mColor != (TThis)C; }
    void inline operator += (const TThis& c) { add(c); }

    void add(const TThis& C) {
        r = _min8((int)r + C.r, 255);
        g = _min8((int)g + C.g, 255);
        b = _min8((int)b + C.b, 255);
        a = _min8((int)a + C.a, 255);
    }


    void toColorF(float& _r, float& _g, float& _b, float& _a) const
    {
        _r = r * (1.f / 255.f);
        _g = g * (1.f / 255.f);
        _b = b * (1.f / 255.f);
        _a = a * (1.f / 255.f);
    }

    inline operator uint32_t () const { return getU32(); }

private:
static inline uint8_t _clamp8(const float v) {
    if (v < 0.f) return 0u;
    if (v > 255.f) return 255u;
    return (uint8_t)v;
}

// returns byte color to float [0.0f - 1.0f]
static inline float _byte_to_float_01(uint8_t x) {
    union {
        float f; uint32_t i;
    } u;
    u.f = 32768.0f;
    u.i |= x;
    return (u.f - 32768.0f) * (256.0f / 255.0f);
}

static inline uint8_t _min8(int __a, int __b) {
    return (__a < __b) ? (uint8_t)__a : (uint8_t)__b;
}

}; // class Color
//////////////////////////////////////////////////////////////////////////

}; // namespace qd
