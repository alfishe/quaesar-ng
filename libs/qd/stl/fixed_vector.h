#pragma once
#include <qtdDefines.h>

#if QTD_IS_EASTL
#include <EASTL/fixed_vector.h>


namespace qtd {
using eastl::fixed_vector;
}; // namespace qtd

#else //
#include <vector>

namespace qtd {
template<typename T, size_t nodeCount, bool bEnableOverflow = true,
    typename OverflowAllocator = std::allocator<T>>
class fixed_vector : public std::vector<T, OverflowAllocator>
{
    using std::vector<T, OverflowAllocator>::vector;
    using std::vector<T, OverflowAllocator>::operator=;
};

}; // namespace qtd

#endif // QTD_IS_EASTL
