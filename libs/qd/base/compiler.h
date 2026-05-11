#pragma once


//------------------------------------------------------------------------
// Compiler detection macros
#if defined(__clang__)
    #define QD_COMPILER_CLANG 1
    #define QD_COMPILER_NAME "clang"
    #define QD_COMPILER_VERSION (__clang_major__ * 100 + __clang_minor__)
#elif defined(__GNUC__) || defined(__GNUG__)
    #define QD_COMPILER_GNUC 1
    #define QD_COMPILER_NAME "gcc"
    #define QD_COMPILER_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)
#elif defined(_MSC_VER)
    #define QD_COMPILER_MSVC 1
    #define QD_COMPILER_NAME "msvc"
    #define QD_COMPILER_VERSION _MSC_VER
#else
    #define QD_COMPILER_UNKNOWN 1
    #define QD_COMPILER_NAME "unknown"
    #define QD_COMPILER_VERSION 0
#endif
//////////////////////////////////////////////////////////////////////////



//------------------------------------------------------------------------
// Force inline macros
#if defined(_MSC_VER)
    #define QD_FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
    #define QD_FORCE_INLINE inline __attribute__((always_inline))
#else
    #define QD_FORCE_INLINE inline
#endif
//////////////////////////////////////////////////////////////////////////



//------------------------------------------------------------------------
// No inline macros
#if defined(_MSC_VER)
    #define QD_NO_INLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
    #define QD_NO_INLINE __attribute__((noinline))
#else
    #define QD_NO_INLINE
#endif
//////////////////////////////////////////////////////////////////////////



//------------------------------------------------------------------------
// Compiler warning control macros
#ifdef _MSC_VER
#define QD_PUSH_VC_WARNING(warning_number) __pragma(warning(push)) __pragma(warning(disable : warning_number))
#define QD_POP_VC_WARNING() __pragma(warning(pop))
#else
#define QD_PUSH_VC_WARNING(warning_number)
#define QD_POP_VC_WARNING()
#endif
//////////////////////////////////////////////////////////////////////////


//------------------------------------------------------------------------
#if defined(QD_COMPILER_CLANG) || defined(EA_COMPILER_CLANG_CL)
#define EACLANGWHELP0_(x) #x
#define EACLANGWHELP1_(x) EACLANGWHELP0_(clang diagnostic ignored x)
// clang-format off
#define QD_PUSH_CLANG_WARNING(w) \
    _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wunknown-warning-option\"") _Pragma(EACLANGWHELP1_(w))
// clang-format on
#define QD_POP_CLANG_WARNING() _Pragma("clang diagnostic pop")
#else
#define QD_PUSH_CLANG_WARNING(w)
#define QD_POP_CLANG_WARNING()
#endif

//////////////////////////////////////////////////////////////////////////


//------------------------------------------------------------------------
// Branch prediction hints
#if defined(__GNUC__) || defined(__clang__)
    #define QD_LIKELY(x)   __builtin_expect(!!(x), 1)
    #define QD_UNLIKELY(x) __builtin_expect(!!(x), 0)
#elif defined(_MSC_VER)
    // MSVC doesn't have a direct equivalent, but we can use empty macros
    // Modern MSVC optimizers handle this well without hints
    #define QD_LIKELY(x)   (x)
    #define QD_UNLIKELY(x) (x)
#else
    #define QD_LIKELY(x)   (x)
    #define QD_UNLIKELY(x) (x)
#endif
//////////////////////////////////////////////////////////////////////////



//------------------------------------------------------------------------
// Nodiscard attribute for return values
#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard) >= 201603L
    #define QD_NODISCARD [[nodiscard]]
#elif defined(_MSC_VER) && _MSC_VER >= 1911
    #define QD_NODISCARD [[nodiscard]]
#elif defined(__GNUC__) && __GNUC__ >= 7
    #define QD_NODISCARD [[nodiscard]]
#elif defined(__clang__) && __clang_major__ >= 4
    #define QD_NODISCARD [[nodiscard]]
#else
    #define QD_NODISCARD
#endif
//////////////////////////////////////////////////////////////////////////




//------------------------------------------------------------------------
// QD_EXCEPTIONS_ENABLED
//
// Defined as 0 or 1. Default follows compiler settings.
// If the compiler has exceptions disabled, forced to 0.
//
///////////////////////////////////////////////////////////////////////////
#if !defined(QD_EXCEPTIONS_ENABLED)
    #if defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)
        #define QD_EXCEPTIONS_ENABLED 0  // off by default even if compiler supports them
    #else
        #define QD_EXCEPTIONS_ENABLED 0
    #endif
#elif (QD_EXCEPTIONS_ENABLED == 1)
    // User wants exceptions, but check compiler support
    #if !defined(__cpp_exceptions) && !defined(__EXCEPTIONS) && !defined(_CPPUNWIND)
        #undef  QD_EXCEPTIONS_ENABLED
        #define QD_EXCEPTIONS_ENABLED 0  // compiler has exceptions disabled
    #endif
#endif
//////////////////////////////////////////////////////////////////////////



#if defined(QD_COMPILER_GNUC)
#define gcc_only_template template
#else
#define gcc_only_template
#endif // __GNUC__


#if defined(QD_COMPILER_GNUC) || defined(QD_COMPILER_CLANG)
#define gcc_template template
#else
#define gcc_template
#endif // __GNUC__


//------------------------------------------------------------------------
// Trap / crash macro
#if defined(__GNUC__) || defined(__clang__)
    #define QD_TRAP() __builtin_trap()
#elif defined(_MSC_VER)
    #define QD_TRAP() __debugbreak()
#else
    #define QD_TRAP() (*(volatile int*)0 = 0)
#endif
//////////////////////////////////////////////////////////////////////////
