#pragma once
#include <qtdDefines.h>

#if QTD_IS_EASTL
#include <EASTL/algorithm.h>
#include <EASTL/functional.h>
#include <EASTL/sort.h>
#else
#include <algorithm>
#include <functional>
#include <iterator>
#include <type_traits>
#endif


namespace qtd
{
#if QTD_IS_EASTL
using eastl::find;
using eastl::find_if;
using eastl::begin;
using eastl::data;
using eastl::equal_to;
using eastl::hash;
using eastl::size;
using eastl::move;
using eastl::forward;
using eastl::swap;
using eastl::distance;
using eastl::upper_bound;
using eastl::stable_sort;
using eastl::is_volatile_v; // determine whether type argument is volatile qualified
#else
using std::find;
using std::find_if;
using std::begin;
using std::data;
using std::equal_to;
using std::hash;
using std::size;
using std::move;
using std::forward;
using std::swap;
using std::distance;
using std::upper_bound;
using std::stable_sort;
using std::is_volatile_v;
#endif


template<class TVector, typename TPredicate>
int find_index(const TVector& vec, TPredicate predicate)
{
    auto it = qtd::find_if(vec.begin(), vec.end(), predicate);
    return (it != vec.end()) ? static_cast<int>(qtd::distance(vec.begin(), it)) : -1;
}

}; // namespace qtd
