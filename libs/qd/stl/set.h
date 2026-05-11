#pragma once
#include <qtdDefines.h>

#if QTD_IS_EASTL
#include <EASTL/set.h>
#else
#include <set>
#endif

namespace qtd {
#if QTD_IS_EASTL
using eastl::set;
#else
using std::set;
#endif
}; // namespace qtd



namespace qd {
#if QTD_IS_EASTL
using eastl::set;
#else
using std::set;
#endif
}; // namespace qd

