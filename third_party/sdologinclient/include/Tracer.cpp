#include "Tracer.h"
#include "StringHelper.h"
#include <MMSystem.h>
#include <Shlwapi.h>
#include <time.h>
#include <stdio.h>

#pragma comment(lib, "Winmm.lib")
#pragma comment(lib, "shlwapi.lib")

HMODULE Tracer::m_hModule = NULL;
FILE* Tracer::m_pLogFile = NULL;
FILE* Tracer::m_pLogFileImp = NULL;
int     Tracer::m_Level = Tracer::Level::Off;
int     Tracer::m_TraceMode = Tracer::TraceMode::MiniTrace;
string	Tracer::m_strMarkFileName = "";

// #define TRACE_TO_OUTPUT

Tracer::Tracer(LPCSTR file, LPCSTR function, int line)
	: m_TraceFile(file)
	, m_TraceFunction(function)
	, m_TraceLine(line)
{
	m_Timestamp = GetTimestamp();

	Trace(file, function, line, "trace", Tracer::Level::Trace, "Entry function %s\n", function);
}

Tracer::~Tracer()
{
	UINT cost = GetTimestamp() - m_Timestamp;
	Trace(m_TraceFile, m_TraceFunction, m_TraceLine, "trace", Tracer::Level::Trace, "Exit function %s time cost %u\n", m_TraceFunction, cost);

	if (cost > 50)
	{
#ifdef _DEBUG
		Trace(m_TraceFile, m_TraceFunction, m_TraceLine, "trace", Tracer::Level::Warning, "Function %s time cost %u\n", m_TraceFunction, cost);
#else
		Trace(m_TraceFile, m_TraceFunction, m_TraceLine, "trace", Tracer::Level::Info, "Function %s time cost %u\n", m_TraceFunction, cost);
#endif
	}
	else if (cost > 20)
	{
		Trace(m_TraceFile, m_TraceFunction, m_TraceLine, "trace", Tracer::Level::Debug, "Function %s time cost %u\n", m_TraceFunction, cost);
	}
}

bool Tracer::Initial(HMODULE hModule, int level, int traceMode)
{
	m_hModule = hModule;
	GetMarkFileName();
	return SetLogLevel(level, traceMode);
}

bool Tracer::InitialSdologinentry(HMODULE hModule, int level, int traceMode, const wchar_t* logPath)
{
	m_hModule = hModule;
	GetMarkFileName();
	return SetLogLevelSdologinentry(level, traceMode, logPath);
}

bool Tracer::SetLogLevel(int level, int traceMode)
{
	m_Level = level;
	m_TraceMode = traceMode;

	if (level > Tracer::Level::Off)
	{
		std::string fileName;
		std::string fileNameImp;
		GetLogFileName(fileName, fileNameImp);

		if (traceMode != TraceMode::MiniTrace && m_pLogFile == NULL)
		{
			m_pLogFile = fopen(fileName.c_str(), "w");
			if (m_pLogFile == NULL)
			{
				return false;
			}
		}

		if (m_pLogFileImp == NULL)
		{
			m_pLogFileImp = fopen(fileNameImp.c_str(), "w");
			if (m_pLogFileImp == NULL)
			{
				return false;
			}
		}

		return true;
	}
	else
	{
		if (m_pLogFile)
		{
			fflush(m_pLogFile);
			if (fclose(m_pLogFile) != 0)
			{
				return false;
			}

			m_pLogFile = NULL;
		}

		if (m_pLogFileImp)
		{
			fflush(m_pLogFileImp);
			if (fclose(m_pLogFileImp) != 0)
			{
				return false;
			}

			m_pLogFileImp = NULL;
		}

		return true;
	}
	return true;
}

