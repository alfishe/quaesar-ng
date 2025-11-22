#pragma once
#include "qd/math/mathBase.h"
#include <cmath>


namespace qd {

// clang-format off
template<typename TInt>
class TPoint3
{
    typedef TPoint3<TInt> TThis;

public:
    TInt x, y, z;

    TPoint3() = default;
    TPoint3(const TPoint3&) = default;
    TPoint3(TInt ax, TInt ay, TInt az) {
        x = ax;
        y = ay;
        z = az;
    }
    /// constructs from #TInt array
    enum CtorPtrMark { CTOR_FROM_PTR = 1 };
    TPoint3(const TInt* p, CtorPtrMark /*check*/) { x = p[0]; y = p[1]; z = p[2]; }
    template<typename T2>
    explicit TPoint3(const TPoint3<T2>& p);
    TPoint3& operator= (const TPoint3&) = default;

    void zero() { x = 0; y = 0; z = 0; }
    void set(TInt _x, TInt _y, TInt _z) { x = _x; y = _y; z = _z; }

    const TInt& operator[] (int i) const { return (&x)[i]; }
    TInt& operator[] (int i) { return (&x)[i]; }
    TThis operator- () const { return TThis(-x, -y, -z); }
    TThis operator+ () const { return *this; }

    TThis operator+ (const TThis& a) const { return TThis(x + a.x, y + a.y, z + a.z); }
    TThis operator- (const TThis& a) const { return TThis(x - a.x, y - a.y, z - a.z); }
    float dot(const TThis& a) const { return x * a.x + y * a.y + z * a.z; }
    /// cross product
    TThis operator% (const TThis& a) const { return TThis(y * a.z - z * a.y, z * a.x - x * a.z, x * a.y - y * a.x); }
    TThis operator+ (TInt a) const { return TThis(x + a, y + a, z + a); }
    TThis operator- (TInt a) const { return TThis(x - a, y - a, z - a); }
    TThis operator* (TInt a) const { return TThis(x * a, y * a, z * a); }
    TThis operator/ (TInt a) const { return operator* (1.0f / a); }

    TThis& operator+= (const TThis& a) { x += a.x; y += a.y; z += a.z; return *this; }
    TThis& operator-= (const TThis& a) { x -= a.x; y -= a.y; z -= a.z; return *this; }
    TThis& operator*= (TInt a) { x *= a; y *= a; z *= a; return *this; }
    TThis& operator/= (TInt a) { return operator*= (1.0f / a); }

    bool operator== (const TThis& a) const { return (x == a.x && y == a.y && z == a.z); }
    bool operator!= (const TThis& a) const { return (x != a.x || y != a.y || z != a.z); }

    float lengthSq() const { return x * x + y * y + z * z; }
    float length() const { return sqrtf(lengthSq()); }
    void normalize() { *this *= safeinv(length()); }
    float lengthF() const { return fastsqrt(lengthSq()); }
    void normalizeF() { *this *= safeinvsqrtfast(lengthSq()); }

    template<class T> static TPoint3<T> xyz(const T& a) { return TPoint3<T>(a.x, a.y, a.z); }
    template<class T> static TPoint3<T> xzy(const T& a) { return TPoint3<T>(a.x, a.z, a.y); }
    template<class T> static TPoint3<T> x0y(const T& a) { return TPoint3<T>(a.x, 0, a.y); }
    template<class T> static TPoint3<T> x0z(const T& a) { return TPoint3<T>(a.x, 0, a.z); }
    template<class T> static TPoint3<T> xy0(const T& a) { return TPoint3<T>(a.x, a.y, 0); }
    template<class T> static TPoint3<T> xz0(const T& a) { return TPoint3<T>(a.x, a.z, 0); }
    template<class T> static TPoint3<T> xVy(const T& a, float v) { return TPoint3<T>(a.x, v, a.y); }
    template<class T> static TPoint3<T> xVz(const T& a, float v) { return TPoint3<T>(a.x, v, a.z); }
    template<class T> static TPoint3<T> xyV(const T& a, float v) { return TPoint3<T>(a.x, a.y, v); }
    template<class T> static TPoint3<T> xzV(const T& a, float v) { return TPoint3<T>(a.x, a.z, v); }
    template<class T> static TPoint3<T> rgb(const T& a) { return TPoint3<T>(a.r, a.g, a.b); }

