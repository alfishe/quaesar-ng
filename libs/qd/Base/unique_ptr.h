#pragma once
#include <EASTL/unique_ptr.h>

namespace qd {
	
template <typename T>
using unique_ptr = eastl::unique_ptr<T>;

template <typename T, typename... Args>
constexpr auto make_unique(Args&&... args) -> unique_ptr<T> {
    return eastl::make_unique<T>(std::forward<Args>(args)...);
}

}; // namespace qd
