#ifndef _SAPCLIENT_STRINGHELPER_H_
#define _SAPCLIENT_STRINGHELPER_H_

#include <vector>
#include <sstream>
#include <algorithm>
#include <locale.h>
#include <assert.h>
#include <locale>
#include "PlatformMacros.h"
#include "util.h"
#if defined(_MSC_VER)
#pragma warning(disable:4996)
#include "Platform-Win32.h"
#include <objbase.h>
#else 
#include "Platform-Unix.h"
#endif

#if defined(C11_SUPPORT)
#include <codecvt>
#endif

using std::vector;
using std::string;
using std::wstring;

class StringHelper
{
	StringHelper(const StringHelper&);
	StringHelper& operator=(const StringHelper&);
private:
	StringHelper() { ; }
	~StringHelper() { ; }
private:
	// 用于gcc下模板特化的trick
	template<typename T>
	struct DummyIdentity
	{
		typedef T type;
	};
public:
	static std::string	MakeLower(const std::string& in_str)
	{
		string instr = in_str;
		std::transform( instr.begin(), instr.end(), instr.begin(),tolower);
		return instr;
	}
	static std::wstring	MakeLower(const std::wstring& in_str)
	{
		wstring instr = in_str;
		std::transform( instr.begin(), instr.end(), instr.begin(),tolower);
		return instr;
	}
	static std::string	MakeUpper(const std::string& in_str)
	{
		string instr = in_str;
		std::transform( instr.begin(), instr.end(), instr.begin(),toupper);
		return instr;
	}
	static std::wstring	MakeUpper(const std::wstring& in_str)
	{
		wstring instr = in_str;
		std::transform( instr.begin(), instr.end(), instr.begin(),toupper);
		return instr;
	}
	
	//////////////////////////////////////////////////////////////////////////

	static wstring ConvertInt(unsigned int v,const wstring& fmt)
	{
		static const size_t digit_size = 100;
		wchar_t instr[digit_size+1] = { 0 };
		swprintf(instr,digit_size,fmt.c_str(),v);
		return instr;
	}

	static std::string	ConvertFromUInt(unsigned int in_int)
	{
		static const size_t digit_size = 100;
		char instr[digit_size+1] = { 0 };
		sprintf(instr,"%u",in_int);
		return instr;
	}
	static std::wstring	ConvertFromUInt64W(unsigned __int64 in_int)
	{
		static const size_t digit_size = 100;
		wchar_t instr[digit_size+1] = { 0 };
		swprintf(instr,digit_size,L"%llu",in_int);
		return instr;
	}
	static std::string	ConvertFromInt(int in_int)
	{
		static const size_t digit_size = 100;
		char instr[digit_size+1] = { 0 };
		sprintf(instr,"%d",in_int);
		return instr;
	}
	static std::wstring	ConvertFromUIntW(unsigned int in_int)
	{
		static const size_t digit_size = 100;
		wchar_t instr[digit_size+1] = { 0 };
		swprintf(instr,digit_size,L"%u",in_int);
		return instr;
	}
	static std::wstring	ConvertFromIntW(int in_int)
	{
		static const size_t digit_size = 100;
		wchar_t instr[digit_size+1] = { 0 };
		swprintf(instr,digit_size,L"%d",in_int);
		return instr;
	}
	
	static int	ConvertToInt(std::string in_str) { return atoi( in_str.c_str());	}
	static unsigned int	ConvertToUInt(std::string in_str,int radix = 10) { char* pstr = NULL;unsigned int ret = strtoul(in_str.c_str(),&pstr,radix);return ret;	}
	static unsigned int	ConvertToUInt(std::wstring in_str,int radix = 10) { wchar_t* pstr = NULL;unsigned int ret = wcstoul(in_str.c_str(),&pstr,radix);return ret; }

#ifdef PLATFORM_WINDOWS
	static int	ConvertToInt(std::wstring in_str) { return atoi( UnicodeToANSI(in_str).c_str()); }
	static double ConvertToDouble(std::wstring double_str) { return atof(UnicodeToANSI(double_str).c_str()); }
#endif

