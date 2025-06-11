#pragma once

//------------------------------------------------------------------------
// It isn't work with 0 parameters due to passed like: QD_FOR_EACH_NARG_(, QD_FOR_EACH_RSEQ_N()
//------------------------------------------------------------------------
#define QD_GET_ARGS_COUNT(...) QD_FOR_EACH_NARG_(__VA_ARGS__, QD_FOR_EACH_RSEQ_N())


#define QD_FOR_EACH(WHAT, ...) QD_FOR_EACH_(QD_GET_ARGS_COUNT(__VA_ARGS__), WHAT, __VA_ARGS__)


//------------------------------------------------------------------------
// clang-format off
#define QD_EXPAND(x) x
#define QD_FOR_EACH_1(WHAT, X)       WHAT(X)
#define QD_FOR_EACH_2(WHAT, X, ...)  WHAT(X) QD_EXPAND(QD_FOR_EACH_1(WHAT, __VA_ARGS__))
#define QD_FOR_EACH_3(WHAT, X, ...)  WHAT(X) QD_EXPAND(QD_FOR_EACH_2(WHAT, __VA_ARGS__))
#define QD_FOR_EACH_4(WHAT, X, ...)  WHAT(X) QD_EXPAND(QD_FOR_EACH_3(WHAT, __VA_ARGS__))
#define QD_FOR_EACH_5(WHAT, X, ...)  WHAT(X) QD_EXPAND(QD_FOR_EACH_4(WHAT, __VA_ARGS__))
#define QD_FOR_EACH_6(WHAT, X, ...)  WHAT(X) QD_EXPAND(QD_FOR_EACH_5(WHAT, __VA_ARGS__))
#define QD_FOR_EACH_7(WHAT, X, ...)  WHAT(X) QD_EXPAND(QD_FOR_EACH_6(WHAT, __VA_ARGS__))
#define QD_FOR_EACH_8(WHAT, X, ...)  WHAT(X) QD_EXPAND(QD_FOR_EACH_7(WHAT, __VA_ARGS__))
#define QD_FOR_EACH_9(WHAT, X, ...)  WHAT(X) QD_EXPAND(QD_FOR_EACH_8(WHAT, __VA_ARGS__))
#define QD_FOR_EACH_10(WHAT, X, ...) WHAT(X) QD_EXPAND(QD_FOR_EACH_9(WHAT, __VA_ARGS__))
#define QD_FOR_EACH_11(WHAT, X, ...) WHAT(X) QD_EXPAND(QD_FOR_EACH_10(WHAT, __VA_ARGS__))
#define QD_FOR_EACH_12(WHAT, X, ...) WHAT(X) QD_EXPAND(QD_FOR_EACH_11(WHAT, __VA_ARGS__))
#define QD_FOR_EACH_13(WHAT, X, ...) WHAT(X) QD_EXPAND(QD_FOR_EACH_12(WHAT, __VA_ARGS__))
#define QD_FOR_EACH_14(WHAT, X, ...) WHAT(X) QD_EXPAND(QD_FOR_EACH_13(WHAT, __VA_ARGS__))
#define QD_FOR_EACH_15(WHAT, X, ...) WHAT(X) QD_EXPAND(QD_FOR_EACH_14(WHAT, __VA_ARGS__))

#define QD_FOR_EACH_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _LAST_N, ...) _LAST_N
#define QD_FOR_EACH_RSEQ_N()       15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
#define QD_FOR_EACH_NARG_(...)     QD_EXPAND(QD_FOR_EACH_ARG_N(__VA_ARGS__))
#define QD_CONCATENATE(x, y)       x##y
#define QD_FOR_EACH_(N, WHAT, ...) QD_EXPAND(QD_CONCATENATE(QD_FOR_EACH_, N)(WHAT, __VA_ARGS__))


// clang-format on

static_assert(QD_GET_ARGS_COUNT() == 1);
static_assert(QD_GET_ARGS_COUNT(a1) == 1);
static_assert(QD_GET_ARGS_COUNT(a1, a2) == 2);
