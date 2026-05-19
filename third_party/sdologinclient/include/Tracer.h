#ifndef _LIB_COMMON_TRACER_H_
#define _LIB_COMMON_TRACER_H_

#ifdef _WIN32

#include <Windows.h>
#include <tchar.h>
#include <string>

using std::string;

#define __LOGGER_NAME__  "root"

class Tracer
{
public:
	struct Level
	{
		enum { Off, Fault, Error, Warning, Info, Debug, Trace };
	};
	struct TraceMode
	{
		enum { MiniTrace, AllTrace };
	};
public:
	Tracer(LPCSTR file, LPCSTR function, int line);
	~Tracer();
public:
	static bool    Initial(HMODULE hModule, int level, int traceMode);
	static bool    InitialSdologinentry(HMODULE hModule, int level, int traceMode, const wchar_t* logPath);
	static int     ParseLevel(LPCSTR level);
	static int     ParseLevel(LPCWSTR level);
	static LPCSTR  LevelToStringA(int level);
	static LPCWSTR LevelToStringW(int level);
	static UINT    GetTimestamp();
	static std::string  GetSystemDateA();
	static std::wstring GetSystemDateW();
	static void    MarkSendLog();

	static bool		GetLogLevel(int& level, int& traceMode);
	static bool		SetLogLevel(int level, int traceMode);
	static bool		SetLogLevelSdologinentry(int level, int traceMode, const wchar_t* logPath);

	static void Trace(LPCSTR file, LPCSTR function, int line, LPCSTR logger, int level, LPCSTR format, ...);
	static void Trace(LPCSTR file, LPCSTR function, int line, LPCSTR logger, int level, LPCWSTR format, ...);
private:
	static void GetLogFileName(std::string& fileName, std::string& fileNameImp);
	static void GetLogFileNameSdologinentry(std::string& fileName, std::string& fileNameImp, const wchar_t* logPath);
	static void GetMarkFileName();
private:
	static HMODULE m_hModule;
	static FILE* m_pLogFile;
	static FILE* m_pLogFileImp;
	static int     m_Level;
	static int     m_TraceMode;
	static string	m_strMarkFileName;
	LPCSTR         m_TraceFile;
	LPCSTR         m_TraceFunction;
	const int      m_TraceLine;
	UINT           m_Timestamp;

};

#ifndef TRACE_OFF

#undef TRACEF
#define TRACEF(...) Tracer::Trace(__FILE__, __FUNCTION__, __LINE__, __LOGGER_NAME__, Tracer::Level::Fault, __VA_ARGS__)

#undef TRACEE
#define TRACEE(...) Tracer::Trace(__FILE__, __FUNCTION__, __LINE__, __LOGGER_NAME__, Tracer::Level::Error, __VA_ARGS__)

#undef TRACEW
#define TRACEW(...) Tracer::Trace(__FILE__, __FUNCTION__, __LINE__, __LOGGER_NAME__, Tracer::Level::Warning, __VA_ARGS__)

#undef TRACEI
#define TRACEI(...) Tracer::Trace(__FILE__, __FUNCTION__, __LINE__, __LOGGER_NAME__, Tracer::Level::Info, __VA_ARGS__)

#undef TRACED
#define TRACED(...) Tracer::Trace(__FILE__, __FUNCTION__, __LINE__, __LOGGER_NAME__, Tracer::Level::Debug, __VA_ARGS__)

#undef  TRACET
#define TRACET() Tracer __Tracer(__FILE__,__FUNCTION__,__LINE__)

#else

#undef TRACEF
#define TRACEF 

#undef TRACEE
#define TRACEE 

#undef TRACEW
#define TRACEW 

#undef TRACEI
#define TRACEI 

#undef TRACED
#define TRACED 

#undef  TRACET
#define TRACET()

#endif

#else

#undef TRACEF
#define TRACEF 

#undef TRACEE
#define TRACEE 

#undef TRACEW
#define TRACEW 

#undef TRACEI
#define TRACEI 

#undef TRACED
#define TRACED 

#undef  TRACET
#define TRACET()

#endif

#endif
