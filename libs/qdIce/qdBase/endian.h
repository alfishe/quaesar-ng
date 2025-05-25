#pragma once
#include <EASTL/type_traits.h>
#include <EASTL/utility.h>
#include <SDL_endian.h>


namespace qd {

// endian bit's swapping from little-endian to big-endian
template <int TBytesCount>
inline void swapBytes_(void*);

template <>
inline void swapBytes_<1>(void*) {
}

template <>
inline void swapBytes_<2>(void* p) {
    union TSwap {
        struct {
            uint8_t b0, b1;
        };
    };
    TSwap* c = reinterpret_cast<TSwap*>(p);
    eastl::swap(c->b0, c->b1);
}

template <>
inline void swapBytes_<4>(void* p) {
    struct TSwap {
        uint8_t b0, b1, b2, b3;
    };
    TSwap* c = reinterpret_cast<TSwap*>(p);
    eastl::swap(c->b0, c->b3);
    eastl::swap(c->b1, c->b2);
}

template <>
inline void swapBytes_<8>(void* p) {
    uint32_t* c = reinterpret_cast<uint32_t*>(p);
    swapBytes_<4>(c);
    swapBytes_<4>(c + 1);
    eastl::swap(*c, *(c + 1));
    (void)p;
}

// swaps bytes order if Platform is BigEndian
template <typename TInt>
inline void swapBytes(TInt* p) {
    swapBytes_<sizeof(TInt)>(p);
}


// swaps bytes order if Platform is BigEndian
template <typename T>
inline void fixByteOrder(T* p) {
    if (eastl::endian::native == eastl::endian::little)
        swapBytes_<sizeof(T)>(p);
}


EA_NODISCARD inline uint16_t swap16(uint16_t v) {
    return SDL_Swap16(v);
}

EA_NODISCARD inline uint32_t swap32(uint32_t x) {
    return SDL_Swap32(x);
}


};  // namespace qd
