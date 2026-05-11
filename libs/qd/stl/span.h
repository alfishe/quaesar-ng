#pragma once
#include <qtdDefines.h>
#include "vector.h"

#if QTD_IS_EASTL
#include <EASTL/span.h>
#else
#include <span>
#endif

namespace qtd
{
#if QTD_IS_EASTL
using eastl::span;
#else
using std::span;
#endif

}; // namespace qtd


namespace qd
{
template<typename T>
qtd::span<T*> make_span(qtd::vector<T*>& vec) {
    return {vec.data(), vec.size()};
}

template<typename T>
qtd::span<const T*> make_span(const qtd::vector<T*>& vec) {
    return {vec.data(), vec.size()};
}

template<typename T1, typename TList>
qtd::span<T1> make_span_cast_(TList& vec) {
    return {reinterpret_cast<T1*>(vec.data()), vec.size()};
}

}; // namespace qd
