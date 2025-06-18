#pragma once
#include <EASTL/vector_map.h>

namespace qd
{

//template<typename Key, typename T, typename Compare = eastl::less<Key>, typename Allocator = EASTLAllocatorType>
using eastl::vector_map; // = eastl::vector_map<Key, T, Compare, Allocator, qd::vector<eastl::pair<Key, T>, Allocator> >;

}; // namespace qd

