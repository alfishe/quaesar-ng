#pragma once
#include "limits"
#include <EASTL/type_traits.h>
#include <qd/debug/assert.h>
#include <stdint.h>


namespace qd {

//////////////////////////////////////////////////////////////////////////
// FIXED POINT
// TF - Number of bits for Frac Part

template<typename RawIntT, int TF>
class FixedPoint_
{
public:
    enum {
        FracBits = TF
    };
    typedef RawIntT TRawType;

protected:
    typedef FixedPoint_<TRawType, TF> myself;
    TRawType mV = 0;

public:
    typedef FixedPoint_<int32_t, 12> Fixed32;
    typedef FixedPoint_<int64_t, 12> Fixed64;

    static constexpr inline const TRawType getFactor() { return 1 << (FracBits); }
    static constexpr inline uint32_t getTotalBits() { return (uint32_t)sizeof(TRawType) * 8; }
    static constexpr inline uint32_t getFracBits() { return TF; }
    static constexpr inline const myself maxVal() { return CreateFromRaw(std::numeric_limits<TRawType>::max()); }

public:
    inline FixedPoint_() = default;

    // For integral types only:
    template<typename TVal, class = typename eastl::enable_if<std::is_integral<TVal>::value>::type>
    inline constexpr FixedPoint_(TVal Value)
        : mV(myself::_fromIntVal<TVal>(Value)) {}

    inline constexpr /*explicit*/ FixedPoint_(float v)
        : mV(myself::_fromFloatVal(v)) {}

    inline constexpr /*explicit*/ FixedPoint_(double v)
        : mV(myself::_fromFloatVal<double>(v)) {}

    // FixedPoint_(const myself&) = default;
    // FixedPoint_(myself&& rv) = default;

    template<typename ValueT2>
    inline FixedPoint_(const FixedPoint_<ValueT2, TF>& Value) {
        assert(sizeof(TRawType) >= sizeof(ValueT2));
        mV = (TRawType)Value.getRaw();
    }

    inline TRawType getRaw() const { return mV; }
    inline void setRaw(TRawType Val) { mV = Val; }

    inline bool isZero() const { return (mV == 0); }

    // 		myself& operator = (const myself& Value) = default; // {mV = Value.mV; return *this; }
    // 		myself& operator = (myself&& Value) = default;

    inline myself& operator+= (const myself& x) {
        mV += x.mV;
        return *this;
    }
    inline myself& operator-= (const myself& x) {
        mV -= x.mV;
        return *this;
    }
    // negative
    inline myself operator- () const {
        myself t(*this);
        t.mV = -t.mV;
        return t;
    }

    // friend functions
    inline friend myself operator+ (const myself& x, const myself& y) {
        myself t(x);
        t.mV += y.mV;
        return t;
    }
    inline friend myself operator- (const myself& x, const myself& y) {
        myself t(x);
        t.mV -= y.mV;
        return t;
    }

    // multiply only on integer values
    inline constexpr myself operator* (int a) const { return myself::createFromRaw(mV * a); }
    inline constexpr myself operator/ (int a) const { return myself::createFromRaw(mV / a); }

    // comparison operators
    inline friend bool operator== (const myself& x, const myself& y) { return x.mV == y.mV; }
    inline friend bool operator!= (const myself& x, const myself& y) { return x.mV != y.mV; }
    inline friend bool operator> (const myself& x, const myself& y) { return x.mV > y.mV; }
    inline friend bool operator< (const myself& x, const myself& y) { return x.mV < y.mV; }
    inline friend bool operator>= (const myself& x, const myself& y) { return x.mV >= y.mV; }
    inline friend bool operator<= (const myself& x, const myself& y) { return x.mV <= y.mV; }

protected:
    template<typename TVal>
    static constexpr inline TRawType _fromIntVal(TVal f) {
        return ((TRawType)f << FracBits);
    }
    template<typename TVal>
    static constexpr inline TRawType _fromFloatVal(TVal f) {
        return (TRawType)(f * (TVal)(1 << FracBits));
    }

public:
    inline double toDouble() const {
        const double Factor = (1.0 / (double)myself::getFactor());
        double Res = double(mV) * (Factor);
        return Res;
    }

    inline float toFloat() const {
        constexpr const float factor = (1.0f / (float)myself::getFactor());
        float res = float(mV) * factor;
        return res;
    }

    inline FixedPoint_<int32_t, 12> toFix32() const;

    inline Fixed64 toFix64() const {
        Fixed64 Fix64 = Fixed64::createFromRaw((int64_t)mV);
        return Fix64;
    }

    inline TRawType toInt() const { return (mV >= 0) ? (mV >> FracBits) : -((-mV) >> FracBits); }

    inline int toInt32() const { return (int)(mV >> FracBits); }

    inline TRawType toUIntRound() const {
        TRawType v = (mV >> (FracBits - 1));
        bool bOdd = (v & 1) != 0;
        v = v >> 1;
        return bOdd ? v + 1 : v;
    }

    inline myself& multRatio(int nom, int dem = 1) {
        mV = (mV * nom) / dem;
        return *this;
    }

    inline myself& floorSelf() {
        mV = mV & ~((1 << FracBits) - 1); // mask
        return *this;
    }


    // return float as 1000 ms = 1 sec
    inline TRawType toMilli() const {
        static constexpr int fixedFactor = myself::getFactor() >> 3; // = 4096/8=512
        static constexpr int fixedRes = 1000 / 8; // =125
        return (mV * fixedRes) / fixedFactor; // v*125/512 = (v*128-v*3) >> 9
    }


    inline void setInt(TRawType Val) { mV = _fromIntVal(Val); }

