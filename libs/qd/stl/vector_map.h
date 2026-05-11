#pragma once
#include <qtdDefines.h>

#if QTD_IS_EASTL
#include <EASTL/vector_map.h>
#else
#include <map>
#endif

namespace qtd {
#if QTD_IS_EASTL
using eastl::vector_map;
#else
// std fallback: vector_map degrades to std::map
template<typename Key, typename T, typename Compare = std::less<Key>,
    typename Allocator = std::allocator<std::pair<const Key, T>>>
using vector_map = std::map<Key, T, Compare, Allocator>;
#endif
}; // namespace qtd

