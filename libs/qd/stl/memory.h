#pragma once
#include <qtdDefines.h>

#if QTD_IS_EASTL
#include <EASTL/memory.h>
#else
#include <memory>
#endif

namespace qtd {
#if QTD_IS_EASTL
using eastl::destroy_at;
#else
using std::destroy_at;
#endif
}; // namespace qtd

