#pragma once
#include "qd/base/base.h"
#include <initializer_list>


namespace qd {

// clang-format off

template <typename TInt>
class TPoint2 {
    typedef TPoint2<TInt> TThis;

public:
    TInt x, y;

    inline TPoint2() : x((TInt)0), y((TInt)0) {}

    template <typename TInt2>
    inline TPoint2(const TPoint2<TInt2>& p) : x((TInt)p.x), y((TInt)p.y) {}

    template <typename T0, typename T1>
    inline TPoint2(T0 _x, T1 _y) : x((TInt)_x), y((TInt)_y) {}

    template <typename TInt2>
    inline constexpr TPoint2(std::initializer_list<TInt2> il) {
        if (il.size() >= 1) x = (TInt)*(il.begin());
        else x = 0;
        if (il.size() >= 2) y = (TInt)*(il.begin() + 1);
        else y = x;
    }

    inline void zero() { x = 0; y = 0; }

    template <typename TVal>
    inline TThis& operator=(const TPoint2<TVal>& p) { x = (TInt)p.x; y = (TInt)p.y; return *this; }

    float dot(const TThis& a) const { return x * a.x + y * a.y; }

    TThis operator+ (TInt a) const { return TThis(x + a, y + a); }
    TThis operator- (TInt a) const { return TThis(x - a, y - a); }
    TThis operator* (TInt a) const { return TThis(x * a, y * a); }
    TThis operator/ (TInt a) const { return operator* (1.0f / a); }

    inline bool operator==(const TPoint2& p) const { return x == p.x && y == p.y; }
    inline bool operator!=(const TPoint2& p) const { return x != p.x || y != p.y; }
    inline TPoint2& operator+=(const TPoint2& p) { x += p.x; y += p.y; return *this; }
    inline TPoint2& operator*=(const TPoint2& p) { x *= p.x; y *= p.y; return *this; }
    inline TPoint2& operator/=(const TPoint2& p) { x /= p.x; y /= p.y; return *this; }
    inline TPoint2& operator-=(const TPoint2& p) { x -= p.x; y -= p.y; return *this; }
    TThis& operator*= (TInt a) { x *= a; y *= a; return *this; }
    TThis& operator/= (TInt a) { return operator*= (1.0f / a); }

    const TInt& operator[] (int i) const { return (&x)[i]; }
    TInt& operator[] (int i) { return (&x)[i]; }
    TPoint2 operator- () const { return TPoint2(-x, -y); }
    TPoint2 operator+ () const { return *this; }
    TPoint2 operator+ (const TPoint2& a) const { return TPoint2(x + a.x, y + a.y); }
    TPoint2 operator- (const TPoint2& a) const { return TPoint2(x - a.x, y - a.y); }

    template <typename T0, typename T1>
    inline void set(const T0& _x, const T1& _y) { x = (TInt)_x; y = (TInt)_y; }

    template <typename TVal>
    inline void set(const TPoint2<TVal>& p) { x = (TInt)p.x; y = (TInt)p.y; }

    float lengthSq() const { return (float)(x * x + y * y); }
    float length() const { return sqrtf((float)(x * x + y * y)); }

};  // class TPoint2
//////////////////////////////////////////////////////////////////////////



template<class T> float dot(const TPoint2<T>& a, const TPoint2<T>& b) { return a * b; }
template<class T> TPoint2<T> cross(const TPoint2<T>& a, const TPoint2<T>& b) { return a % b; }

template<class T> TPoint2<T> operator*(float a, const TPoint2<T>& p) { return TPoint2<T>(p.x * a, p.y * a); }
template<class T> float lengthSq(const TPoint2<T>& a) { return a.x * a.x + a.y * a.y; }
template<class T> float length(const TPoint2<T>& a) { return sqrtf(lengthSq(a)); }
template<class T> TPoint2<T> normalize(const TPoint2<T>& a) { return safeinv(length(a)) * a; }
template<class T> TPoint2<T> mul(const TPoint2<T>& a, const TPoint2<T>& b) { return TPoint2<T>(a.x * b.x, a.y * b.y); }
template<class T> TPoint2<T> div(const TPoint2<T>& a, const TPoint2<T>& b) { return TPoint2<T>(a.x / b.x, a.y / b.y); }
template<class T> TPoint2<T> floor(const TPoint2<T>& a) { return TPoint2<T>(::floorf(a.x), ::floorf(a.y)); }
template<class T> TPoint2<T> max(const TPoint2<T>& a, const TPoint2<T>& b) { return TPoint2<T>(max(a.x, b.x), max(a.y, b.y)); }
template<class T> TPoint2<T> min(const TPoint2<T>& a, const TPoint2<T>& b) { return TPoint2<T>(min(a.x, b.x), min(a.y, b.y)); }



template<typename TInt = int>
class TSize : public TPoint2<TInt>
{
    typedef TPoint2<TInt> TSuper;

public:
    using TPoint2<TInt>::TPoint2;
    bool isSizeValid() const { return this->x > 0 && this->y > 0; }
}; // class Size2


using IPoint2 = TPoint2<int>;
using Point2 = TPoint2<float>;
using Size2 = TSize<int>;
using Size2F = TSize<float>;



};  // namespace qd
