#pragma once
#include <EABase/eabase.h>
#include <EASTL/internal/config.h>
#include <stdint.h>

using THash32 = uint32_t;

#define SIDENT(x)      x
#define STRINGIFY(x)   _STRINGIFY2(x) /* #x */
#define _STRINGIFY2(x) #x
#define CON(a, b)      a##b
#define PASTE(a, b)    CON(a, b)


// clang-format off

#define SAFE_DELETE(p)          { delete (p); (p) = nullptr; }
#define SAFE_FREE(p)            { free(p); (p) = nullptr; }
#define SAFE_DESTROY(p)         { if (p) { (p)->destroy(); (p) = nullptr; } }
#define SAFE_DESTROY_AND_DELETE(p) { if (p) { (p)->destroy(); delete (p); (p) = nullptr; } }

//------------------------------------------------------------------------
// Forward declaration for class and struct
#define FORWARD_DECLARATION_1(c) class c;
#define FORWARD_DECLARATION_2(n1, c) namespace n1 { class c; }
#define FORWARD_DECLARATION_3(n1, n2, c) namespace n1 { namespace n2 { class c; } }
#define FORWARD_DECLARATION_4(n1, n2, n3, c) namespace n1 { namespace n2 { namespace n3 { class c; } } }

#define FORWARD_DECLARATION_1S(c) struct c;
#define FORWARD_DECLARATION_2S(n1, c) namespace n1 { struct c; }
#define FORWARD_DECLARATION_3S(n1, n2, c) namespace n1 { namespace n2 { struct c; } }
#define FORWARD_DECLARATION_4S(n1, n2, n3, c) namespace n1 { namespace n2 { namespace n3 { struct c; } } }
//------------------------------------------------------------------------
// clang-format on


template<typename T>
T c_def(T v)
{
    return v;
}

// place to set breakpoint for debugging
#define BPT() [](){return 0;}();
