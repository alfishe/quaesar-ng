#pragma once
#include "qdIce/qdBase/base.h"



namespace eastl {
class allocator;

template<typename T>
struct equal_to;

template<typename T>
struct hash;

template<typename T, typename Allocator>
class basic_string;

template<typename T>
class basic_string_view;

template<typename T, int nodeCount, bool bEnableOverflow, typename OverflowAllocator>
class fixed_string;

template<typename T, typename Allocator>
class vector;

template<typename Key, typename T, typename Compare, typename Allocator, typename RandomAccessContainer>
class vector_map;

template<typename T, size_t nodeCount, bool bEnableOverflow, typename OverflowAllocator>
class fixed_vector;

template<typename T, size_t N>
struct array;

template<typename Key, typename T, typename Hash, typename Predicate, typename Allocator, bool bCacheHashCode>
class hash_map;

template<typename T1, typename T2>
struct pair;
} // namespace eastl

//-------------------------------------------------------------------------

namespace qd {

using string = eastl::basic_string<char, eastl::allocator>;
using string_view = eastl::basic_string_view<char>;
using wstring_view = eastl::basic_string_view<wchar_t>;


template<size_t S>
using TInlineString = eastl::fixed_string<char, S, true, eastl::allocator>;
using InlineString = eastl::fixed_string<char, 255, true, eastl::allocator>;


template<typename T, typename Allocator = EASTLAllocatorType>
using vector = eastl::vector<T, Allocator>;


template<typename Key, typename T, typename Compare = eastl::less<Key>, typename Allocator = EASTLAllocatorType>
using vector_map = eastl::vector_map<Key, T, Compare, Allocator, qd::vector<eastl::pair<Key, T>, Allocator> >;


template<typename T, size_t S>
using TInlineVector = eastl::fixed_vector<T, S, true, eastl::allocator>;
template<typename T, size_t S>
using array = eastl::array<T, S>;


using Blob = vector<uint8_t>;


template<typename K, typename V>
using THashMap = eastl::hash_map<K, V, eastl::hash<K>, eastl::equal_to<K>, eastl::allocator, false>;


template<typename K, typename V>
using TPair = eastl::pair<K, V>;


using eastl::allocator;

} // namespace qd
