#pragma once
#include <qtdDefines.h>
//------------------------------------------------------------------------
#if QTD_IS_EASTL
#include <EASTL/atomic.h>

namespace qtd {
using eastl::atomic;
}; // namespace qtd

//////////////////////////////////////////////////////////////////////////
#else // QTD_IS_EASTL
//------------------------------------------------------------------------
#include <atomic>

namespace qtd {
using std::atomic;
}; // namespace qtd

#endif // QTD_IS_STD
//////////////////////////////////////////////////////////////////////////
