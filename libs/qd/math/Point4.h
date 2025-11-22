#pragma once
#include "qd/math/mathBase.h"
#include <cmath>

namespace qd {

// clang-format off
template<typename TInt>
class TPoint4
{
    typedef TPoint4<TInt> TThis;

public:
    TInt x, y, z, w;

    TPoint4() = default;
    TPoint4(const TPoint4&) = default;
    TPoint4(TInt ax, TInt ay, TInt az, TInt aw) { x = ax; y = ay; z = az; w = aw; }
    /// constructs from #TInt array
    enum CtorPtrMark { CTOR_FROM_PTR = 1 };
    TPoint4(const TInt* p, CtorPtrMark /*check*/) { x = p[0]; y = p[1]; z = p[2]; w = p[3]; }

    template<typename T2>
    explicit TPoint4(const TPoint4<T2>& p) { x = (TInt)p.x; y = (TInt)p.y; z = (TInt)p.z; w = (TInt)p.w; }

    TPoint4& operator= (const TPoint4&) = default;

    void zero() { x = 0; y = 0; z = 0; w = 0; }
    void set(TInt _x, TInt _y, TInt _z, TInt _w) { x = _x; y = _y; z = _z; w = _w; }

    const TInt& operator[] (int i) const { return (&x)[i]; }
    TInt& operator[] (int i) { return (&x)[i]; }

    TThis operator- () const { return TThis(-x, -y, -z, -w); }
    TThis operator+ () const { return *this; }

    TThis operator+ (const TThis& a) const { return TThis(x + a.x, y + a.y, z + a.z, w + a.w); }
    TThis operator- (const TThis& a) const { return TThis(x - a.x, y - a.y, z - a.z, w - a.w); }
    float dot(const TThis& a) const { return x * a.x + y * a.y + z * a.z + w * a.w; }

    /// 3D cross product on (x,y,z), w set to 0
    TThis operator% (const TThis& a) const { return TThis(y * a.z - z * a.y, z * a.x - x * a.z, x * a.y - y * a.x, 0); }

    TThis operator+ (TInt a) const { return TThis(x + a, y + a, z + a, w + a); }
    TThis operator- (TInt a) const { return TThis(x - a, y - a, z - a, w - a); }
    TThis operator* (TInt a) const { return TThis(x * a, y * a, z * a, w * a); }
    TThis operator/ (TInt a) const { return operator* (1.0f / a); }

    TThis& operator+= (const TThis& a) { x += a.x; y += a.y; z += a.z; w += a.w; return *this; }
    TThis& operator-= (const TThis& a) { x -= a.x; y -= a.y; z -= a.z; w -= a.w; return *this; }
    TThis& operator*= (TInt a) { x *= a; y *= a; z *= a; w *= a; return *this; }
    TThis& operator/= (TInt a) { return operator*= (1.0f / a); }

    bool operator== (const TThis& a) const { return (x == a.x && y == a.y && z == a.z && w == a.w); }
    bool operator!= (const TThis& a) const { return !(*this == a); }

    float lengthSq() const { return x * x + y * y + z * z + w * w; }
    float length() const { return sqrtf(lengthSq()); }
    void normalize() { *this *= safeinv(length()); }

    // Swizzle / helper constructors (extended; w defaults to 0)
    template<class T> static TPoint4<T> xyz(const T& a) { return TPoint4<T>(a.x, a.y, a.z, 0); }
    template<class T> static TPoint4<T> xzy(const T& a) { return TPoint4<T>(a.x, a.z, a.y, 0); }
    template<class T> static TPoint4<T> x0y(const T& a) { return TPoint4<T>(a.x, 0,   a.y, 0); }
    template<class T> static TPoint4<T> x0z(const T& a) { return TPoint4<T>(a.x, 0,   a.z, 0); }
    template<class T> static TPoint4<T> xy0(const T& a) { return TPoint4<T>(a.x, a.y, 0,    0); }
    template<class T> static TPoint4<T> xz0(const T& a) { return TPoint4<T>(a.x, a.z, 0,    0); }
    template<class T> static TPoint4<T> xVy(const T& a, float v) { return TPoint4<T>(a.x, v, a.y, 0); }
    template<class T> static TPoint4<T> xVz(const T& a, float v) { return TPoint4<T>(a.x, v, a.z, 0); }
    template<class T> static TPoint4<T> xyV(const T& a, float v) { return TPoint4<T>(a.x, a.y, v, 0); }
    template<class T> static TPoint4<T> xzV(const T& a, float v) { return TPoint4<T>(a.x, a.z, v, 0); }
    template<class T> static TPoint4<T> rgb(const T& a) { return TPoint4<T>(a.r, a.g, a.b, 0); }
    template<class T> static TPoint4<T> xyzw(const T& a) { return TPoint4<T>(a.x, a.y, a.z, a.w); }

