#pragma once
#include <qd/base/base.h>


namespace qd {

template <typename TInt>
class TPoint2 {
    typedef TPoint2<TInt> TThis;

public:
    TInt x, y;

    template <typename TInt2>
    inline TPoint2(const TPoint2<TInt2>& p) : x((TInt)p.x), y((TInt)p.y) {
    }

    template <typename T0, typename T1>
    inline TPoint2(T0 _x, T1 _y) : x((TInt)_x), y((TInt)_y) {
    }

    template <typename TVal>
    inline TThis& operator=(const TPoint2<TVal>& p) {
        x = (TInt)p.x;
        y = (TInt)p.y;
        return *this;
    }

    inline bool operator==(const TPoint2& p) const {
        return x == p.x && y == p.y;
    }

    inline bool operator!=(const TPoint2& p) const {
        return x != p.x || y != p.y;
    }

    inline TPoint2& operator+=(const TPoint2& p) {
        x += p.x;
        y += p.y;
        return *this;
    }

    inline TPoint2& operator*=(const TPoint2& p) {
        x *= p.x;
        y *= p.y;
        return *this;
    }

    inline TPoint2& operator/=(const TPoint2& p) {
        x /= p.x;
        y /= p.y;
        return *this;
    }

    inline TPoint2& operator-=(const TPoint2& p) {
        x -= p.x;
        y -= p.y;
        return *this;
    }

    template <typename T0, typename T1>
    inline void set(const T0& _x, const T1& _y) {
        x = (TInt)_x;
        y = (TInt)_y;
    }

    template <typename TVal>
    inline void set(const TPoint2<TVal>& p) {
        x = (TInt)p.x;
        y = (TInt)p.y;
    }

};  // class TPoint2
//////////////////////////////////////////////////////////////////////////

using IPoint2 = TPoint2<int>;
using Size = TPoint2<int>;
using Point2 = TPoint2<float>;


};  // namespace qd
