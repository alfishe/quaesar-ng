#pragma once
#include <qd/base/compiler.h>
#include <qd/stl/string.h>
#if QD_USE_SDL
#include <SDL_assert.h>
#endif

#undef assert

#if QD_USE_SDL

template<typename T>
constexpr inline T _def_expr_const(T value) {
    return value;
}

#define assert(condition) SDL_assert(_def_expr_const(!!(condition)))


#if (SDL_ASSERT_LEVEL > 0)
//------------------------------------------------------------------------
#define QDSDL_enabled_assert_2(condition, pFormat, ...)                                                                       \
    do {                                                                                                                      \
        while (!(condition)) {                                                                                                \
            qtd::string textFormat = qd::string_format(pFormat, ##__VA_ARGS__);                                               \
            struct SDL_AssertData sdl_assert_data = {0, 0, textFormat.c_str(), 0, 0, 0, 0};                                   \
            const SDL_AssertState sdl_assert_state = SDL_ReportAssertion(&sdl_assert_data, SDL_FUNCTION, SDL_FILE, SDL_LINE); \
            if (sdl_assert_state == SDL_ASSERTION_RETRY) {                                                                    \
                continue; /* go again. */                                                                                     \
            }                                                                                                                 \
            else if (sdl_assert_state == SDL_ASSERTION_BREAK) {                                                               \
                SDL_TriggerBreakpoint();                                                                                      \
            }                                                                                                                 \
            break; /* not retrying. */                                                                                        \
        }                                                                                                                     \
    } while (SDL_NULL_WHILE_LOOP_CONDITION)

#endif // (SDL_ASSERT_LEVEL > 0)
//////////////////////////////////////////////////////////////////////////


#ifndef assert2
#if SDL_ASSERT_LEVEL > 0 /* normal settings. */
#define assert2(expr, pFormat, ...) QDSDL_enabled_assert_2((expr), pFormat, ##__VA_ARGS__)
#else
#define assert2(expr, pFormat, ...) SDL_assert(c_def(0 != (expr)) && pFormat)
#endif // #if SDL_ASSERT_LEVEL
#endif

#define ASSERT_F(expr, pFormat, ...) QDSDL_enabled_assert_2((expr), pFormat, ##__VA_ARGS__)

#define QD_HALT(pFormat, ...) QDSDL_enabled_assert_2(0, pFormat, ##__VA_ARGS__)


//------------------------------------------------------------------------
#define ASSERT_AND_DO(expression, do_action, ...)                                                                         \
    if (QD_UNLIKELY(!(expression))) {                                                                                     \
        qtd::string textFormat = qd::string_format(__VA_ARGS__);                                                          \
        struct SDL_AssertData sdl_assert_data = {0, 0, textFormat.c_str(), 0, 0, 0, 0};                                   \
        const SDL_AssertState sdl_assert_state = SDL_ReportAssertion(&sdl_assert_data, SDL_FUNCTION, SDL_FILE, SDL_LINE); \
        if (sdl_assert_state == SDL_ASSERTION_BREAK)                                                                      \
            SDL_TriggerBreakpoint();                                                                                      \
        do_action;                                                                                                        \
    }                                                                                                                     \
    else
//////////////////////////////////////////////////////////////////////////

#else
#include <cassert> // use standard assert

#define assert2(expr, format, ...)                                                                                \
    do {                                                                                                           \
        if (!(expr)) {                                                                                             \
            qtd::string textFormat = qd::string_format(format, ##__VA_ARGS__);                                     \
            fprintf(stderr, "Assertion failed: %s\nFile: %s\nLine: %d\n", textFormat.c_str(), __FILE__, __LINE__); \
            assert(expr);                                                                                          \
        }                                                                                                          \
    } while (0)

#define ASSERT_F(expr, pFormat, ...) assert2(expr, pFormat, ##__VA_ARGS__)

#define QD_HALT(pFormat, ...)                    \
    do {                                         \
        ASSERT_F(false, pFormat, ##__VA_ARGS__); \
        QD_TRAP();                               \
    } while (0)

//------------------------------------------------------------------------
#define ASSERT_AND_DO(expression, do_action, ...) \
    if (QD_UNLIKELY(!(expression))) {             \
        do_action;                                \
    } /*TODO*/                                    \
    else
//////////////////////////////////////////////////////////////////////////


#endif
