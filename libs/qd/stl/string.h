#pragma once
#include "qd/stl/forwardDecl.h"
#include <EASTL/string.h>
#include <EASTL/string_view.h>


namespace qd {

using string = eastl::basic_string<char, eastl::allocator>;
using string_view = eastl::basic_string_view<char>;
using wstring_view = eastl::basic_string_view<wchar_t>;


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
//////////////////////////////////////////////////////////////////////////


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
