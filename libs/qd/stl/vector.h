#pragma once
#include <qtdDefines.h>

#if QTD_IS_EASTL
#include <EASTL/vector.h>

namespace qtd {
using eastl::vector;
}; // namespace qtd

#else // QTD_IS_STD
#include <vector>

namespace qtd {
using std::vector;
}; // namespace qtd

#endif
