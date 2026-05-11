#pragma once
#include <qtdDefines.h>

#if QTD_IS_EASTL
#include <EASTL/optional.h>
#else
#include <optional>
#endif


namespace qtd
{
#if QTD_IS_EASTL
using eastl::optional;
#else
using std::optional;
#endif
}; // namespace qtd



namespace qd
{
#if QTD_IS_EASTL
using eastl::optional;
#else
using std::optional;
#endif
}; // namespace qd

