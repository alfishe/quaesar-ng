#pragma once
#include <qtdDefines.h>

#if QTD_IS_EASTL
#include <EASTL/bitvector.h>
#else
#include <vector>
#endif

namespace qtd {
#if QTD_IS_EASTL
using bitvector = eastl::bitvector<>;
#else
using bitvector = std::vector<bool>;
#endif
}; // namespace qtd

