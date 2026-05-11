#pragma once
#include <qtdDefines.h>

#if QTD_IS_EASTL
#include <EASTL/vector_set.h>
#else
#include <set>
#endif

namespace qtd {
#if QTD_IS_EASTL
using eastl::vector_set;
#else
// std fallback: vector_set degrades to std::set
template<typename Key, typename Compare = std::less<Key>,
    typename Allocator = std::allocator<Key>>
using vector_set = std::set<Key, Compare, Allocator>;
#endif
}; // namespace qtd

