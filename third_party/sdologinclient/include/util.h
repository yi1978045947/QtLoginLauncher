#pragma once

#ifndef ASSERT
#include <assert.h>
#include "Tracer.h"
#define ASSERT assert
#endif

#ifndef _DEBUG
	#define ASSERT_RETURN(X) if(!(X)){TRACEE("ASSERT %s", #X); return;}
	#define ASSERT_RETURN_VALUE(X, RET) if(!(X)){TRACEE("ASSERT %s %d", #X, (int)RET); return RET;}
#else
	#define ASSERT_RETURN(X) if(!(X)){ ASSERT(0); return;}
	#define ASSERT_RETURN_VALUE(X, RET) if(!(X)){ ASSERT(0); return RET;}
#endif

#ifndef SAFE_DELETE
#define SAFE_DELETE(P) if(P){if(P != NULL){delete P;}P=NULL;}
#endif

#ifndef SAFE_CLOSE
#define SAFE_CLOSE(P) if(P){P->Close();P=NULL;}
#endif

#ifndef SAFE_FREE
	#define SAFE_FREE(P) if(P){free(P);P=NULL;}
#endif

#ifndef SAFE_DESTROY
#define SAFE_DESTROY(WND) if(::IsWindow(WND)){DestroyWindow(WND);WND=NULL;}
#endif

#ifndef SAFE_CLOSE_HANDLE
	#define SAFE_CLOSE_HANDLE(H) if(H){CloseHandle(H);H=NULL;}
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(P) if(P){P->Release();P=NULL;}
#endif

#ifdef _DEBUG
#define RASSERT(x,r) { if(!(x)) return r; }
#else
#define RASSERT(x,r) { if(!(x)) return r; }
#endif

#ifndef EASSERT
#ifdef _DEBUG
#ifdef _WIN64
// x64下不支持c/c++和汇编混编
#define EASSERT(x,r) { if(!(x)) { __debugbreak();return r; } }
#else
#define EASSERT(x,r) { if(!(x)) { __asm { int 3 };return r; } }
#endif
#define EASSERTV(x) { if(!(x)) { assert(0);return; } }
#else
#define EASSERT(x,r) { if(!(x)) return r; }
#define EASSERTV(x) { if(!(x)) { assert(0);return; } }
#endif
#endif

#define CALC_BMWIDTHBYTES(cx)	((((cx)*32+31)&(~31))>>3)
