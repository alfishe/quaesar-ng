#pragma once
#include <qtdDefines.h>

#if QTD_IS_EASTL
#include <EASTL/functional.h>
#else
#include <functional>
#endif


namespace qtd
{
#if QTD_IS_EASTL
using eastl::function;
#else
using std::function;
#endif
}; // namespace qtd

