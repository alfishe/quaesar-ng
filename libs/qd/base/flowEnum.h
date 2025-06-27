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
        FAILED = 2,

        REPEAT = 3,
        LOOP = 3,
    };
    EFlow::Type mVal = EFlow::UNDEF;

    EFlow() = default;

    template<typename TInt>
    EFlow(TInt val)
        : mVal(static_cast<Type>(val))
    {}

    constexpr operator EFlow::Type () const { return mVal; }

}; // enum EFlow
//////////////////////////////////////////////////////////////////////////

}; // namespace qd
