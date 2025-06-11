#pragma once
#include "qd/STL/forwardDecl.h"
#include <EASTL/string.h>

namespace qd {

template<class TString>
inline const char* CC(const TString& str)
{
    return str.c_str();
}
inline const char* CC(const char* pStr)
{
    return pStr;
}
inline const wchar_t* CC(const wchar_t* pStr)
{
    return pStr;
}
inline const wchar_t* CW(const wchar_t* pStr)
{
    return pStr;
}


inline string stringFormat(const char* pFormat, ...)
{
    va_list argList;
    va_start(argList, pFormat);
    string result;
    result.sprintf_va_list(pFormat, argList);
    va_end(argList);
    return result;
}


}; // namespace qd
