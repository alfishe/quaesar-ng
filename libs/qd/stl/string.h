#pragma once
#include <EASTL/fixed_string.h>
#include <EASTL/string.h>
#include <EASTL/string_view.h>


namespace qtd {
using eastl::string;
using eastl::string_view;
using eastl::to_string;
};

namespace qd {
using eastl::string;
using eastl::string_view;
};


namespace qd {

template<size_t S>
using InlineString_ = eastl::fixed_string<char, S, true, eastl::allocator>;
using InlineString = eastl::fixed_string<char, 255, true, eastl::allocator>;

// using string = eastl::basic_string<char, eastl::allocator>;
// using string_view = eastl::basic_string_view<char>;
using wstring_view = eastl::basic_string_view<wchar_t>;


template<size_t TCapacity = 32>
inline qtd::string string_format(const char* pFormat, ...) {
    va_list argList;
    va_start(argList, pFormat);
    qtd::string result;
    result.reserve(TCapacity);
    result.sprintf_va_list(pFormat, argList);
    va_end(argList);
    return result;
}


template<size_t TCapacity = 32>
inline qtd::string string_format_v(const char* pFormat, va_list argList) {
    qtd::string result;
    result.reserve(TCapacity);
    result.sprintf_va_list(pFormat, argList);
    return result;
}


inline qtd::string string_format() { // for zero args template
    return {};
}


template<class TString>
bool starts_with(const TString& str, const char* start) {
    size_t len = strlen(start);
    if (str.length() < len)
        return false;
    return memcmp(str.data(), start, len) == 0;
}


template<class TString>
bool starts_with(const TString& str, const TString& start) {
    if (str.length() < start.length())
        return false;
    return memcmp(str.data(), start.data(), start.length()) == 0;
}


template<class TString>
bool ends_with(const TString& str, const char* end) {
    size_t len = strlen(end);
    if (str.length() < len)
        return false;
    return memcmp(str.data() + str.length() - len, end, len) == 0;
}


template<class TString>
bool ends_with(const TString& str, const TString& end) {
    if (str.length() < end.length())
        return false;
    return memcmp(str.data() + str.length() - end.length(), end.data(), end.length()) == 0;
}


inline char to_upper(char c) {
    return (c >= 'a' && c <= 'z') ? c &= ~32 : c;
}


inline bool is_blank(char c) {
    return c == ' ' || c == '\t';
}


inline bool is_blankW(unsigned int c) {
    return c == ' ' || c == '\t' || c == 0x3000;
}


inline bool is_digit(char c) {
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}


inline int stricmp(const char* str1, const char* str2) {
    int d;
    while ((d = qd::to_upper(*str2) - qd::to_upper(*str1)) == 0 && *str1) {
        str1++;
        str2++;
    }
    return d;
}


inline int strnicmp(const char* str1, const char* str2, size_t count) {
    int d = 0;
    while (count > 0 && (d = qd::to_upper(*str2) - qd::to_upper(*str1)) == 0 && *str1) {
        str1++;
        str2++;
        count--;
    }
    return d;
}


}; // namespace qd
//////////////////////////////////////////////////////////////////////////


// clang-format off
template<class TString>
inline const char* CC(const TString& str) { return str.c_str(); }
inline const char* CC(const qtd::string_view& pStr) { return pStr.data(); }
inline const char* CC(const char* pStr) { return pStr; }
inline const wchar_t* CC(const wchar_t* pStr) { return pStr; }
inline const wchar_t* CW(const wchar_t* pStr) { return pStr; }

// clang-format on