	static std::wstring HexToString(const std::string& strHex)
	{
		const unsigned char *pbyHex = (const unsigned char *)strHex.c_str();
		return HexToString(pbyHex, strHex.length());
	}
	static std::wstring HexToString(const unsigned char* pbyHex,size_t nLen)
	{
		if(NULL == pbyHex || 0 == nLen) return L"";
		wchar_t szTemp[16] = { 0 };

		wstring str;

		while (nLen--)
		{
			swprintf(szTemp,16, L"%02x", *pbyHex++);
			str += szTemp;
		}

		return str;
	}
	static std::string	StringToHex(std::wstring strString)
	{
		size_t nLen = strString.length();
		if(0 == nLen) return "";
		if((nLen%2) != 0) return "";

		string strHex;
		wchar_t *pbyBuffer = (wchar_t *)strString.c_str();
		unsigned char  byValue = 0;

		for (size_t i = 0; i < nLen - 1; i = i + 2)
		{
			if (pbyBuffer[i] >= 48  && pbyBuffer[i] <= 57)
			{
				byValue = pbyBuffer[i] - '0';
			}
			else if(pbyBuffer[i] >= 65 && pbyBuffer[i] <= 70)
			{
				byValue = pbyBuffer[i] - 55;
			}
			else if(pbyBuffer[i] >= 97 && pbyBuffer[i] <= 102)
			{
				byValue = pbyBuffer[i] - 87;
			}
			else
			{
				return "";
			}

			if (pbyBuffer[i + 1] >= 48  && pbyBuffer[i + 1] <= 57)
			{
				byValue = byValue * 16 + pbyBuffer[i + 1] - '0';
			}
			else if(pbyBuffer[i + 1] >= 65 && pbyBuffer[i + 1] <= 70)
			{
				byValue = byValue * 16 + pbyBuffer[i + 1] - '0' - 7;
			}
			else if(pbyBuffer[i + 1] >= 97 && pbyBuffer[i + 1] <= 102)
			{
				byValue = byValue * 16 + pbyBuffer[i + 1] - 87;
			}
			else
			{
				return "";
			}

			strHex.append(1, (char)byValue);
		}

		return strHex;
	}
	//////////////////////////////////////////////////////////////////////////

