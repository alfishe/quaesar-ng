#include <SDL_assert.h>
#include <qdIce/qdBase/string.h>
#include <EASTL/internal/config.h>


#ifndef assert
#define assert(condition) SDL_assert(condition)
#endif


#if (SDL_ASSERT_LEVEL > 0)

#define QDSDL_enabled_assert_2(condition, pFormat, ...)                                     \
    do {                                                                                    \
        while (!(condition)) {                                                              \
            qd::string textFormat = qd::stringFormat(pFormat, ##__VA_ARGS__);               \
            struct SDL_AssertData sdl_assert_data = {0, 0, textFormat.c_str(), 0, 0, 0, 0}; \
            const SDL_AssertState sdl_assert_state =                                        \
                SDL_ReportAssertion(&sdl_assert_data, SDL_FUNCTION, SDL_FILE, SDL_LINE);    \
            if (sdl_assert_state == SDL_ASSERTION_RETRY) {                                  \
                continue; /* go again. */                                                   \
            } else if (sdl_assert_state == SDL_ASSERTION_BREAK) {                           \
                SDL_TriggerBreakpoint();                                                    \
            }                                                                               \
            break; /* not retrying. */                                                      \
        }                                                                                   \
    } while (SDL_NULL_WHILE_LOOP_CONDITION)

#endif /* enabled assertions support code */


#if SDL_ASSERT_LEVEL > 0 /* normal settings. */
#define assert2(expr, pFormat, ...) QDSDL_enabled_assert_2((expr), pFormat, ##__VA_ARGS__)
#else
#define assert2(expr, pFormat, ...) SDL_assert(c_def(0 != (expr)) && pFormat)
#endif  // #if SDL_ASSERT_LEVEL



#define QD_HALT() { SDL_ASSERT_LEVEL( __FILE__, __LINE__, "HALT" ); EASTL_DEBUG_BREAK(); }
