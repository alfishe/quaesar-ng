#pragma once
#include <stdint.h>


namespace qd {

//////////////////////////////////////////////////////////////////////////
// ETrisult - HAS THREE MAIN STATES: SUCCESS/FAIL/NO_RESULT
// ETrisult - it's result of process with some common states
struct ETrisult
{
	typedef ETrisult TThis;
	enum Type : uint8_t {
		NO_RESULT = 0, // work result not defined (unknown result status)
		UNKNOWN   = 0,

		// good result
		SUCCESS   = 1, // work is done with Success status

		// fail result
		FAIL      = 2,
		ERROR     = 2,
		// ADDITIONAL exception results
		EXPIRED = ERROR + 1,
		EXCEPTION = ERROR + 2,
	};
    ETrisult::Type mV = NO_RESULT;

public:
    ETrisult() = default;
    template<typename TInt>
    ETrisult(TInt val)
        : mV(static_cast<Type>(val))
    {}

	inline ETrisult/*TThis*/(bool r) { mV = (Type)r; }
	inline TThis& operator = (bool r)  { mV = (Type)((uint8_t)r); return *this; }

	inline bool isUnknown() const {
		return mV == TThis::NO_RESULT;
	}
	inline bool hasResult() const {
		return mV != TThis::NO_RESULT;
	}
	inline bool isSuccess() const {
		return mV == TThis::SUCCESS;
	}
	inline void setSuccess() {
		mV = TThis::SUCCESS;
	}
	inline bool isError() const {
		return mV >= TThis::ERROR;
	}
	inline void setError() {
		mV = TThis::ERROR;
	}
	inline bool isFailed() const {
		return mV >= TThis::FAIL;
	}
    inline uint8_t getRaw() const { return mV; }
    inline ETrisult::Type get() const { return mV; }

}; // enum ETrisult
//////////////////////////////////////////////////////////////////////////

}; // namespace qd


using qd::ETrisult;
