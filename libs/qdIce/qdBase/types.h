#pragma once
#include <EASTL/internal/config.h>
#include <stdint.h>


namespace qd {


// UNIX TIME, it's Seconds from 1970
//__time64_t
typedef int64_t TTime64;
typedef uint32_t TTime32U;
static constexpr uint32_t TTime32U_MAX = UINT32_MAX;  //~0u;
static constexpr int64_t TTime64_MAX = INT64_MAX;


//////////////////////////////////////////////////////////////////////////
template <typename TInt>
class TPoint {
    typedef TPoint<TInt> TThis;

public:
    TInt x, y;

    template <typename TInt2>
    inline TPoint(const TPoint<TInt2>& p) : x((TInt)p.x), y((TInt)p.y) {
    }

    template <typename T0, typename T1>
    inline TPoint(T0 _x, T1 _y) : x((TInt)_x), y((TInt)_y) {
    }

    template <typename TVal>
    inline TThis& operator=(const TPoint<TVal>& p) {
        x = (TInt)p.x;
        y = (TInt)p.y;
        return *this;
    }

    inline bool operator==(const TPoint& p) const {
        return x == p.x && y == p.y;
    }

    inline bool operator!=(const TPoint& p) const {
        return x != p.x || y != p.y;
    }

    inline TPoint& operator+=(const TPoint& p) {
        x += p.x;
        y += p.y;
        return *this;
    }

    inline TPoint& operator*=(const TPoint& p) {
        x *= p.x;
        y *= p.y;
        return *this;
    }

    inline TPoint& operator/=(const TPoint& p) {
        x /= p.x;
        y /= p.y;
        return *this;
    }

    inline TPoint& operator-=(const TPoint& p) {
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
    inline void set(const TPoint<TVal>& p) {
        x = (TInt)p.x;
        y = (TInt)p.y;
    }

};  // class TPoint

//////////////////////////////////////////////////////////////////////////
using Int2 = TPoint<int>;
using Vec2 = TPoint<float>;


//////////////////////////////////////////////////////////////////////////
struct EFlow {
    enum Type : uint8_t {
        NO_RESULT = 0,
        SUCCESS = 1,
        FAILED = 2,
        REPEAT = 3,
    };
    EFlow::Type mVal = (EFlow::Type)0;

    template <typename TInt>
    EFlow(TInt val) : mVal(static_cast<Type>(val)) {
    }

    constexpr operator EFlow::Type() const {
        return mVal;
    }

};  // enum EFlow
//////////////////////////////////////////////////////////////////////////

};  // namespace qd
