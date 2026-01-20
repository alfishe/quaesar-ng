#include <EASTL/internal/config.h>
#include <qd/stl/string.h>
#include <SDL_assert.h>


#ifndef assert
#define assert(condition)       \
    EA_DISABLE_VC_WARNING(4127) \
    SDL_assert(condition);      \
    EA_RESTORE_VC_WARNING()
#endif


#if (SDL_ASSERT_LEVEL > 0)

//------------------------------------------------------------------------
#define QDSDL_enabled_assert_2(condition, pFormat, ...)                                     \
    do                                                                                      \
    {                                                                                       \
        while (!(condition))                                                                \
        {                                                                                   \
            qtd::string textFormat = qd::string_format(pFormat, ##__VA_ARGS__);              \
            struct SDL_AssertData sdl_assert_data = {0, 0, textFormat.c_str(), 0, 0, 0, 0}; \
            const SDL_AssertState sdl_assert_state =                                        \
                SDL_ReportAssertion(&sdl_assert_data, SDL_FUNCTION, SDL_FILE, SDL_LINE);    \
            if (sdl_assert_state == SDL_ASSERTION_RETRY)                                    \
            {                                                                               \
                continue; /* go again. */                                                   \
            }                                                                               \
            else if (sdl_assert_state == SDL_ASSERTION_BREAK)                               \
            {                                                                               \
                SDL_TriggerBreakpoint();                                                    \
            }                                                                               \
            break; /* not retrying. */                                                      \
        }                                                                                   \
    } while (SDL_NULL_WHILE_LOOP_CONDITION)

#endif /* enabled assertions support code */
//////////////////////////////////////////////////////////////////////////


#ifndef assert2
#  if SDL_ASSERT_LEVEL > 0 /* normal settings. */
#    define assert2(expr, pFormat, ...) QDSDL_enabled_assert_2((expr), pFormat, ##__VA_ARGS__)
#  else
#  define assert2(expr, pFormat, ...) SDL_assert(c_def(0 != (expr)) && pFormat)
#     endif // #if SDL_ASSERT_LEVEL
#endif


#define ASSERT_F(expr, format, ...) EASTL_ASSERT_MSG(expr, qd::string_format(format, __VA_ARGS__).c_str());


#define QD_HALT(pFormat, ...) QDSDL_enabled_assert_2(0, pFormat, ##__VA_ARGS__)


//------------------------------------------------------------------------
#define ASSERT_AND_DO(expression, do_action, ...)                                       \
    if (EASTL_UNLIKELY(!(expression)))                                                  \
    {                                                                                   \
        qtd::string textFormat = qd::string_format(__VA_ARGS__);                         \
        struct SDL_AssertData sdl_assert_data = {0, 0, textFormat.c_str(), 0, 0, 0, 0}; \
        const SDL_AssertState sdl_assert_state =                                        \
            SDL_ReportAssertion(&sdl_assert_data, SDL_FUNCTION, SDL_FILE, SDL_LINE);    \
        if (sdl_assert_state == SDL_ASSERTION_BREAK)                                    \
            SDL_TriggerBreakpoint();                                                    \
        do_action;                                                                      \
    }                                                                                   \
    else
//////////////////////////////////////////////////////////////////////////
