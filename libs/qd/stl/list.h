#pragma once
#include <qtdDefines.h>

#if QTD_IS_EASTL
#include <EASTL/list.h>
#else
#include <list>
#endif

namespace qtd {
#if QTD_IS_EASTL
using eastl::list;
#else
using std::list;
#endif
}; // namespace qtd



namespace qd {
#if QTD_IS_EASTL
using eastl::list;
#else
using std::list;
#endif
}; // namespace qd

