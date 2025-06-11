#pragma once
#include <stdint.h>
#include <EASTL/type_traits.h>
#include <qd/qdDebug/assert.h>


namespace qd
{

	//////////////////////////////////////////////////////////////////////////
	// FIXED POINT
	// TF - Number of bits for Frac Part

	template<typename RawIntT, int TF>
	class CFixedPoint
	{
	public:
		enum {
			FracBits = TF
		};
		typedef RawIntT TRawType;

	protected:
		typedef CFixedPoint<TRawType, TF> myself;
		TRawType mV = 0;

	public:
		typedef CFixedPoint<int32_t, 12> CFixed32;
		typedef CFixedPoint<int64_t, 12> CFixed64;

		static constexpr inline const TRawType getFactor() { return 1 << (FracBits); }
		static constexpr inline uint32_t getTotalBits() { return (uint32_t)sizeof(TRawType) * 8; }
		static constexpr inline uint32_t getFracBits() { return TF; }
		static constexpr inline const myself maxVal() {
			return CreateFromRaw( std::numeric_limits<TRawType>::max() );
		}

	public:

		inline CFixedPoint() = default;

		// For integral types only:
		template<typename TVal, class = typename eastl::enable_if<std::is_integral<TVal>::value>::type>
		inline constexpr CFixedPoint(TVal Value)
			: mV(myself::_fromIntVal<TVal>(Value))
		{}

		inline constexpr /*explicit*/ CFixedPoint(float v)
			: mV(myself::_fromFloatVal(v))
		{}

 		inline constexpr /*explicit*/ CFixedPoint(double v)
			: mV(myself::_fromFloatVal<double>(v))
		{}

		inline CFixedPoint(const myself& v) = default;

	#ifdef RVALUE_REFERENCES_SUPPORTED
		//CFixedPoint(myself&& rv) = default;
	#endif // RVALUE_REFERENCES_SUPPORTED

		template<typename ValueT2>
		inline CFixedPoint(const CFixedPoint<ValueT2, TF>& Value) {
			assert(sizeof(TRawType) >= sizeof(ValueT2) );
			mV = (TRawType)Value.getRaw();
		}

		inline TRawType getRaw() const { return mV; }
		inline void setRaw(TRawType Val) { mV = Val; }

		inline bool isZero() const {
			return (mV == 0);
		}

// 		myself& operator = (const myself& Value) = default; // {mV = Value.mV; return *this; }
// 		myself& operator = (myself&& Value) = default;

		inline myself& operator += (const myself& x) { mV += x.mV; return *this; }
		inline myself& operator -= (const myself& x) { mV -= x.mV; return *this; }
		// negative
		inline myself  operator -  ( ) const { myself t(*this); t.mV = -t.mV; return t; }

		// friend functions
		inline friend myself operator + (const myself& x, const myself& y) { myself t(x); t.mV += y.mV; return t;}
		inline friend myself operator - (const myself& x, const myself& y) { myself t(x); t.mV -= y.mV; return t;}

		// multiply only on integer values
		inline constexpr myself operator * (int a) const { return myself::CreateFromRaw(mV * a); }
		inline constexpr myself operator / (int a) const { return myself::CreateFromRaw(mV / a); }

		// comparison operators
		inline friend bool operator == (const myself& x, const myself& y) { return x.mV == y.mV; }
		inline friend bool operator != (const myself& x, const myself& y) { return x.mV != y.mV; }
		inline friend bool operator >  (const myself& x, const myself& y) { return x.mV > y.mV; }
		inline friend bool operator <  (const myself& x, const myself& y) { return x.mV < y.mV; }
		inline friend bool operator >= (const myself& x, const myself& y) { return x.mV >= y.mV; }
		inline friend bool operator <= (const myself& x, const myself& y) { return x.mV <= y.mV; }

	protected:
		template<typename TVal>
		static constexpr inline TRawType _fromIntVal(TVal f)  {
			return ((TRawType)f << FracBits);
		}
		template<typename TVal>
		static constexpr inline TRawType _fromFloatVal(TVal f) {
			return (TRawType)(f * (TVal)(1 << FracBits) );
		}

	public:
		inline double toDouble() const  {
			const double Factor = (1.0 / (double)myself::getFactor());
			double Res = double(mV) * (Factor);
			return Res;
		}

		inline float toFloat() const {
			const float Factor = (1.0f / (float)myself::getFactor());
			float Res = (float)(float(mV) * (Factor));
			return Res;
		}

		inline CFixed32 toFix32() const;

		inline CFixed64 toFix64() const {
			CFixed64 Fix64 = CFixed64::createFromRaw( (int64_t)mV );
			return Fix64;
		}

		inline TRawType toInt() const {
			if (mV >= 0)
				return (mV >> FracBits);
			else
				return -((-mV) >> FracBits);
		}

		inline int toInt32() const {
			return (int)ToInt();
		}

		inline TRawType toUInt() const {
			return (mV >> FracBits);
		}

		inline TRawType toUIntRound() const {
			TRawType v = (mV >> (FracBits-1));
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
			static constexpr int fixedRes = 1000 / 8; // =125
			static constexpr int fixedFactor = myself::GetFactor() >> 3; // = 4096/8=512
			// return (mV * fixedRes) / fixedFactor; -- // v*125/512 = (v*128-v*3) >> 9
			const int v2 = mV >> 2; // =/ 4
			const int v8 = v2 >> 6; // =/ 256
			const int v9 = v8 >> 1; // =/ 512
			return v2 - (v8 + v9);
		}



		inline void setInt(TRawType Val) {
			mV = _fromIntVal(Val);
		}