    template<class T> void set_xyz(const T& a) { x = a.x, y = a.y, z = a.z; }
    template<class T> void set_xzy(const T& a) { x = a.x, y = a.z, z = a.y; }
    template<class T> void set_x0y(const T& a) { x = a.x, y = 0, z = a.y; }
    template<class T> void set_x0z(const T& a) { x = a.x, y = 0, z = a.z; }
    template<class T> void set_xy0(const T& a) { x = a.x, y = a.y, z = 0; }
    template<class T> void set_xz0(const T& a) { x = a.x, y = a.z, z = 0; }
    template<class T> void set_xVy(const T& a, float v) { x = a.x, y = v, z = a.y; }
    template<class T> void set_xVz(const T& a, float v) { x = a.x, y = v, z = a.z; }
    template<class T> void set_xyV(const T& a, float v) { x = a.x, y = a.y, z = v; }
    template<class T> void set_xzV(const T& a, float v) { x = a.x, y = a.z, z = v; }
    template<class T> void set_rgb(const T& a) { x = a.r, y = a.g, z = a.b; }
}; // class TPoint3
//////////////////////////////////////////////////////////////////////////


template<class T> float dot(const TPoint3<T>& a, const TPoint3<T>& b) { return a.dot(b); }
template<class T> TPoint3<T> cross(const TPoint3<T>& a, const TPoint3<T>& b) { return a % b; }

template<class T> TPoint3<T> operator*(float a, const TPoint3<T>& p) { return TPoint3<T>(p.x * a, p.y * a, p.z * a); }
template<class T> float lengthSq(const TPoint3<T>& a) { return a.x * a.x + a.y * a.y + a.z * a.z; }
template<class T> float length(const TPoint3<T>& a) { return sqrtf(lengthSq(a)); }
template<class T> TPoint3<T> normalize(const TPoint3<T>& a) { return safeinv(length(a)) * a; }
template<class T> TPoint3<T> mul(const TPoint3<T>& a, const TPoint3<T>& b) { return TPoint3<T>(a.x * b.x, a.y * b.y, a.z * b.z); }
template<class T> TPoint3<T> div(const TPoint3<T>& a, const TPoint3<T>& b) { return TPoint3<T>(a.x / b.x, a.y / b.y, a.z / b.z); }

template<class T> TPoint3<T> floor(const TPoint3<T>& a) { return TPoint3<T>(floorf(a.x), floorf(a.y), floorf(a.z)); }
template<class T> TPoint3<T> ceil(const TPoint3<T>& a) { return TPoint3<T>(ceilf(a.x), ceilf(a.y), ceilf(a.z)); }
template<class T> TPoint3<T> round(const TPoint3<T>& a) { return TPoint3<T>(roundf(a.x), roundf(a.y), roundf(a.z)); }
template<class T> TPoint3<T> frac(const TPoint3<T>& a) { return a - floor(a); }

template<class T> TPoint3<T> abs(const TPoint3<T>& a) { return TPoint3<T>(fabsf(a.x), fabsf(a.y), fabsf(a.z)); }
template<class T> TPoint3<T> sqrt(const TPoint3<T>& a) { return TPoint3<T>(sqrtf(a.x), sqrtf(a.y), sqrtf(a.z)); }
#undef max
#undef min
template<class T> TPoint3<T> max(const TPoint3<T>& a, const TPoint3<T>& b) { return TPoint3<T>(max(a.x, b.x), max(a.y, b.y), max(a.z, b.z)); }
template<class T> TPoint3<T> min(const TPoint3<T>& a, const TPoint3<T>& b) { return TPoint3<T>(min(a.x, b.x), min(a.y, b.y), min(a.z, b.z)); }
template<class T> TPoint3<T> max(const TPoint3<T>& a, const float b) { return TPoint3<T>(max(a.x, b), max(a.y, b), max(a.z, b)); }
template<class T> TPoint3<T> min(const TPoint3<T>& a, const float b) { return TPoint3<T>(min(a.x, b), min(a.y, b), min(a.z, b)); }

//clang-format on

using IPoint3 = TPoint3<int>;
using Point3 = TPoint3<float>;
using Size3 = TPoint3<int>;
using Size3F = TPoint3<float>;


}; // namespace qd
