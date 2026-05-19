#pragma once

#include <tchar.h>
#include <windows.h>
#include <string.h>
#include <assert.h>

#if defined(_WIN32) || defined(_WIN64)
#define SNDACALL			__stdcall
#else
#define SNDACALL
#endif

#if !defined(interface)
#define interface	struct
#endif

#if !defined(PURE)
#define PURE		= 0
#endif

#define SNDACALLBACK		SNDACALL
#define SNDAMETHOD(Type)	virtual Type SNDACALL
#define SNDAAPI(Type)		extern "C" Type SNDACALL

#ifdef _USE_LINUX
#define SNDADLLAPI 
#else
#define SNDADLLAPI extern "C" _declspec(dllexport)
#endif

#ifdef _USE_LINUX
#define CLASSEXPORT 
#else
#define CLASSEXPORT _declspec(dllexport)
#endif

#ifndef PARRAYSIZE
#define PARRAYSIZE(array) ((sizeof(array)/sizeof(array[0])))
#endif

#ifndef DELETEP
#define DELETEP(ptr) \
	if(NULL != (ptr)) \
{ \
	delete (ptr); \
	(ptr) = NULL; \
}
#endif

#ifndef STRNCPY
#define STRNCPY(dstPtr,srcPtr) \
	memset(dstPtr, 0x0, sizeof(dstPtr)/sizeof(wchar_t)); \
	wcsncpy(dstPtr, srcPtr, sizeof(dstPtr)/sizeof(wchar_t)); 
#endif

#ifndef ZEROMEM
#define ZEROMEM(ptr) \
	memset(ptr, 0x0,sizeof(ptr));
#endif

typedef signed __int64 int64;
typedef signed int int32;
typedef signed short int16;
typedef signed char int8;

typedef unsigned __int64 uint64;
typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;
typedef unsigned char byte;
typedef unsigned long       ulong;


#if defined(_WIN64)
typedef uint64 uintptr;
typedef uint32 EventValue;
typedef uint32 MessageValue;
typedef uint32 ServiceValue;
typedef uint32 uin;
typedef uint32 tid;
typedef uint64 param;
#else
typedef uint32 uintptr;
typedef uint32 EventValue;
typedef uint32 MessageValue;
typedef uint32 ServiceValue;
typedef uint32 uin;
typedef uint32 tid;
typedef uint32 param;
#endif

typedef uint32 Cookie;

#ifndef NULL
#define NULL 0
#endif


#ifndef NULL
#define NULL 0
#endif

#ifndef IN
#define IN	
#endif

#ifndef OUT
#define OUT
#endif

#ifndef ASSERT
#define ASSERT assert
#endif


//////////////////////////////////////////////////////////////////////////

interface ISBYTE
{
	SNDAMETHOD(HRESULT) Release() PURE;
	SNDAMETHOD(HRESULT) PutValue(BYTE byValue) PURE;
	SNDAMETHOD(HRESULT) GetValue(BYTE* pbyValue) PURE;
};

interface ISDWORD
{
	SNDAMETHOD(HRESULT) Release() PURE;
	SNDAMETHOD(HRESULT) GetValue(DWORD *pdwValue) PURE;
	SNDAMETHOD(HRESULT) PutValue(DWORD dwValue) PURE;
};

interface ITicketMgr
{
	SNDAMETHOD(HRESULT) StoreTicket(const wchar_t* szPt,const wchar_t* szTicket,const wchar_t* szAppid) PURE;
	SNDAMETHOD(HRESULT) GetTicket(const wchar_t* szPt,wchar_t szTicket[], UINT uTicketLen,const wchar_t* szAppid) PURE;
	SNDAMETHOD(HRESULT) SetAutoLogin(const wchar_t* szPt,const wchar_t* szAppid,BOOL bAutoLogin) PURE;
	SNDAMETHOD(HRESULT) IsAutoLogin(const wchar_t* szPt,const wchar_t* szAppid,BOOL *bResult) PURE;
	SNDAMETHOD(HRESULT) StoreTicketBirth(const wchar_t* szPt,const wchar_t* szAppid,const wchar_t* szBirth) PURE;
	SNDAMETHOD(HRESULT) GetTicketBirth(const wchar_t* szPt,const wchar_t* szAppid,wchar_t szBirth[], UINT uBirthLen) PURE;
};

