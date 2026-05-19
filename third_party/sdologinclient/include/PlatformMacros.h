#ifndef _LIB_COMMON_PLATFORMMACROS_H_
#define _LIB_COMMON_PLATFORMMACROS_H_

#ifdef _WIN32
#define PLATFORM_WINDOWS
#else
#endif

#if (defined(__GNUC__) || _MSC_VER >= 1900)
#define C11_SUPPORT
#endif

#endif