	static std::wstring	Utf8ToUnicode(const std::string& strUtf8)
	{
		wstring wstrUnicode = L"";
		size_t len = strUtf8.size();
		for (size_t i = 0; i < len; ++i)
		{
			unsigned int c = (unsigned int)(strUtf8[i]) & 0x000000ff;
			if (c >= 0x00 && c < 0xC0)
			{
				wstrUnicode.push_back((wchar_t)(c));
			}
			else if (c >= 0xc0 && c < 0xe0)
			{
				EASSERT(i + 1 < len,L"");
				unsigned int c1 = (unsigned int)(strUtf8[i+1]) & 0x000000ff;
				EASSERT(c1 >= 0x80 && c1 < 0xc0,L"");
				wstrUnicode.push_back((wchar_t)(((c & 0x1f) << 6) | (c1 & 0x3f)));
				i += 1;
			}
			else if (c >= 0xe0 && c < 0xf0)
			{
				EASSERT(i + 2 < len, L"");
				unsigned int c1 = (unsigned int)(strUtf8[i + 1]) & 0x000000ff;
				unsigned int c2 = (unsigned int)(strUtf8[i + 2]) & 0x000000ff;
				EASSERT(c1 >= 0x80 && c1 < 0xc0,L"");
				EASSERT(c2 >= 0x80 && c2 < 0xc0,L"");
				wstrUnicode.push_back((wchar_t)(((c & 0x0f) << 12) | ((c1 & 0x3f) << 6) | (c2 & 0x3f)));
				i += 2;
			}
			else
			{
				EASSERT(i + 3 < len, L"");
				unsigned int c1 = (unsigned int)(strUtf8[i + 1]) & 0x000000ff;
				unsigned int c2 = (unsigned int)(strUtf8[i + 2]) & 0x000000ff;
				unsigned int c3 = (unsigned int)(strUtf8[i + 3]) & 0x000000ff;
				EASSERT(c1 >= 0x80 && c1 < 0xc0,L"");
				EASSERT(c2 >= 0x80 && c2 < 0xc0,L"");
				EASSERT(c3 >= 0x80 && c3 < 0xc0,L"");
				wstrUnicode.push_back((wchar_t)(((c & 0x07) << 18) | ((c1 & 0x3f) << 12) | ((c2 & 0x3f) << 6) | (c3 & 0x3f)));
				i += 3;
			}
		}
#if defined(PLATFORM_WINDOWS)
		{
			int len = ::MultiByteToWideChar(CP_UTF8, 0, strUtf8.c_str(), -1, NULL, 0);
			if (len == 0) return L"";

			wchar_t* punicode = new wchar_t[len];
			::MultiByteToWideChar(CP_UTF8, 0, strUtf8.c_str(), -1, punicode, len);

			wstring ans = punicode;
			delete[] punicode;
		//	EASSERT(ans == wstrUnicode, L"");
		}
#endif
		return wstrUnicode;
	}
	static std::string	UnicodeToUtf8(const std::wstring& strUnicode)
	{
		string ret = "";
		size_t len = strUnicode.size();
		for(size_t i = 0;i < len;++i)
		{
			wchar_t c = strUnicode[i];
			if(c < 0x80)
			{
				ret.push_back(c&0x7f);
			}
			else if(c < 0x0800)
			{
				ret.push_back((c>>6)&0x1f | 0xc0);
				ret.push_back(c&0x3f | 0x80);
			}
			else if(c < 0x010000)
			{
				ret.push_back((c>>12)&0x0f | 0xE0);
				ret.push_back((c>>6)&0x3f | 0x80);
				ret.push_back(c&0x3f | 0x80);
			}
			else
			{
#if !defined(PLATFORM_WINDOWS)
				ret.push_back((c>>18)&0x7 | 0xF0);
				ret.push_back((c>>12)&0x3f | 0x80);
				ret.push_back((c>>6)&0x3f | 0x80);
				ret.push_back(c&0x3f | 0x80);
#endif
			}
		}
#if defined(PLATFORM_WINDOWS)
		{
			int len = ::WideCharToMultiByte(CP_UTF8, 0, strUnicode.c_str(), -1, NULL, 0, NULL, NULL);   
			if (len == 0) return "";   

			char* putf8 = new char[len];
			::WideCharToMultiByte(CP_UTF8, 0, strUnicode.c_str(), -1, putf8, len, NULL, NULL);

			string strUtf8 = putf8;
			delete[] putf8;
			return strUtf8;
			EASSERT(ret == strUtf8,"");
		}
#endif
		return ret;
	}

#ifdef PLATFORM_WINDOWS
	static std::wstring	ANSIToUnicode(const std::string& strAnsi)
	{
#if defined(C11_SUPPORT)
		std::wstring_convert< std::codecvt<wchar_t,char,std::mbstate_t> > cvt_ansi(new std::codecvt<wchar_t,char,std::mbstate_t>("CHS")); // GBK<->Unicode转换器
		return cvt_ansi.from_bytes(strAnsi);
#else
		int len = ::MultiByteToWideChar(CP_936, 0, strAnsi.c_str(), -1, NULL, 0);
		if (len == 0) return L"";
		wchar_t* punicode = new wchar_t[len];
		::MultiByteToWideChar(CP_936, 0, strAnsi.c_str(), -1, punicode, len);
		wstring wstrUnicode = punicode;
		delete[] punicode;
		return wstrUnicode;
#endif
	}

	static std::string	UnicodeToANSI(const std::wstring& strUnicode)
	{
#if defined(C11_SUPPORT)
		std::wstring_convert< std::codecvt<wchar_t,char,std::mbstate_t> > cvt_ansi(new std::codecvt<wchar_t,char,std::mbstate_t>("CHS")); // GBK<->Unicode转换器
		return cvt_ansi.to_bytes(strUnicode);
#else
		int len = ::WideCharToMultiByte(CP_936, 0, strUnicode.c_str(), -1, NULL, 0, NULL, NULL);
		if (len == 0) return "";
		char* putf8 = new char[len];
		::WideCharToMultiByte(CP_936, 0, strUnicode.c_str(), -1, putf8, len, NULL, NULL);
		string strAnsi = putf8;
		delete[] putf8;
		return strAnsi;
#endif
	}
#endif

#ifdef PLATFORM_WINDOWS
	static std::string  ANSIToUtf8(const std::string& strAnsi)
	{
		wstring wstr = ANSIToUnicode(strAnsi);
		return UnicodeToUtf8(wstr);
	}
	static std::string  Utf8ToANSI(const std::string& strUtf8)
	{
		wstring wstr = Utf8ToUnicode(strUtf8);
		return UnicodeToANSI(wstr);
	}
#endif