bool Tracer::SetLogLevelSdologinentry(int level, int traceMode, const wchar_t* logPath)
{
	m_Level = level;
	m_TraceMode = traceMode;

	if (level > Tracer::Level::Off)
	{
		std::string fileName;
		std::string fileNameImp;
		GetLogFileNameSdologinentry(fileName, fileNameImp, logPath);

		if (traceMode != TraceMode::MiniTrace && m_pLogFile == NULL)
		{
			m_pLogFile = fopen(fileName.c_str(), "w");
			if (m_pLogFile == NULL)
			{
				return false;
			}
		}

		if (m_pLogFileImp == NULL)
		{
			m_pLogFileImp = fopen(fileNameImp.c_str(), "w");
			if (m_pLogFileImp == NULL)
			{
				return false;
			}
		}

		return true;
	}
	else
	{
		if (m_pLogFile)
		{
			fflush(m_pLogFile);
			if (fclose(m_pLogFile) != 0)
			{
				return false;
			}

			m_pLogFile = NULL;
		}

		if (m_pLogFileImp)
		{
			fflush(m_pLogFileImp);
			if (fclose(m_pLogFileImp) != 0)
			{
				return false;
			}

			m_pLogFileImp = NULL;
		}

		return true;
	}
	return true;
}

bool Tracer::GetLogLevel(int& level, int& traceMode)
{
	level = m_Level;
	traceMode = m_TraceMode;
	return true;
}

int Tracer::ParseLevel(LPCSTR level)
{
	if (_stricmp(level, "trace") == 0)
	{
		return Level::Trace;
	}
	else if (_stricmp(level, "debug") == 0)
	{
		return Level::Debug;
	}
	else if (_stricmp(level, "info") == 0)
	{
		return Level::Info;
	}
	else if (_stricmp(level, "warning") == 0)
	{
		return Level::Warning;
	}
	else if (_stricmp(level, "error") == 0)
	{
		return Level::Error;
	}
	else if (_stricmp(level, "fault") == 0)
	{
		return Level::Fault;
	}
	else
	{
		return Level::Off;
	}
}

int Tracer::ParseLevel(LPCWSTR level)
{
	return ParseLevel(StringHelper::UnicodeToANSI(level).c_str());
}

LPCSTR Tracer::LevelToStringA(int level)
{
	switch (level)
	{
	case Level::Trace:
		return "Trace";
	case Level::Debug:
		return "Debug";
	case Level::Info:
		return "Info";
	case Level::Warning:
		return "Warning";
	case Level::Error:
		return "Error";
	case Level::Fault:
		return "Fault";
	default:
		return "Off";
	}
}

LPCWSTR Tracer::LevelToStringW(int level)
{
	switch (level)
	{
	case Level::Trace:
		return L"Trace";
	case Level::Debug:
		return L"Debug";
	case Level::Info:
		return L"Info";
	case Level::Warning:
		return L"Warning";
	case Level::Error:
		return L"Error";
	case Level::Fault:
		return L"Fault";
	default:
		return L"Off";
	}
}

UINT Tracer::GetTimestamp()
{
	return timeGetTime();
}


string Tracer::GetSystemDateA() {
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);

	char szDate[100] = { 0 };
	sprintf_s(szDate, 100, "%04ld-%02ld-%02ld %02ld:%02ld:%02ld %ld",
		sysTime.wYear, sysTime.wMonth, sysTime.wDay,
		sysTime.wHour, sysTime.wMinute, sysTime.wSecond,
		sysTime.wMilliseconds
	);
	return szDate;
}

//wstring Tracer::GetSystemDateW(){
//	SYSTEMTIME sysTime;
//	GetLocalTime(&sysTime);
//
//	TCHAR szDate[100] = { 0 };
//	swprintf_s(szDate, 100, L"%04ld-%02ld-%02ld %02ld:%02ld:%02ld %ld",
//		sysTime.wYear, sysTime.wMonth, sysTime.wDay,
//		sysTime.wHour, sysTime.wMinute, sysTime.wSecond,
//		sysTime.wMilliseconds
//		);
//	 return szDate;
//}

