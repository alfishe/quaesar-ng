#pragma once
#include "qtdDefines.h"
#include <qd/stl/utility.h>

#if QD_USE_SDL
//#include <SDL_endian.h>
#endif // 

#if QTD_IS_EASTL
#include <EASTL/type_traits.h>
namespace qtd { using eastl::endian; }
#else
#include "bit"
namespace qtd { using std::endian; }
#endif


namespace qd {

constexpr inline bool is_big_endian() {
    return qtd::endian::native == qtd::endian::big;
}
constexpr inline bool is_little_endian() {
    return qtd::endian::native == qtd::endian::little;
}

// endian bit's swapping from little-endian to big-endian
template<int TBytesCount>
inline void swapBytes_(void*);

template<>
inline void swapBytes_<1>(void*) {}

template<>
inline void swapBytes_<2>(void* p) {
    QD_PUSH_VC_WARNING(4201) // nameless struct/union
    union TSwap {
        struct {
            uint8_t b0, b1;
        };
    };
    QD_POP_VC_WARNING()
    TSwap* c = reinterpret_cast<TSwap*>(p);
    qtd::swap(c->b0, c->b1);
}

template<>
inline void swapBytes_<4>(void* p) {
    struct TSwap {
        uint8_t b0, b1, b2, b3;
    };
    TSwap* c = reinterpret_cast<TSwap*>(p);
    qtd::swap(c->b0, c->b3);
    qtd::swap(c->b1, c->b2);
}

template<>
inline void swapBytes_<8>(void* p) {
    uint32_t* c = reinterpret_cast<uint32_t*>(p);
    swapBytes_<4>(c);
    swapBytes_<4>(c + 1);
    qtd::swap(*c, *(c + 1));
    (void)p;
}

// swaps bytes order if Platform is BigEndian
template<typename TInt>
inline void swapBytes(TInt* p) {
    swapBytes_<sizeof(TInt)>(p);
}

// swaps bytes order if Platform is BigEndian
template<typename T>
inline void fixByteOrder(T* p) {
    if (qd::is_little_endian())
        swapBytes_<sizeof(T)>(p);
}

QD_NODISCARD inline uint16_t swap16(uint16_t v) {
    return ((v & 0xFF00) >> 8) | ((v & 0x00FF) << 8);
}

QD_NODISCARD inline uint32_t swap32(uint32_t x) {
    return ((x & 0xFF000000) >> 24) | ((x & 0x00FF0000) >> 8) |
            ((x & 0x0000FF00) << 8) | ((x & 0x000000FF) << 24);
}


}; // namespace qd
