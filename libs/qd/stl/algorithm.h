#pragma once
#include <EASTL/algorithm.h>
#include <EASTL/functional.h>


namespace qtd
{
using eastl::find;
using eastl::begin;
using eastl::data;
using eastl::equal_to;
using eastl::hash;
using eastl::size;


template<class TVector, typename TPredicate>
int find_index(const TVector& vec, TPredicate predicate)
{
    auto it = eastl::find_if(vec.begin(), vec.end(), predicate);
    return (it != vec.end()) ? static_cast<int>(eastl::distance(vec.begin(), it)) : -1;
}

}; // namespace qtd



namespace qd {
using eastl::find;
using eastl::begin;
using eastl::data;
using eastl::equal_to;
using eastl::hash;
using eastl::size;


template<class TVector, typename TPredicate>
int find_index(const TVector& vec, TPredicate predicate)
{
    auto it = eastl::find_if(vec.begin(), vec.end(), predicate);
    return (it != vec.end()) ? static_cast<int>(eastl::distance(vec.begin(), it)) : -1;
}

}; // namespace qd

