#pragma once

#include <Windows.h>
#include <wchar.h>
#include <shlwapi.h>
#include "LicenseManager.h"
#include "Tracer.h"
#include "PathHelper.h"

class CLogManager
{
	CLogManager(const CLogManager&);
	CLogManager& operator=(const CLogManager&);
public:
	CLogManager() { ; }
	~CLogManager() { ; }
public:
	static void InitialLog(HMODULE hModule,bool forceDebug = false)
	{
		wchar_t buffer[MAX_PATH] = { 0 },path[MAX_PATH*2] = { 0 };
		GetModuleFileNameW(hModule, buffer, _countof(buffer));
		swprintf_s(path, _countof(path), L"%s\\..\\", buffer);

		LicenseManager::Initial(path);
		bool bExpired = LicenseManager::GetInstance()->GetValue<bool>("Expiration");
		bool bGenerateLog = LicenseManager::GetInstance()->GetValue<bool>("GenerateLog");
		LicenseManager::Destroy();

		// 在没有license授权时，默认输出warning以上级别的log，同时log中不输出文件、类、函数等信息。
		int level = Tracer::Level::Warning;
		int traceMode = Tracer::TraceMode::MiniTrace;

		if(forceDebug)
		{
			level = Tracer::Level::Trace;
			traceMode = Tracer::TraceMode::AllTrace;
		}
		else if (bExpired == false && bGenerateLog == true)	// 未过期 && 生成日志
		{
			traceMode = Tracer::TraceMode::AllTrace;

			FILE* pFile = fopen(PathHelper::GetFullPath(hModule, "log.config").c_str(), "r");
			if(pFile != NULL)
			{
				char buffer[64] = {0};
				int len = fread(buffer, sizeof(char), _countof(buffer), pFile);
				if(len >= _countof(buffer))
				{
					len = _countof(buffer) - 1;
				}
				buffer[len] = 0;

				level = Tracer::ParseLevel(buffer);

				fclose(pFile);
			}
		}

		Tracer::Initial(hModule, level, traceMode);
	}
public:
	static void DeleteOldLogDir(const wchar_t* szPath)
	{
		if(NULL == szPath) return;
		if(!PathIsDirectoryW(szPath)) return;

		wchar_t wszFilePath[MAX_PATH+1] = { 0 };
		wcsncpy_s(wszFilePath,MAX_PATH,szPath,MAX_PATH);
		wcsncat_s(wszFilePath,MAX_PATH,L"\\*",2);

		WIN32_FIND_DATAW findFileData;
		HANDLE hListFile = FindFirstFileW(wszFilePath,&findFileData);

		if(INVALID_HANDLE_VALUE == hListFile) return;

		wchar_t wszFullPath[MAX_PATH+1] = {0};

		SYSTEMTIME st;
		GetLocalTime(&st);

		do 
		{
			if(wcsncmp(findFileData.cFileName,L".",1) == 0|| 0 == wcsncmp(findFileData.cFileName,L"..",2)) continue;

			wchar_t year[5] = { 0 },month[3] = { 0 },day[3] = { 0 };
			wcsncpy_s(year,5,findFileData.cFileName,4);
			wcsncpy_s(month,3,findFileData.cFileName+5,2);
			wcsncpy_s(day,3,findFileData.cFileName+8,2);

			int nYear = _wtoi(year),nMonth = _wtoi(month),nDay = _wtoi(day);
			swprintf_s(wszFullPath,MAX_PATH,L"%s\\%s",szPath,findFileData.cFileName);
			if ((nYear<st.wYear)||(nYear==st.wYear&&nMonth<st.wMonth)||(nYear==st.wYear&&nMonth==st.wMonth&&nDay<=st.wDay-3))
			{
				PathHelper::ForceRemoveDir(wszFullPath);
			}
		} while (FindNextFileW(hListFile,&findFileData));

		FindClose(hListFile);
	}
};