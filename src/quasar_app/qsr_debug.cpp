#include "qsr_debug.h"
#include "EASTL/fixed_string.h"


void debug(const char* format, ...) {
    eastl::fixed_string<char, 1024, false> formatBuffer;
    formatBuffer.sprintf("[%s] %s", __func__, format);
    va_list args;
    va_start(args, format);
    SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_DEBUG, formatBuffer.c_str(), args);
    va_end(args);
}