std::wstring Tracer::GetSystemDateW() {
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);

	wchar_t szDate[100] = { 0 }; // **明确使用 wchar_t**
	swprintf_s(szDate, 100, L"%04d-%02d-%02d %02d:%02d:%02d %03d",
		sysTime.wYear, sysTime.wMonth, sysTime.wDay,
		sysTime.wHour, sysTime.wMinute, sysTime.wSecond,
		sysTime.wMilliseconds);

	return szDate;
}

void Tracer::MarkSendLog()
{
	GetMarkFileName();
	HANDLE hFile = CreateFileA(m_strMarkFileName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hFile);
	}
	else
	{
		TRACEW(_T("Create mark file failed. errorcode[%d]"), GetLastError());
	}
}

void Tracer::Trace(LPCSTR file, LPCSTR function, int line, LPCSTR logger, int level, LPCSTR format, ...)
{
	if (level > m_Level)
	{
		return;
	}

	if (level <= Level::Error)
	{
		MarkSendLog();
	}

	va_list args;
	va_start(args, format);

	char buffer[1024] = { 0 };
	::vsnprintf_s(buffer, 1024, _countof(buffer) - 1, format, args);

	va_end(args);

	LPCSTR prefix = m_TraceMode == TraceMode::MiniTrace ? "" : function;

	char text[1024] = { 0 };
	if (strlen(buffer) > 0 && buffer[strlen(buffer) - 1] == '\n')
	{
		_snprintf_s(text, 1024, _countof(text) - 1, "%s TH: %d TS: %s %s %s", LevelToStringA(level), GetCurrentThreadId(), GetSystemDateA().c_str(), prefix, buffer);
	}
	else
	{
		_snprintf_s(text, 1024, _countof(text) - 1, "%s TH: %d TS: %s %s %s\n", LevelToStringA(level), GetCurrentThreadId(), GetSystemDateA().c_str(), prefix, buffer);
	}

	if (m_pLogFile)
	{
		fwrite(text, sizeof(char), strlen(text), m_pLogFile);
		fflush(m_pLogFile);
	}

	if (m_pLogFileImp && level <= Level::Warning)
	{
		fwrite(text, sizeof(char), strlen(text), m_pLogFileImp);
		fflush(m_pLogFileImp);
	}
}

void Tracer::Trace(LPCSTR file, LPCSTR function, int line, LPCSTR logger, int level, LPCWSTR format, ...)
{
	if (level > m_Level)
	{
		return;
	}

	if (level <= Level::Error)
	{
		MarkSendLog();
	}

	va_list args;
	va_start(args, format);

	WCHAR buffer[1024] = { 0 };
	::_vsnwprintf_s(buffer, 1024, _countof(buffer) - 1, format, args);

	va_end(args);

	std::wstring prefix = m_TraceMode == TraceMode::MiniTrace ? L"" : StringHelper::Utf8ToUnicode(function);

	WCHAR text[1024] = { 0 };
	if (wcslen(buffer) > 0 && buffer[wcslen(buffer) - 1] == '\n')
	{
		_snwprintf_s(text, 1024, _countof(text) - 1, L"%s TH: %d TS: %s %s %s", LevelToStringW(level), GetCurrentThreadId(), GetSystemDateW().c_str(), prefix.c_str(), buffer);
	}
	else
	{
		_snwprintf_s(text, 1024, _countof(text) - 1, L"%s TH: %d TS: %s %s %s\n", LevelToStringW(level), GetCurrentThreadId(), GetSystemDateW().c_str(), prefix.c_str(), buffer);
	}

	std::string str = StringHelper::UnicodeToANSI(text);

	if (m_pLogFile)
	{
		fwrite(str.c_str(), sizeof(char), str.size(), m_pLogFile);
		fflush(m_pLogFile);
	}

	if (m_pLogFileImp && level <= Level::Warning)
	{
		fwrite(str.c_str(), sizeof(char), str.size(), m_pLogFileImp);
		fflush(m_pLogFileImp);
	}
}

