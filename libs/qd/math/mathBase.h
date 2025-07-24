#pragma once

#define M_FPI  3.14159265358979323846f
#define M_F2PI 6.28318530717958647692f
#define M_FPI2 1.57079632679489661923f


namespace qd {
template<typename T1, typename T2>
[[nodiscard]] inline T1 min(const T1 __a, const T2 __b)
{
    return (T1)__a < (T1)__b ? (T1)__a : (T1)__b;
}

template<typename T1, typename T2>
[[nodiscard]] inline T1 max(const T1 __a, const T2 __b)
{
    return (T1)__a > (T1)__b ? (T1)__a : (T1)__b;
}

template<typename T1, typename T2>
[[nodiscard]] inline T1 max_get(const T1 Value, const T2 tMax)
{
    if (Value < (T1)tMax)
        return (T1)tMax;
    return tMax;
}
template<typename T1, typename T2>
[[nodiscard]] inline T1 min_get(const T1 Value, const T2 tMin)
{
    if (Value > (T1)tMin)
        return (T1)tMin;
    return tMin;
}

template<typename T1, typename T2>
inline void max_inplace(T1& Value, const T2 tMax)
{
    if (Value < (T1)tMax)
        Value = (T1)tMax;
}
template<typename T1, typename T2>
inline void min_inplace(T1& Value, const T2 tMin)
{
    if (Value > (T1)tMin)
        Value = (T1)tMin;
}

template<typename T, typename T1, typename T2>
[[nodiscard]] inline constexpr T clamp(const T Value, const T1 tMin, const T2 tMax)
{
    // assert(tMax >= tMin);
    if (Value < tMin)
        return tMin;
    if (Value > tMax)
        return tMax;
    return Value;
}

template<typename T, typename T1, typename T2>
inline void clamp_inplace(T& Value, const T1 tMin, const T2 tMax)
{
    assert(tMax >= tMin);
    if (Value < (T)tMin)
        Value = (T)tMin;
    if (Value > (T)tMax)
        Value = (T)tMax;
}

template<typename T1, typename T2>
inline void clamp_max_inplace(T1& Value, const T2 tMax)
{
    if (Value > (T1)tMax)
        Value = (T1)tMax;
}
template<typename T1, typename T2>
inline void clamp_min_inplace(T1& Value, const T2 tMin)
{
    if (Value < (T1)tMin)
        Value = (T1)tMin;
}

// RETURN CLAMPED BY MAXIMAL EDGE VALUE
template<typename T1, typename T2>
[[nodiscard]] inline T1 clamp_max(const T1 Value, const T2 tMax)
{
    if (Value > (T1)tMax)
        return (T1)tMax;
    return Value;
}

// RETURN CLAMPED BY MINIMAL EDGE VALUE
template<typename T1, typename T2>
[[nodiscard]] inline T1 clamp_min(const T1 Value, const T2 tMin)
{
    if (Value < (T1)tMin)
        return (T1)tMin;
    return Value;
}


// FROM [Min to max) (exclusive)
namespace details {
template<int bMinInclusive, int bMaxInclusive, class T, class T1, class T2>
[[nodiscard]] constexpr inline bool is_in_(const T Value, const T1 tMin, const T2 tMax)
{
    if constexpr (bMinInclusive)
    {
        if (Value < (T)tMin)
            return false;
    }
    else if (Value <= (T)tMin)
        return false;

    if constexpr (bMaxInclusive)
    {
        if (Value > (T)tMax)
            return false;
    }
    else if (Value >= (T)tMax)
        return false;
    return true;
}
}; // namespace details

// IsIn(Min, max)
template<class T, class T1, class T2>
[[nodiscard]] inline bool is_in_00(const T Value, const T1 tMin, const T2 tMax)
{
    return details::is_in_<0, 0>(Value, tMin, tMax);
}
// IsIn[Min, max]
template<class T, class T1, class T2>
[[nodiscard]] inline bool is_in_11(const T Value, const T1 tMin, const T2 tMax)
{
    return details::is_in_<1, 1>(Value, tMin, tMax);
}
// IsIn[Min, max)
template<class T, class T1, class T2>
[[nodiscard]] inline bool is_in_10(const T Value, const T1 tMin, const T2 tMax)
{
    return details::is_in_<1, 0>(Value, tMin, tMax);
}
template<class T, class T1, class T2>
[[nodiscard]] inline bool is_in_01(const T Value, const T1 tMin, const T2 tMax)
{
    return details::is_in_<0, 1>(Value, tMin, tMax);
}

}; // namespace qd
