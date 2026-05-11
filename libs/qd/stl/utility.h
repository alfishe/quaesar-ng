#pragma once
#if QTD_IS_EASTL
#include <EASTL/utility.h>
#else
#include <utility>
#endif


namespace qtd {
#if QTD_IS_EASTL
using eastl::make_pair;
using eastl::pair;
#else
using std::make_pair;
using std::pair;
#endif
}; // namespace qtd

