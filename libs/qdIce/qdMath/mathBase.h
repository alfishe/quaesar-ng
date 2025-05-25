#pragma once

#define M_FPI	3.14159265358979323846f
#define M_F2PI	6.28318530717958647692f
#define M_FPI2	1.57079632679489661923f


namespace qd
{
	template<typename T1, typename T2>
	[[nodiscard]] inline T1 min(const T1 __a, const T2 __b) {
		return (T1)__a < (T1)__b ? (T1)__a : (T1)__b;
	}

	template<typename T1, typename T2>
	[[nodiscard]] inline T1 max(const T1 __a, const T2 __b) {
		return (T1)__a > (T1)__b ? (T1)__a : (T1)__b;
	}

	template<typename T1, typename T2>
	[[nodiscard]] inline T1 maxGet(const T1 Value, const T2 tMax) {
		if (Value < (T1)tMax)
			return (T1)tMax;
		return tMax;
	}
	template<typename T1, typename T2>
	[[nodiscard]] inline T1 minGet(const T1 Value, const T2 tMin) {
		if (Value > (T1)tMin)
			return (T1)tMin;
		return tMin;
	}

	template<typename T1, typename T2>
	inline void maxSelf(T1 & Value, const T2 tMax) {
		if (Value < (T1)tMax)
			Value = (T1)tMax;
	}
	template<typename T1, typename T2>
	inline void minSelf(T1 & Value, const T2 tMin) {
		if (Value > (T1)tMin)
			Value = (T1)tMin;
	}

	template<typename T, typename T1, typename T2>
	[[nodiscard]] inline constexpr T clamp(const T Value, const T1 tMin, const T2 tMax)
	{
		//assert(tMax >= tMin);
		if (Value < tMin)
			return tMin;
		if (Value > tMax)
			return tMax;
		return Value;
	}

	template<typename T, typename T1, typename T2>
	inline void clampSelf(T & Value, const T1 tMin, const T2 tMax)
	{
		assert(tMax >= tMin);
		if (Value < (T)tMin)
			Value = (T)tMin;
		if (Value > (T)tMax)
			Value = (T)tMax;
	}

	template<typename T1, typename T2>
	inline void clampMaxSelf(T1 & Value, const T2 tMax)
	{
		if (Value > (T1)tMax)
			Value = (T1)tMax;
	}
	template<typename T1, typename T2>
	inline void clampMinSelf(T1 & Value, const T2 tMin)
	{
		if (Value < (T1)tMin)
			Value = (T1)tMin;
	}

	// RETURN CLAMPED BY MAXIMAL EDGE VALUE
	template<typename T1, typename T2>
	[[nodiscard]] inline T1 clampMaxGet(const T1 Value, const T2 tMax)
	{
		if (Value > (T1)tMax)
			return (T1)tMax;
		return Value;
	}

	// RETURN CLAMPED BY MINIMAL EDGE VALUE
	template<typename T1, typename T2>
	[[nodiscard]] inline T1 clampMinGet(const T1 Value, const T2 tMin)
	{
		if (Value < (T1)tMin)
			return (T1)tMin;
		return Value;
	}


	// FROM [Min to max) (exclusive)
	template<int MinInclusive, int MaxInclusive, class T, class T1, class T2>
	[[nodiscard]] constexpr inline bool isIn_(const T Value, const T1 tMin, const T2 tMax)
	{
		if constexpr (MinInclusive)
		{
			if (Value < (T)tMin)
				return false;
		}
		else if (Value <= (T)tMin)
			return false;

		if constexpr (MaxInclusive)
		{
			if (Value > (T)tMax)
				return false;
		}
		else if (Value >= (T)tMax)
			return false;
		return true;
	}

	// IsIn(Min, max)
	template<class T, class T1, class T2>
	[[nodiscard]] inline bool isIn00(const T Value, const T1 tMin, const T2 tMax)
	{
		return isIn_<0, 0>(Value, tMin, tMax);
	}
	// IsIn[Min, max]
	template<class T, class T1, class T2>
	[[nodiscard]] inline bool isIn11(const T Value, const T1 tMin, const T2 tMax)
	{
		return isIn_<1, 1>(Value, tMin, tMax);
	}
	// IsIn[Min, max)
	template<class T, class T1, class T2>
	[[nodiscard]] inline bool isIn10(const T Value, const T1 tMin, const T2 tMax)
	{
		return isIn_<1, 0>(Value, tMin, tMax);
	}
	template<class T, class T1, class T2>
	[[nodiscard]] inline bool isIn01(const T Value, const T1 tMin, const T2 tMax)
	{
		return isIn_<0, 1>(Value, tMin, tMax);
	}

}; // namespace sg