	static std::string Format(const char* format, ...)
	{
		va_list args;
		va_start(args, format);

		static const size_t buff_size = 4096;
		char buffer[buff_size+1] = {0};
		vsnprintf(buffer,buff_size,format,args);
		va_end(args);
		return buffer;
	}

	static std::wstring Format(const wchar_t* format, ...)
	{
		va_list args;
		va_start(args, format);

		static const size_t buff_size = 4096;
		wchar_t buffer[buff_size] = {0};
		vswprintf(buffer,buff_size,format,args);

		va_end(args);

		return buffer;
	}


	//////////////////////////////////////////////////////////////////////////

	static std::string	Reverse(const std::string& src_string)
	{
		string inner_src = src_string;
		std::reverse( inner_src.begin(), inner_src.end());
		return inner_src;
	}
	static std::wstring	Reverse(const std::wstring& src_string)
	{
		wstring inner_src = src_string;
		std::reverse( inner_src.begin(), inner_src.end());
		return inner_src;
	}
	static bool	IsNumberic(const std::string& num) { return num.find_first_not_of("0123456789.") == string::npos; }
	static bool	IsNumberic(const std::wstring& num) { return num.find_first_not_of(L"0123456789.") == wstring::npos; }
	static bool IsSpace(const std::string& str)
	{
		if(str.empty()) return true;
		std::string ret = TrimAll(str);
		return (0 == ret.size());
	}
	static bool IsSpace(const std::wstring& str)
	{
		if(str.empty()) return true;
		std::wstring ret = TrimAll(str);
		return (0 == ret.size());
	}
	static std::string	TrimAll( std::string src_str) { return TrimAll(src_str, " \r\n\t"); }
	static std::wstring	TrimAll( std::wstring src_str) { return TrimAll(src_str, L" \r\n\t"); }

	static std::string TrimAll(const std::string& src,const string& delim)
	{
		if(src.empty()) return src;
		std::string ret = src;
		ret.erase(0,ret.find_first_not_of(delim));
		ret.erase(ret.find_last_not_of(delim) + 1);
		return ret;
	}
	static std::wstring	TrimAll(const std::wstring& src,const wstring& delim)
	{
		if (src.empty()) return src;
		wstring ret = src;
		ret.erase(0,ret.find_first_not_of(delim));
		ret.erase(ret.find_last_not_of(delim) + 1);
		return ret;
	}

	// srcStr为源字符串，将对它里面的子串进行替换
	// replacedStr为替换字符串
	// replacingStr为将要替换的字符串
	// 函数返回替换后的字符串
	// 比如，如果需要将abcedef中的e替换为m，则用法如下
	// Replace("abcedef", "e", "m");
	static std::string	Replace(const std::string& strSrc,const std::string &strReplaced,const std::string &strReplacing)
	{
		string strResult = strSrc;

		string::size_type pos = 0;
		string::size_type srclen = strReplaced.size();
		string::size_type dstlen = strReplacing.size();

		while( (pos = strResult.find(strReplaced, pos)) != string::npos )
		{
			strResult.replace( pos, srclen, strReplacing );
			pos += dstlen;
		}

		return strResult;
	}
	static std::wstring	Replace(const std::wstring& strSrc,const std::wstring &strReplaced,const std::wstring &strReplacing)
	{
		wstring strResult = strSrc;

		wstring::size_type pos = 0;
		wstring::size_type srclen = strReplaced.size();
		wstring::size_type dstlen = strReplacing.size();

		while( (pos = strResult.find(strReplaced, pos)) != wstring::npos )
		{
			strResult.replace( pos, srclen, strReplacing );
			pos += dstlen;
		}

		return strResult;
	}

	template<typename T> 
	static T FromString(const std::string& s)
	{
		return FromString(s,DummyIdentity<T>());
	}

