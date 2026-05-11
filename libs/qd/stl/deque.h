#pragma once
#if QTD_IS_EASTL
#include <EASTL/deque.h>
#else
#include <deque>
#endif


namespace qtd {
#if QTD_IS_EASTL
using eastl::deque;
#else
using std::deque;
#endif
}; // namespace qtd

