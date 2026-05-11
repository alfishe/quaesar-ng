#pragma once
#include <qd/base/base.h>
#include <qd/debug/assert.h>

//////////////////////////////////////////////////////////////////////////
//
// Example of usage:
//
//
// QD_TRY {
//  if (!invokeAction())
//    QD_THROW_OR_DO(Exception("Can't read file"), return false);
// }
// QD_CATCH(qd::Exception& ex) {
//     m_ApplicationException = ex.getError();
// };
// QD_CATCH(std::bad_alloc& ex) {
//     logErr("STD::BAD_ALLOC FOUND: Not Enough Memory: \"%s\"", ex.what());
// };
//
//////////////////////////////////////////////////////////////////////////

namespace qd {


#define G_DO(cmd, ...)     \
    {                      \
        cmd;               \
        G_DO(__VA_ARGS__); \
    }


//////////////////////////////////////////////////////////////////////////
#if EASTL_EXCEPTIONS_ENABLED

#define QD_THROW_OR_DO(TException, action, ...) throw TException
#define QD_TRY                                  try
#define QD_CATCH(x)                             catch (x)

#else // QD_EXCEPTIONS_ENABLED

#define QD_THROW_OR_DO(TException, action, ...) \
    if constexpr (true) {                       \
        const auto& _tmp_ex_v = TException;     \
        QD_UNUSED(_tmp_ex_v);                   \
        assert(0 && "EXCEPTION");               \
        action;                                 \
        __VA_ARGS__;                            \
    }

#define QD_TRY if (1)

// turning exception into lambda
#define QD_CATCH(TException)          \
    if constexpr (0) [[maybe_unused]] \
    auto _tmp_cb##__LINE__ = [&](TException)

#endif // QD_EXCEPTIONS_ENABLED
//////////////////////////////////////////////////////////////////////////


template<class TException, typename... TArgs>
inline void throw_(TArgs&&... args) {
#if QD_EXCEPTIONS_ENABLED
    throw TException(args...);
#else
    TException ex(args...);
#endif
}

}; // namespace qd