	template<typename T> 
	static T FromString(const std::wstring& s)
	{
		return FromString(s,DummyIdentity<T>());
	}

private:
	template<typename T>
	static T FromString(const std::string& s,DummyIdentity<T>)
	{
		if(0 == s.size()) return T();
		std::istringstream is(s);
		T t;
		is >> t;
		is.str("");
		return t;
	}
private:
	template<typename T> 
	static T FromString(const std::wstring& s,DummyIdentity<T>)
	{
		if(0 == s.size()) return T();
		std::wistringstream is(s);
		T t;
		is >> t;
		is.str(L"");
		return t;
	}
public:
	static std::string FromString(const std::string& s,DummyIdentity<std::string>) { return s; }
	static std::wstring FromString(const std::string& s,DummyIdentity<std::wstring>) { return Utf8ToUnicode(s); }
	static std::string FromString(const std::wstring& s,DummyIdentity<std::string>) { return UnicodeToUtf8(s); }
	static std::wstring FromString(const std::wstring& s,DummyIdentity<std::wstring>) { return s; }

public:
	template<typename T> static std::string ToString(const T& t) { return ToString(t,DummyIdentity<T>()); }
	template<typename T> static std::wstring ToStringW(const T& t) { return ToStringW(t,DummyIdentity<T>()); }
//
#ifdef PLATFORM_WINDOWS
	static std::wstring ToStringW(const std::string& s,DummyIdentity<std::string>) { return ANSIToUnicode(s); }
#endif
	static std::string ToString(const std::string& s,DummyIdentity<std::string>) { return s; }
	static std::wstring ToStringW(const std::wstring& s,DummyIdentity<std::wstring>) { return s; }

private:
	template<typename T> static std::string ToString(const T& t,DummyIdentity<T>) { std::ostringstream s;s << t;return s.str(); }
	template<typename T> static std::wstring ToStringW(const T& t,DummyIdentity<T>) { std::wostringstream s;s << t;return s.str(); }
public:

	static wstring Merge(const vector<wstring>& vec,const wchar_t* delim)
	{
		if(NULL == delim) return L"";
		wstring ret = L"";
		for(size_t i = 0;i < vec.size();++i)
		{
			if(0 != i) ret += delim;
			ret += vec[i];
		}
		return ret;
	}

	static string Merge(const vector<string>& vec,const char* delim)
	{
		if(NULL == delim) return "";
		string ret = "";
		for(size_t i = 0;i < vec.size();++i)
		{
			if(0 != i) ret += delim;
			ret += vec[i];
		}
		return ret;
	}

	// 分割是delim中的一个字符,连续的delim中的一个字符会被当成一个,头尾分隔符会被忽略
	static void Split(const char* str,const char* delim, std::vector<std::string> &outputVec)
	{
		if (NULL == str || NULL == delim) return;
		Split<std::string>(str, delim, outputVec);
	}

	static void Split(const wchar_t* str, const wchar_t* delim, std::vector<std::wstring> &outputVec)
	{
		if (NULL == str || NULL == delim) return;
		Split<std::wstring>(str, delim, outputVec);
	}

	template<class T>
	static void Split(const T& str,const T& delim, std::vector<T> &outputVec)
	{
		T from = str;
		size_t prev = 0, pos = from.find_first_of(delim);
		for (; pos != T::npos;)
		{
			if (pos != prev)
			{
				T s = from.substr(prev, pos - prev);
				outputVec.push_back(s);
			}
			prev = pos + 1;
			pos = from.find_first_of(delim, prev);
		}
		if (prev != T::npos && prev != str.size())
		{
			outputVec.push_back(from.substr(prev));
		}
	}

