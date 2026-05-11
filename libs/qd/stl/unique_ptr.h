#pragma once
#include <qtdDefines.h>

#if QTD_IS_EASTL
#include <EASTL/unique_ptr.h>
#else
#include <memory>
#endif

namespace qd {

//using eastl::unique_ptr;
//using eastl::make_unique;

}; // namespace qd


namespace qtd {

#if QTD_IS_EASTL
using eastl::unique_ptr;
using eastl::make_unique;
#else
using std::unique_ptr;
using std::make_unique;
#endif

}; // namespace qtd
