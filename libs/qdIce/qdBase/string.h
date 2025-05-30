#pragma once
#include <EASTL/string.h>
#include "qdIce/qdSTL/forwardDecl.h"

namespace qd
{

inline string stringFormat(const char* pFormat, ...) {
    va_list argList;
    va_start(argList, pFormat);
    string result;
    result.sprintf_va_list(pFormat, argList);
    va_end(argList);
    return result;
}


}; // namespace qd

