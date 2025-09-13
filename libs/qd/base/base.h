#pragma once
#include <EABase/eabase.h>
#include <EASTL/internal/config.h>
#include <stdint.h>

using THash32 = uint32_t;

// clang-format off

#define SIDENT(x)      x
#define STRINGIFY(x)   _STRINGIFY2(x) /* #x */
#define _STRINGIFY2(x) #x
#define CON(a, b)      a##b
#define PASTE(a, b)    CON(a, b)

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

// dummy place to set breakpoint if needed
#define BPT() [](){return 0;}();

template<typename T> T c_def(T v) { return v; }
template<typename T> constexpr inline T c_expr(T value) { return value; }


EA_DISABLE_VC_WARNING(4100) // unreferenced formal parameter
template<typename... T> inline void unused(T&&... x) { (void(sizeof...(x))); }
#define G_UNUSED(...) unused(__VA_ARGS__)
EA_RESTORE_VC_WARNING()


#define MAKE4C(a, b, c, d) ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))
#define _MAKE4C(s) MAKE4C(s[0], s[1], s[2], s[3])

namespace qd {
	static constexpr uint32_t _noPos = UINT32_MAX; // ~0u
}; // namespace qd


//------------------------------------------------------------------------
// COMPILER
#ifdef _MSC_VER
//#  pragma warning(disable :4100) // unreferenced formal parameter
//#  pragma warning(disable :4512) // assignment operator could not be generated
//#  pragma warning(disable :4201)  // nonstandard extension used : nameless struct/union
//#  pragma warning(default :4263) // equivalent -Winconsistent-missing-override
//#  pragma warning(default :4266)
#endif // _MSC_VER



// clang-format on
//////////////////////////////////////////////////////////////////////////
