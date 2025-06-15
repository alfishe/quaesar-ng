#pragma once
#include <qd/base/base.h>


namespace qd {


// UNIX TIME, it's Seconds from 1970
//__time64_t
typedef int64_t TTime64;
typedef uint32_t TTime32U;
static constexpr uint32_t TTime32U_MAX = UINT32_MAX;  //~0u;
static constexpr int64_t TTime64_MAX = INT64_MAX;

};