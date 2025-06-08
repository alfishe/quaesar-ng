#pragma once
#include <EASTL/version.h>
#include <qdIce/qdBase/base.h>
#include <qdIce/qdDebug/assert.h>


namespace qd {


#define G_DO(cmd, ...)     \
    {                      \
        cmd;               \
        G_DO(__VA_ARGS__); \
    }


//////////////////////////////////////////////////////////////////////////
#if EASTL_EXCEPTIONS_ENABLED

#define G_THROW_OR_DO(TException, action, ...) throw TException
#define G_TRY                                  try
#define G_CATCH(x)                             catch (x)

#else // EASTL_EXCEPTIONS_ENABLED

#define G_THROW_OR_DO(TException, action, ...) \
    if (c_def(true))                           \
    {                                          \
        const auto& _tmp_ex_v = TException;    \
        EA_UNUSED(_tmp_ex_v);                  \
        assert(0 && "EXCEPTION");              \
        action;                                \
        __VA_ARGS__;                           \
    }

#define G_TRY if (1)

// turning exception into lambda
#define G_CATCH(TException) \
    if (c_def(0))           \
    auto _tmp_cb##__LINE__ = [&](TException)

#endif // EASTL_EXCEPTIONS_ENABLED
//////////////////////////////////////////////////////////////////////////


template<class TException, typename... TArgs>
inline void throw_(TArgs&&... args)
{
#if EASTL_EXCEPTIONS_ENABLED
    throw TException(args...);
#else
    TException ex(args...);
#endif
}

}; // namespace qd
