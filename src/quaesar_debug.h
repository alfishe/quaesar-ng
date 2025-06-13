#pragma once
#include "SDL_log.h"


#ifdef TRACE
#undef TRACE
#endif


#if 0  // debug unimplemented
#define TRACE() SDL_Log("WARN: Using of unimplemented function: '%s()'", __func__)
#else
#define TRACE()
#endif


extern void debug(const char* x, ...);
