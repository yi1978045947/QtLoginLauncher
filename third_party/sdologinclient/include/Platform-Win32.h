#ifndef _LIB_COMMON_PLATFORMWIN32_H_
#define _LIB_COMMON_PLATFORMWIN32_H_

#include <xstring>
#include <Windows.h>

#ifndef UNICODE
#define CreateGUID CreateGUIDW
#else
#define CreateGUID CreateGUIDA
#endif

#define CP_936 936

#endif
