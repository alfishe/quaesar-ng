#pragma once
#include <stdint.h>


namespace qd {

//////////////////////////////////////////////////////////////////////////
struct EFlow {
    enum Type : uint8_t {
        UNDEF = 0,
        NO_RESULT = 0,

        DONE = 1,
        STOP = 1,
        SUCCESS = 1,

        CONTINUE = 2,
        REPEAT = 2,
        LOOP = 2,
    };
    EFlow::Type mVal = EFlow::UNDEF;

    EFlow() = default;

    template<typename TInt>
    EFlow(TInt val)
        : mVal(static_cast<Type>(val))
    {}

    constexpr operator EFlow::Type () const { return mVal; }
    bool isDone() const { return mVal == EFlow::DONE; }
    bool isStop() const { return mVal == EFlow::STOP; }
    bool isContinue() const { return mVal == EFlow::CONTINUE; }
    bool hasResult() const { return mVal != EFlow::UNDEF; }
    bool maybeContinue() const { return mVal == EFlow::UNDEF || mVal == EFlow::CONTINUE; }

}; // enum EFlow
//////////////////////////////////////////////////////////////////////////

}; // namespace qd
