#pragma once
#include <EASTL/span.h>
#include "vector.h"

namespace qtd
{
using eastl::span;

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
	
}; // namespace qd
