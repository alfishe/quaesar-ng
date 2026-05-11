#pragma once
#if QTD_IS_EASTL
#include <EASTL/map.h>
#else
#include <map>
#endif


namespace qtd {
#if QTD_IS_EASTL
using eastl::map;
#else
using std::map;
#endif
}; // namespace qtd