//////////////////////////////////////////////////////////////////////////

#define SAFESTORE_ID   1
#define ALGORITHM_ID   2
#define SBYTE_ID       3
#define SDWORD_ID      4
#define TICKETMGR_ID   5

#pragma comment(lib, "Crypt32.lib")

interface IBassgUnkown
{
	SNDAMETHOD(HRESULT) AddRef() PURE;
	SNDAMETHOD(HRESULT) Release() PURE;
	SNDAMETHOD(HRESULT) QueryInterface(VOID **lpObject, UINT nObjectId) PURE;
};

interface ISafeStore : public IBassgUnkown
{
	SNDAMETHOD(HRESULT) PushBack(ISBYTE* pbyValue) PURE;
	SNDAMETHOD(HRESULT) Insert(ISBYTE* pbyValue, ISDWORD* pdwPos) PURE;
	SNDAMETHOD(HRESULT) Empty(void) PURE;
	SNDAMETHOD(HRESULT) Erase(ISDWORD* pdwPos) PURE;
	SNDAMETHOD(HRESULT) Remove(ISDWORD* pdwPos,ISDWORD* pdwSize) PURE;
	SNDAMETHOD(HRESULT) GetData(BYTE* pszCipher, UINT &uCipherLen) PURE;
};

interface IAlgorithm : public IBassgUnkown
{
	SNDAMETHOD(HRESULT) RSA(BYTE* pszCipher, UINT &uCipherLen, CHAR szChallengeCode[], UINT uCodeLen) PURE;
};

//////////////////////////////////////////////////////////////////////////
// 以下三个函数如果是静态库，可以直接调用，如果为动态库为导出函数

HRESULT RSA_(BYTE *pszPlain, UINT uPlainLen, BYTE* pszCipher, UINT &uCipherLen, CHAR szChallengeCode[], UINT uCodeLen);
HRESULT RSAEx(BYTE *pszPlain, UINT uPlainLen, BYTE* pszCipher, UINT &uCipherLen, CHAR publicKey[], UINT uKeyLen);
BOOL PwdSimpleDecrypt(BYTE* pszCipher,UINT nCipherLen,BYTE* pszPlain,UINT& nPlainLen);
HRESULT CreateObject(VOID **pObject, UINT nObjectId);
int sdoa_encrypt(const char* data,size_t size,const char* key,char* result);

int DesEcbEnc(const char* key,const unsigned char *data, int insize, unsigned char *result);
int DesEcbDec(const char* key,const unsigned char *data, int insize, unsigned char *result);

size_t sdoa_3des_cbc_decrypt(const char *data,size_t insize,const char *key,char *result);

int AesEncEx(unsigned char abyKey[16],const unsigned char *pbyInBlk, int nInLen, unsigned char *pbyOutBlk);
int AesDecEx(unsigned char abyKey[16],const unsigned char *pbyInBlk, int nInLen, unsigned char *pbyOutBlk);

//////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
#define BASSG_DLL   _T("bassg.dll")
#else
#define BASSG_DLL	_T("bassg.dll")
#endif

inline VOID* CreateObject(UINT nObjectId, const TCHAR *pszDllPath = NULL)
{
	HMODULE hBassg = ::GetModuleHandle(BASSG_DLL);
	if(!hBassg)
	{
		TCHAR szPath[MAX_PATH] = {0};
		if (!pszDllPath)
		{
			::GetModuleFileName(NULL,szPath,MAX_PATH);
			*(_tcsrchr(szPath, '\\') + 1) = 0;
		}
		else
		{
			_tcsncpy_s(szPath,MAX_PATH,pszDllPath,sizeof(szPath)-1);
		}

		unsigned int cRemaining = (unsigned int)((sizeof(szPath)-_tcslen(szPath))-1);
		_tcsncat_s(szPath,MAX_PATH,BASSG_DLL, cRemaining);
		hBassg = ::LoadLibrary(szPath);
	}
	
	typedef HRESULT (CREATEOBJ)(VOID **pObject, UINT nObjectId);
	CREATEOBJ *pCreateObj = (CREATEOBJ*)::GetProcAddress(hBassg, "CreateObject");

	VOID *pObject = NULL;
	if(pCreateObj)
	{
		pCreateObj((VOID **)&pObject, nObjectId);
	}

	return pObject;
}
