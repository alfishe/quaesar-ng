#pragma once
#include <qtdDefines.h>

#if QTD_IS_EASTL
#include <EASTL/fixed_function.h>
#else
#include <functional>
#endif


namespace qtd {
#if QTD_IS_EASTL
using eastl::fixed_function;
#else
// std fallback: fixed_function degrades to std::function
template<int SizeInBytes, typename Signature>
using fixed_function = std::function<Signature>;
#endif
}; // namespace qtd

