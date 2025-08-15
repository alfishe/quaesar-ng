#pragma once
#include <EASTL/unique_ptr.h>


namespace qd {

// template<typename T, typename Deleter = eastl::default_delete<T> >
// using unique_ptr = eastl::unique_ptr<T, Deleter>;

using eastl::unique_ptr;
using eastl::make_unique;


}; // namespace qd