	// 分割标识是字符串
	static void Split2(const char* str,const char* delim, std::vector<std::string> &outputVec)
	{
		if (NULL == str || NULL == delim) return;
		size_t len = strlen(str);
		char* szBuffer = new char[len+1];
		memset(szBuffer,0,sizeof(char)*(len+1));
		strncpy(szBuffer,str,len);

		char* p = szBuffer,*token = strstr(szBuffer,delim);

		while(token != NULL)
		{
			*token = 0;
			outputVec.push_back(p);
			p = token + strlen(delim);
			token = strstr(p,delim);
		}
		outputVec.push_back(p);
		delete[] szBuffer;
	}
	static void Split2(const wchar_t* str,const wchar_t* delim, std::vector<std::wstring> &outputVec)
	{
		if (NULL == str || NULL == delim) return;
		size_t len = wcslen(str);
		wchar_t* szBuffer = new wchar_t[len+1];
		memset(szBuffer,0,sizeof(wchar_t)*(len+1));
		wcsncpy(szBuffer,str,len);
		wchar_t* p = szBuffer,*token = wcsstr(szBuffer,delim);

		while(token != NULL)
		{
			*token = 0;
			outputVec.push_back(p);
			p = token + wcslen(delim);
			token = wcsstr(p,delim);
		}
		outputVec.push_back(p);
		delete [] szBuffer;
	}

	// 分割是delim中的一个字符,连续的delim中的一个字符会被当成多个
	static void Split3(const char* str,const char* delim, std::vector<std::string> &outputVec)
	{
		if (NULL == str || NULL == delim) return;
		string from = str;
		size_t prev = 0,pos = from.find_first_of(delim);
		for(;pos != string::npos;)
		{
			string s = from.substr(prev,pos-prev);
			outputVec.push_back(s);
			prev = pos + 1;
			pos = from.find_first_of(delim,prev);
		}
		outputVec.push_back(from.substr(prev));
	}
	static void Split3(const wchar_t* str,const wchar_t* delim, std::vector<std::wstring> &outputVec)
	{
		if (NULL == str || NULL == delim) return;
		wstring from = str;
		size_t prev = 0,pos = from.find_first_of(delim);
		for(;pos != wstring::npos;)
		{
			wstring s = from.substr(prev,pos-prev);
			outputVec.push_back(s);
			prev = pos + 1;
			pos = from.find_first_of(delim,prev);
		}
		outputVec.push_back(from.substr(prev));
	}

	static void Split(const wstring& str,const wstring& ldelim,const wstring& rdelim,std::vector<wstring>& inner,std::vector<wstring>& outter)
	{
		wstring wstrText = str;
		for(;0 != wstrText.size();)
		{
			const wchar_t* pLeft = wcsstr(wstrText.c_str(),ldelim.c_str());
			if(NULL == pLeft) break;
			size_t lpos = pLeft - wstrText.c_str();
			lpos += ldelim.size();

			const wchar_t* pRight = wcsstr(wstrText.c_str()+lpos,rdelim.c_str());
			if(NULL == pRight) break;
			size_t rpos = pRight - wstrText.c_str();

			wstring w = wstrText.substr(0,lpos-ldelim.size());
			outter.push_back(w);

			wstring s = wstrText.substr(lpos,rpos-lpos);
			inner.push_back(s);

			lpos -= ldelim.size();
			wstrText.erase(0,rpos+rdelim.size());
		}
		outter.push_back(wstrText);
	}

	static std::string CreateGUIDA()
	{
		char buffer[64] = { 0 };
#if defined(_MSC_VER)
		GUID guid;
		if (S_OK != ::CoCreateGuid(&guid)) return "";
		
		_snprintf_s(buffer, _countof(buffer), "%08X%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X", 
			guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
			guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
		return buffer;
#else
		// xx xx xx xx - xx xx - 4x xx - 8x xx - xx xx xx xx xx xx
		// xx xx xx xx - xx xx - 4x xx - 9x xx - xx xx xx xx xx xx
		// xx xx xx xx - xx xx - 4x xx - ax xx - xx xx xx xx xx xx
		// xx xx xx xx - xx xx - 4x xx - bx xx - xx xx xx xx xx xx
		srand((unsigned int)(time(NULL)));
		const char* c = "89ab";
		char *p = buffer;
		for(unsigned int i = 0;i < 16;++i)
		{
			int b = rand()%255;
			switch(i)
			{
			case 6:
				sprintf(p,"4%x",b%15);
				break;
			case 8:
				sprintf(p,"%c%x",c[rand()%strlen(c)],b%15);
				break;
			default:
				sprintf(p,"%02x",b);
				break;
			}
			p += 2;
		}
		*p = 0;
		return buffer;
#endif
	}
	static std::wstring CreateGUIDW() { return Utf8ToUnicode(CreateGUIDA()); }
};

#endif
