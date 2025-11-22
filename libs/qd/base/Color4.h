#pragma once
#include "color.h"


namespace qd {

class Color4
{
    // clang-format off
public:
    float r, g, b, a;

    Color4() = default;

    Color4(float rr, float gg, float bb, float aa = 1.f) {
        r = rr; g = gg; b = bb; a = aa;
    }

    Color4(Color c) {
        r = float(c.r) / 255.f; g = float(c.g) / 255.f; b = float(c.b) / 255.f; a = float(c.a) / 255.f;
    }

    void set(float k) {
        r = k; g = k; b = k; a = k;
    }
    void set(float _r, float _g, float _b, float _a) {
        r = _r; g = _g; b = _b; a = _a;
    }
    void zero() { set(0); }

    float& operator[] (int i) { return (&r)[i]; }
    const float& operator[] (int i) const { return (&r)[i]; }

    Color4 operator+ () const { return *this; }
    Color4 operator- () const { return Color4(-r, -g, -b, -a); }
    Color4 operator* (float k) const { return Color4(r * k, g * k, b * k, a * k); }
    Color4 operator/ (float k) const { return operator* (1.0f / k); }
    Color4 operator* (const Color4& c) const { return Color4(r * c.r, g * c.g, b * c.b, a * c.a); }
    Color4 operator/ (const Color4& c) const { return Color4(r / c.r, g / c.g, b / c.b, a / c.a); }
    Color4 operator+ (const Color4& c) const { return Color4(r + c.r, g + c.g, b + c.b, a + c.a); }
    Color4 operator- (const Color4& c) const { return Color4(r - c.r, g - c.g, b - c.b, a - c.a); }
    Color4& operator+= (const Color4& c) {
        r += c.r; g += c.g; b += c.b; a += c.a;
        return *this;
    }
    Color4& operator-= (const Color4& c) {
        r -= c.r; g -= c.g; b -= c.b; a -= c.a;
        return *this;
    }
    Color4& operator*= (const Color4& c) {
        r *= c.r; g *= c.g; b *= c.b; a *= c.a;
        return *this;
    }
    Color4& operator/= (const Color4& c) {
        r /= c.r; g /= c.g; b /= c.b; a /= c.a;
        return *this;
    }
    Color4& operator*= (float k) {
        r *= k; g *= k; b *= k; a *= k;
        return *this;
    }
    Color4& operator/= (float k) { return operator*= (1.0f / k); }

    bool operator== (const Color4& c) const { return (r == c.r && g == c.g && b == c.b && a == c.a); }
    bool operator!= (const Color4& c) const { return (r != c.r || g != c.g || b != c.b || a != c.a); }

    void clamp0() {
        if (r < 0) r = 0;
        if (g < 0) g = 0;
        if (b < 0) b = 0;
        if (a < 0) a = 0;
    }
    void clamp1() {
        if (r > 1) r = 1;
        if (g > 1) g = 1;
        if (b > 1) b = 1;
        if (a > 1) a = 1;
    }
    void clamp01() {
        if (r < 0) r = 0; else if (r > 1) r = 1;
        if (g < 0) g = 0; else if (g > 1) g = 1;
        if (b < 0) b = 0; else if (b > 1) b = 1;
        if (a < 0) a = 0; else if (a > 1) a = 1;
    }

    Color toColor() const {
        Color out;
        out.r = (uint8_t)(clamp(static_cast<uint32_t>(r * 255.0f), 0u, 255u));
        out.g = (uint8_t)(clamp(static_cast<uint32_t>(g * 255.0f), 0u, 255u));
        out.b = (uint8_t)(clamp(static_cast<uint32_t>(b * 255.0f), 0u, 255u));
        out.a = (uint8_t)(clamp(static_cast<uint32_t>(a * 255.0f), 0u, 255u));
        return out;
    }
    // clang-format on

}; // class Color4

}; // namespace qd