    inline void setFloat(float Val) { mV = _fromFloatVal(Val); }

    template<typename ValueT2>
    myself& fromFixed(const FixedPoint_<ValueT2, TF>& r) {
        if (c_def(sizeof(ValueT2) > sizeof(TRawType))) {
            assert(r.GetRaw() <= (ValueT2)std::numeric_limits<TRawType>::max());
        }
        mV = (TRawType)r.GetRaw();
        return *this;
    }

    inline static myself createFromRaw(TRawType v) {
        myself fp;
        fp.mV = v;
        return fp;
    }

    inline static myself createFromInt(RawIntT v) {
        myself fp;
        fp.mV = _fromIntVal(v);
        return fp;
    }

    inline static myself createFromFloat(float v) {
        FixedPoint_ fp;
        fp.mV = _fromFloatVal(v);
        return fp;
    }


    // from Int (1000 ms == 1sec)
    inline static myself createFromTimeMs(TRawType v) {
        FixedPoint_ fp;
        fp.mV = (TRawType)((v * ((1 << FracBits) / 8)) / (1000 / 8));
        return fp;
    }


    static void LogInfo() {
        // CLog3::CSection s("Fixed Point Log");
        // CLog3::get()->PrintLn("TotalBits: %u, FracBits: %u, sizeof(TRawType): %u, MaxVal: 0x%lX", GetTotalBits(),
        // GetFracBits(), (int)sizeof(TRawType), (long)MaxVal() );
    }

}; // class FixedPoint_
//////////////////////////////////////////////////////////////////////////

typedef FixedPoint_<int32_t, 12> Fixed32;


template<typename TRawType, int TF>
inline FixedPoint_<int32_t, 12> FixedPoint_<TRawType, TF>::toFix32() const {
    static_assert((int)myself::FracBits == (int)qd::Fixed32::FracBits);
    qd::Fixed32 res = qd::Fixed32::createFromRaw((int32_t)mV);
    return res;
}


//////////////////////////////////////////////////////////////////////////
// fixed with float
class CFix32WF
{
    typedef CFix32WF myself;
    Fixed32 m_Fixed;
    float m_Float;

public:
    CFix32WF()
        : m_Float(0) {}

    explicit CFix32WF(const Fixed32& fx, float f)
        : m_Fixed(fx)
        , m_Float(f) {}

    explicit CFix32WF(const Fixed32& fx)
        : m_Fixed(fx)
        , m_Float(m_Fixed.toFloat()) {}

    CFix32WF(const myself& rv)
        : m_Fixed(rv.m_Fixed)
        , m_Float(rv.m_Float) {}

#ifdef RVALUE_REFERENCES_SUPPORTED
    CFix32WF(myself&& rv)
        : m_Float(rv.m_Float)
        , m_Fixed(rv.m_Fixed) {}
#endif // RVALUE_REFERENCES_SUPPORTED

    explicit CFix32WF(float f)
        : m_Fixed(f)
        , m_Float(f) {}


    void setFloat(float FVal) {
        m_Float = FVal;
        m_Fixed.setFloat(FVal);
    }

    void setFixed(const Fixed32& FixVal) {
        m_Fixed = FixVal;
        m_Float = m_Fixed.toFloat();
    }

    float getFloat() const { return m_Float; }
    const Fixed32& getFixed() const { return m_Fixed; }

    inline operator float () const { return m_Float; }

    inline operator Fixed32 () const { return m_Fixed; }

    myself& operator= (const myself& x) {
        m_Fixed = x.m_Fixed;
        m_Float = x.m_Float;
        return *this;
    }
    myself& operator= (myself&& x) = default;

    inline myself& operator+= (const myself& x) {
        m_Fixed += x.m_Fixed;
        m_Float += x.m_Float;
        return *this;
    }
    inline myself& operator-= (const myself& x) {
        m_Fixed -= x.m_Fixed;
        m_Float -= x.m_Float;
        return *this;
    }
    // negative
    inline myself operator- () const {
        myself t(*this);
        t.m_Fixed = -t.m_Fixed;
        t.m_Float = -t.m_Float;
        return t;
    }

    // friend functions
    inline friend myself operator+ (const myself& x, const myself& y) {
        myself t(x);
        t += y;
        return t;
    }
    inline friend myself operator- (const myself& x, const myself& y) {
        myself t(x);
        t -= y;
        return t;
    }

    // comparison operators
    inline friend bool operator== (const myself& x, const myself& y) { return x.m_Fixed == y.m_Fixed; }
    inline friend bool operator!= (const myself& x, const myself& y) { return x.m_Fixed != y.m_Fixed; }
    inline friend bool operator> (const myself& x, const myself& y) { return x.m_Fixed > y.m_Fixed; }
    inline friend bool operator< (const myself& x, const myself& y) { return x.m_Fixed < y.m_Fixed; }
    inline friend bool operator>= (const myself& x, const myself& y) { return x.m_Fixed >= y.m_Fixed; }
    inline friend bool operator<= (const myself& x, const myself& y) { return x.m_Fixed <= y.m_Fixed; }


}; // class CFixWithFloat



typedef FixedPoint_<int32_t, 12> Fixed32;
typedef FixedPoint_<int64_t, 12> Fixed64;

// FIX32_MAX == 524288 sec == 145 hours == 6 days
#define FIX32_MAX Fixed32::MaxVal()
#define FIX64_MAX Fixed64::MaxVal()
//////////////////////////////////////////////////////////////////////////


}; // namespace qd
//////////////////////////////////////////////////////////////////////////

using qd::Fixed32;
using qd::Fixed64;
