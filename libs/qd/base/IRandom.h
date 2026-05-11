#pragma once
#include <cstdint>
#include <cmath> // floorf
#include "qd/stl/algorithm.h" // forward


namespace qd {

//------------------------------------------------------------------------
class IRandom
{
public:
    virtual float getFloat01() = 0;
    virtual float getFloat11() = 0;
    virtual float getFloat(float a, float b) = 0;
    virtual int getUInt() = 0;
    virtual int getInt() = 0;
    virtual int getInt(int a, int b) = 0;
    virtual ~IRandom() = default;
}; // class IRandom
//////////////////////////////////////////////////////////////////////////


template<class TRandImpl>
class Random_ final : public IRandom
{
public:
    TRandImpl mRandImpl;

public:

    template<typename... TArgs>
    Random_(TArgs&&... args)
        : mRandImpl(qtd::forward<TArgs>(args)...) {}

    Random_(const Random_<TRandImpl>& i_copy) = default;

    float getFloat01() override {
        return mRandImpl.getFloat01();
    }
    float getFloat11() override {
        return mRandImpl.getFloat11();
    }
    float getFloat(float a, float b) override {
        return mRandImpl.getFloat(a, b);
    }
    int getUInt() override {
        return mRandImpl.getUInt();
    }
    int getInt() override {
        return mRandImpl.getInt();
    }
    int getInt(int a, int b) override {
        return mRandImpl.getInt(a, b); }

}; // class Random_
//////////////////////////////////////////////////////////////////////////


//------------------------------------------------------------------------
// Adapter to extend random implementations with common methods
//
template<class TRandImpl>
class BaseRandExt_
{
public:
    TRandImpl& getImpl() { return *static_cast<TRandImpl*>(this); }

public:
    float getFloat01() {
        float rv = getImpl().nextFloat();
        float frac = rv - floorf(rv);
        return frac;
    }
    float getFloat11() {
        float rv = getImpl().nextFloat();
        float frac = rv - floorf(rv);
        return frac * 2.0f - 1.0f;
    }
    float getFloat(float a, float b) {
        float rv = getImpl().nextFloat();
        float frac = rv - floorf(rv);
        return a + frac * (b - a);
    }
    int getUInt() { return getImpl().nextUInt(); }
    int getInt() { return static_cast<int>(getImpl().nextUInt()); }
    int getInt(int a, int b) { return getImpl().nextIRange(a, b); }

}; // class BaseRandExt_
//////////////////////////////////////////////////////////////////////////

}; // namespace qd