    // Setters
    template<class T> void set_xyz(const T& a) { x = a.x; y = a.y; z = a.z; w = 0; }
    template<class T> void set_xzy(const T& a) { x = a.x; y = a.z; z = a.y; w = 0; }
    template<class T> void set_x0y(const T& a) { x = a.x; y = 0;   z = a.y; w = 0; }
    template<class T> void set_x0z(const T& a) { x = a.x; y = 0;   z = a.z; w = 0; }
    template<class T> void set_xy0(const T& a) { x = a.x; y = a.y; z = 0;   w = 0; }
    template<class T> void set_xz0(const T& a) { x = a.x; y = a.z; z = 0;   w = 0; }
    template<class T> void set_xVy(const T& a, float v) { x = a.x; y = v; z = a.y; w = 0; }
    template<class T> void set_xVz(const T& a, float v) { x = a.x; y = v; z = a.z; w = 0; }
    template<class T> void set_xyV(const T& a, float v) { x = a.x; y = a.y; z = v; w = 0; }
    template<class T> void set_xzV(const T& a, float v) { x = a.x; y = a.z; z = v; w = 0; }
    template<class T> void set_rgb(const T& a) { x = a.r; y = a.g; z = a.b; w = 0; }
    template<class T> void set_xyzw(const T& a) { x = a.x; y = a.y; z = a.z; w = a.w; }
}; // class TPoint4
//////////////////////////////////////////////////////////////////////////

template<class T> float dot(const TPoint4<T>& a, const TPoint4<T>& b) { return a.dot(b); }
template<class T> TPoint4<T> cross3(const TPoint4<T>& a, const TPoint4<T>& b) { return a % b; }

template<class T> TPoint4<T> operator*(float a, const TPoint4<T>& p) { return TPoint4<T>(p.x * a, p.y * a, p.z * a, p.w * a); }
template<class T> float lengthSq(const TPoint4<T>& a) { return a.x * a.x + a.y * a.y + a.z * a.z + a.w * a.w; }
template<class T> float length(const TPoint4<T>& a) { return sqrtf(lengthSq(a)); }
template<class T> TPoint4<T> normalize(const TPoint4<T>& a) { return safeinv(length(a)) * a; }
template<class T> TPoint4<T> mul(const TPoint4<T>& a, const TPoint4<T>& b) { return TPoint4<T>(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w); }
template<class T> TPoint4<T> div(const TPoint4<T>& a, const TPoint4<T>& b) { return TPoint4<T>(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w); }

template<class T> TPoint4<T> floor(const TPoint4<T>& a) { return TPoint4<T>(floorf(a.x), floorf(a.y), floorf(a.z), floorf(a.w)); }
template<class T> TPoint4<T> ceil(const TPoint4<T>& a) { return TPoint4<T>(ceilf(a.x), ceilf(a.y), ceilf(a.z), ceilf(a.w)); }
template<class T> TPoint4<T> round(const TPoint4<T>& a) { return TPoint4<T>(roundf(a.x), roundf(a.y), roundf(a.z), roundf(a.w)); }
template<class T> TPoint4<T> frac(const TPoint4<T>& a) { return a - floor(a); }

template<class T> TPoint4<T> abs(const TPoint4<T>& a) { return TPoint4<T>(fabsf(a.x), fabsf(a.y), fabsf(a.z), fabsf(a.w)); }
template<class T> TPoint4<T> sqrt(const TPoint4<T>& a) { return TPoint4<T>(sqrtf(a.x), sqrtf(a.y), sqrtf(a.z), sqrtf(a.w)); }

#undef max
#undef min
template<class T> TPoint4<T> max(const TPoint4<T>& a, const TPoint4<T>& b) { return TPoint4<T>(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z), max(a.w, b.w)); }
template<class T> TPoint4<T> min(const TPoint4<T>& a, const TPoint4<T>& b) { return TPoint4<T>(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z), min(a.w, b.w)); }
template<class T> TPoint4<T> max(const TPoint4<T>& a, const float b) { return TPoint4<T>(max(a.x, b), max(a.y, b), max(a.z, b), max(a.w, b)); }
template<class T> TPoint4<T> min(const TPoint4<T>& a, const float b) { return TPoint4<T>(min(a.x, b), min(a.y, b), min(a.z, b), min(a.w, b)); }

using IPoint4 = TPoint4<int>;
using Point4  = TPoint4<float>;
using Size4   = TPoint4<int>;
using Size4F  = TPoint4<float>;

// clang-format on
}; // namespace qd
