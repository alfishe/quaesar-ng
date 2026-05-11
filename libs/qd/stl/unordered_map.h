#pragma once
#include <qtdDefines.h>

#if QTD_IS_EASTL
#include <EASTL/unordered_map.h>
#else
#include <unordered_map>
#endif

namespace qtd {
#if QTD_IS_EASTL
using eastl::unordered_map;
#else
using std::unordered_map;
#endif
}; // namespace qtd

