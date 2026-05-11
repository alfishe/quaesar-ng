#pragma once
#include <qtdDefines.h>

#if QTD_IS_EASTL
#include <EASTL/unordered_set.h>
#else
#include <unordered_set>
#endif

namespace qtd {
#if QTD_IS_EASTL
using eastl::unordered_set;
#else
using std::unordered_set;
#endif
}; // namespace qtd

