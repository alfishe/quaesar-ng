#pragma once
#include <EABase/config/eacompiler.h>


#if defined(EA_COMPILER_GNUC)
#define gcc_only_template template
#else
#define gcc_only_template
#endif  // __GNUC__


#if defined(EA_COMPILER_GNUC) || (defined(EA_COMPILER_CLANG) || defined(__clang__))  // defined(__GNUC__)
#define gcc_template template
#else
#define gcc_template
#endif  // __GNUC__