		inline void setFloat(float Val) {
			mV = _fromFloatVal(Val);
		}

		template<typename ValueT2>
		myself& fromFixed(const CFixedPoint<ValueT2, TF>& r) {
			if ( c_def(sizeof(ValueT2) > sizeof(TRawType)) ) {
				assert( r.GetRaw() <= (ValueT2)std::numeric_limits<TRawType>::max() );
			}
			mV = (TRawType)r.GetRaw();
			return *this;
		}

		inline static myself createFromRaw(TRawType v) {
			myself fp; fp.mV = v; return fp;
		}

		inline static myself createFromInt(RawIntT v) {
			myself fp; fp.mV = _fromIntVal(v);
			return fp;
		}

		inline static myself createFromFloat(float v) {
			CFixedPoint fp; fp.mV = _fromFloatVal(v);
			return fp;
		}


		// from Int (1000 ms == 1sec)
		inline static myself createFromTimeMs(TRawType v) {
			CFixedPoint fp;
			fp.mV = (TRawType) ( ( v * ((1 << FracBits) / 8) ) / (1000/8) );
			return fp;
		}


		static void LogInfo() {
			//CLog3::CSection s("Fixed Point Log");
			//CLog3::get()->PrintLn("TotalBits: %u, FracBits: %u, sizeof(TRawType): %u, MaxVal: 0x%lX", GetTotalBits(), GetFracBits(), (int)sizeof(TRawType), (long)MaxVal() );
		}

	}; // class CFixedPoint
	//////////////////////////////////////////////////////////////////////////


	template<typename TRawType, int TF>
	inline CFixedPoint<int32_t, 12> CFixedPoint<TRawType, TF>::toFix32() const {
		CG_ASSERT( (int)myself::FracBits == (int)CFixed32::FracBits );
		CFixed32 Fix32 = CFixed32::CreateFromRaw( (int32_t)mV );
		//assert( CFloat::Compare( ToFloat(), Fix32.ToFloat() ) );
		return Fix32;
	}


	typedef CFixedPoint<int32_t, 12> CFixed32;


	//////////////////////////////////////////////////////////////////////////
	// fixed with float
	class CFix32WF
	{
		typedef CFix32WF myself;
		CFixed32 m_Fixed;
		float m_Float;
	public:
		CFix32WF()
			: m_Float(0)
		{}

		explicit CFix32WF(const CFixed32& fx, float f)
			: m_Fixed(fx), m_Float(f)
		{}

		explicit CFix32WF(const CFixed32& fx)
			: m_Fixed(fx), m_Float(m_Fixed.toFloat())
		{}

		CFix32WF(const myself& rv)
			: m_Fixed(rv.m_Fixed), m_Float(rv.m_Float)
		{}

	#ifdef RVALUE_REFERENCES_SUPPORTED
		CFix32WF(myself&& rv)
			: m_Float(rv.m_Float), m_Fixed(rv.m_Fixed)
		{}
	#endif // RVALUE_REFERENCES_SUPPORTED

		explicit CFix32WF(float f)
			: m_Fixed( f ), m_Float(f)
		{}


		void setFloat(float FVal) {
			m_Float = FVal;
			m_Fixed.setFloat(FVal);
		}

		void setFixed(const CFixed32& FixVal) {
			m_Fixed = FixVal;
			m_Float = m_Fixed.toFloat();
		}

		float getFloat() const {
			return m_Float;
		}
		const CFixed32& getFixed() const {
			return m_Fixed;
		}

		inline operator float () const {
			return m_Float;
		}

		inline operator CFixed32 () const {
			return m_Fixed;
		}

		myself& operator = (const myself& x) {m_Fixed = x.m_Fixed; m_Float = x.m_Float; return *this;}
		myself& operator = (myself&& x) = default;

		inline myself& operator += (const myself& x) { m_Fixed += x.m_Fixed; m_Float += x.m_Float; return *this; }
		inline myself& operator -= (const myself& x) { m_Fixed -= x.m_Fixed; m_Float -= x.m_Float; return *this; }
		// negative
		inline myself  operator -  ( ) const { myself t(*this); t.m_Fixed = -t.m_Fixed; t.m_Float = -t.m_Float; return t; }

		// friend functions
		inline friend myself operator + (const myself& x, const myself& y) { myself t(x); t += y; return t;}
		inline friend myself operator - (const myself& x, const myself& y) { myself t(x); t -= y; return t;}

		// comparison operators
		inline friend bool operator == (const myself& x, const myself& y) { return x.m_Fixed == y.m_Fixed; }
		inline friend bool operator != (const myself& x, const myself& y) { return x.m_Fixed != y.m_Fixed; }
		inline friend bool operator >  (const myself& x, const myself& y) { return x.m_Fixed >  y.m_Fixed; }
		inline friend bool operator <  (const myself& x, const myself& y) { return x.m_Fixed <  y.m_Fixed; }
		inline friend bool operator >= (const myself& x, const myself& y) { return x.m_Fixed >= y.m_Fixed; }
		inline friend bool operator <= (const myself& x, const myself& y) { return x.m_Fixed <= y.m_Fixed; }


	}; // class CFixWithFloat



typedef CFixedPoint<int32_t, 12> CFixed32;
typedef CFixedPoint<int64_t, 12> CFixed64;

	// FIX32_MAX == 524288 sec == 145 hours == 6 days
#define FIX32_MAX CFixed32::MaxVal()
#define FIX64_MAX CFixed64::MaxVal()
	//////////////////////////////////////////////////////////////////////////


}; // namespace qd