static void MakeDirectoryAvailable(LPCSTR path)
{
	DWORD flag = GetFileAttributesA(path);
	DWORD code = GetLastError();

	if ((flag == 0xFFFFFFFF && code == 2/*not exist*/) || !(flag & FILE_ATTRIBUTE_DIRECTORY))
	{
		::CreateDirectoryA(path, NULL);
	}
}

void Tracer::GetLogFileName(std::string& fileName, std::string& fileNameImp)
{
	char buffer[MAX_PATH] = { 0 };

	GetModuleFileNameA(m_hModule, buffer, _countof(buffer));
	std::string moduleFileName = PathFindFileNameA(buffer);
	std::string moduleFilePath = buffer;

	_snprintf_s(buffer, _countof(buffer), "%s/../logs", moduleFilePath.c_str());
	std::string logRoot = buffer;
	MakeDirectoryAvailable(logRoot.c_str());

	time_t timestamp = { 0 };
	time(&timestamp);
	tm tmTime = { 0 };
	localtime_s(&tmTime, &timestamp);

	strftime(buffer, _countof(buffer), "%Y-%m-%d", &tmTime);
	std::string currentTime = buffer;

	_snprintf_s(buffer, _countof(buffer), "%s/%s", logRoot.c_str(), currentTime.c_str());
	std::string logPath = buffer;
	MakeDirectoryAvailable(logPath.c_str());

	_snprintf_s(buffer, _countof(buffer), "%s/%s_%d.log", logPath.c_str(), moduleFileName.c_str(), GetCurrentProcessId());
	fileName = buffer;

	_snprintf_s(buffer, _countof(buffer), "%s/%s_%d_imp.log", logPath.c_str(), moduleFileName.c_str(), GetCurrentProcessId());
	fileNameImp = buffer;
}

void Tracer::GetLogFileNameSdologinentry(std::string& fileName, std::string& fileNameImp, const wchar_t* m_logPath)
{
	char buffer[MAX_PATH] = { 0 };

	GetModuleFileNameA(m_hModule, buffer, _countof(buffer));
	std::string moduleFileName = PathFindFileNameA(buffer);

	// **手动转换 `m_logPath` 到 `std::string`**
	std::string logRoot;
	if (m_logPath)
	{
		char logPathA[MAX_PATH] = { 0 };
		WideCharToMultiByte(CP_ACP, 0, m_logPath, -1, logPathA, MAX_PATH, NULL, NULL);
		logRoot = logPathA;
	}
	else
	{
		logRoot = "../sdologin/logs"; // **默认回退到 `sdologin/logs`**
	}

	MakeDirectoryAvailable(logRoot.c_str());

	time_t timestamp = { 0 };
	time(&timestamp);
	tm tmTime = { 0 };
	localtime_s(&tmTime, &timestamp);

	strftime(buffer, _countof(buffer), "%Y-%m-%d", &tmTime);
	std::string currentTime = buffer;

	_snprintf_s(buffer, _countof(buffer), "%s/%s", logRoot.c_str(), currentTime.c_str());
	std::string logPath = buffer;
	MakeDirectoryAvailable(logPath.c_str());

	_snprintf_s(buffer, _countof(buffer), "%s/%s_%d.log", logPath.c_str(), moduleFileName.c_str(), GetCurrentProcessId());
	fileName = buffer;

	_snprintf_s(buffer, _countof(buffer), "%s/%s_%d_imp.log", logPath.c_str(), moduleFileName.c_str(), GetCurrentProcessId());
	fileNameImp = buffer;
}

void Tracer::GetMarkFileName()
{
	if (m_strMarkFileName.empty())
	{
		char buffer[MAX_PATH] = { 0 };
		GetModuleFileNameA(m_hModule, buffer, _countof(buffer));

		m_strMarkFileName = buffer;
		m_strMarkFileName += "/../2048A269-AEA5-4c83-9F16-EB2B537286B4";
	}
}