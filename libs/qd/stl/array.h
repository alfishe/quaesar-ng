#pragma once
#include <qtdDefines.h>

#if QTD_IS_EASTL
#include <EASTL/array.h>
#else
#include <array>
#endif

namespace qtd {
#if QTD_IS_EASTL
using eastl::array;
#else
using std::array;
#endif
}; // namespace qtd

