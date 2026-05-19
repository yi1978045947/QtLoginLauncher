#include "StdAfx.h"
#include "CommonFun.h"

#define CP_936 936

void PrintDebugString(const char *szFormat, ... )
{
#ifdef _DEBUG
	char szDebugContent[1024] = {0};
	va_list args;
	va_start(args, szFormat);
	_vsnprintf(szDebugContent, 1023, szFormat, args);
	va_end(args);
	::OutputDebugStringA(szDebugContent);
#endif
}

string IntToStr(int n)
{
	char buff[32];
	sprintf(buff, "%d", n);
	return buff;
}

string GbkToUtf8(const string& str)
{
	if (str.length() == 0)
		return "";
	int len = ::MultiByteToWideChar(CP_936, 0, str.c_str(), -1, NULL, 0);
	if (len <= 0)
		return "";
	WCHAR* wch = new WCHAR[len];
	::MultiByteToWideChar(CP_936, 0, str.c_str(), -1, wch, len);

	len = ::WideCharToMultiByte(CP_UTF8, 0, wch, -1, NULL, 0, NULL, NULL);
	
	if (len <= 0)
	{
		delete [] wch;
		return "";
	}
	CHAR* ch = new CHAR[len];
	::WideCharToMultiByte(CP_UTF8, 0, wch, -1, ch, len, NULL, NULL);
	string ret = ch;
	delete [] ch;
	delete [] wch;
	return ret;
}

string Utf8ToGbk(const string& str)
{
	if (str.length() == 0)
		return "";
	int len = ::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
	if (len <= 0)
		return "";
	WCHAR* wch = new WCHAR[len];
	::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, wch, len);

	len = ::WideCharToMultiByte(CP_936, 0, wch, -1, NULL, 0, NULL, NULL);
	if (len <= 0)
	{
		delete [] wch;
		return "";
	}
	CHAR* ch = new CHAR[len];
	::WideCharToMultiByte(CP_936, 0, wch, -1, ch, len, NULL, NULL);
	string ret = ch;
	delete [] ch;
	delete [] wch;
	return ret;
}
