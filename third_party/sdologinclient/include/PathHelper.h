#ifndef _LIB_COMMON_PATHHELPER_H_
#define _LIB_COMMON_PATHHELPER_H_

#include <Windows.h>
#include <string>
#include <vector>
#include <shlwapi.h>

using std::vector;
using std::string;
using std::wstring;

class PathHelper
{
public:
	static void Initial(HMODULE hModule) { m_hModule = hModule; }
	
	static string GetFullPath(LPCSTR path) { return GetFullPath(m_hModule, path); }
	static wstring GetFullPath(LPCWSTR path) { return GetFullPath(m_hModule, path); }
	
	static string GetFullPath(HMODULE hModule, LPCSTR path);
	static wstring GetFullPath(HMODULE hModule, LPCWSTR path);

	static wstring GetFileExtension(LPCWSTR fileName);
	static wstring GetBaseFileName(const wstring& wstrFileName);

	static wstring GetGamePath(HMODULE hModule);
	static string GetGamePath(HMODULE hModule, LPCSTR path);

	static string GetFullName(LPCSTR path,LPCSTR name)
	{
		string fullPath(path),fileName(name);
		if(0 == fullPath.size()) return "";
		if(fullPath[fullPath.size()-1] != '\\') fullPath += "\\";
		fullPath += fileName;
		return fullPath;
	}
	static wstring GetFullName(LPCWSTR path,LPCWSTR name)
	{
		wstring fullPath(path),fileName(name);
		if(0 == fullPath.size()) return L"";
		if(fullPath[fullPath.size()-1] != '\\') fullPath += L"\\";
		fullPath += fileName;
		return fullPath;
	}

	/** 获取全路径中的目录路径
	*/
	static std::wstring GetFolderDirectory(const std::wstring& filePath);
	static std::string GetFolderDirectory(const std::string& filePath);

	// 获取根目录
	static std::wstring GetRootDirectory(const std::wstring& filePath);
	static std::string GetRootDirectory(const std::string& filePath);

public:

	typedef std::vector<std::wstring> CStdStringVector;

	/** 获取用户Application Data目录
	采用windows api SHGetSpecialFolderPath来获取
	*/
	static std::wstring GetAppDataDir(void);

	/** 获取用户MyDocument目录
	采用windows api SHGetSpecialFolderPath来获取
	*/
	static std::wstring GetMyDocumentDir(void);

	/** 获取所有用户的Application Data目录
	采用windows api SHGetSpecialFolderPath来获取
	*/
	static std::wstring GetCommonAppDataDir(void);

	/** 获取用户主目录
	采用windows api SHGetSpecialFolderPath来获取用户主目录
	*/
	static std::wstring GetHomeDir(void);

	/** 获取用户临时目录
	采用windows api SHGetSpecialFolderPath来获取用户临时目录
	*/
	static std::wstring GetTempDir(void);

	/** 创建多级目录
	*/
	static BOOL CreateMultipleDirectory(const std::wstring& strPath);

	/** 删除多级目录
	*/
	static BOOL RemoveMultipleDirectory(const std::wstring& strPath);

	/** 计算目录下文件个数
	*/
	static int CountFile(const std::wstring& strPath);

#define DIR_MASK_ALL L"*"
#define FILE_MASK_ALL L"*"
	/** 获取strDir目录下的所有子目录
	将.和..两个系统缺省目录过滤掉了
	本函数中采用了模拟递归的方式来将递归算法非递归化
	@return bool,true:表示获取到了子目录，false表示没有获取到任何子目录
	*/
	static BOOL GetAllSubDir(
		const std::wstring & strDir, 
		CStdStringVector &subDirQueue,
		const std::wstring &strDirMask = DIR_MASK_ALL		///目录匹配符可以包含*,?等
		);

	/** 获取strDir目录下的所有文件，不包括子目录下文件
	将.和..两个系统缺省目录过滤掉了
	@return bool,true:表示获取到了子目录，false表示没有获取到任何子目录
	*/
	static BOOL GetDirFile(
		const std::wstring & strDir, 
		CStdStringVector &vecFile,
		const std::wstring &strFileMask = FILE_MASK_ALL		///文件匹配符可以包含*,?等
		);

	/** 获取strDir目录下的所有文件，包括子目录下文件
	@return bool,true:表示获取到了文件，false表示没有获取到任何文件
	*/
	static BOOL GetAllFile(
		const std::wstring & strDir, 
		CStdStringVector &vecFile,
		const std::wstring &strDirMask = DIR_MASK_ALL	,	///目录匹配符可以包含*,?等
		const std::wstring &strFileMask = FILE_MASK_ALL		///文件匹配符可以包含*,?等 
		);

	/** 判断是否是该过滤掉的目录
	将.和..两个系统缺省目录过滤掉了
	@return bool,true:是，false：否
	*/
	static BOOL DirFilter(const std::wstring &strDirName);

	/** 判断相应目录是否存在	
	@return BOOL,TRUE:存在,FALSE:不存在.
	*/
	static BOOL IsDirExist(const std::wstring &strDirPath);

	/** 判断相应文件是否存在
	@return BOOL,TRUE:存在,FALSE:不存在.
	*/
	static BOOL IsFileExist(const std::wstring &strFilePath);

	/** 删除某个目录下所有信息
	@return bool.
	*/
	static BOOL RemoveDir(std::wstring & strPath, const bool &bForce = false);

	/** 强制删除某个目录下所有信息,包括目录
	@return bool.
	*/
	static BOOL ForceRemoveDir(const std::wstring &strPath);

	/** 强制删除某个目录下所有文件，但不包括目录
	@return bool.
	*/
	static BOOL RemoveDirAllFile(const std::wstring &strPath);

	/** 强制删除某个目录下所有子目录
	@return bool.
	*/
	static BOOL RemoveDirAllSubDir(const std::wstring &strPath);
public:
	static BOOL CreateFolder(const wstring& wstrFolderName);
public:
	static BOOL GetPathFromShortCut(const wstring& wstrFileName,wstring& workDir,wstring& pathName);
public:
	static BOOL Standardized(const wstring& wstrFileName,wstring& outFileName);
public:
	static unsigned long long GetDirFileSize(const wstring& wstrPath)
	{
		return 0;
	}
public:
private:
	typedef bool (*PFNDoFile)(const wchar_t* wstrFileName,void* pUserData);
	typedef bool (*PFNDoDir)(const wchar_t* wstrDirName,void* pUserData);
private:
	static void EnumDir(const wchar_t* wstrFilePath,bool recursive,PFNDoDir pfnDoDir,PFNDoFile pfnDoFile,void* pUserData)
	{
		if(NULL == wstrFilePath || NULL == pfnDoDir || NULL == pfnDoFile) return;
		if(!::PathIsDirectoryW(wstrFilePath)) return;

		wchar_t wszFilePath[MAX_PATH+1] = { 0 },wszFullPath[MAX_PATH+1] = { 0 };
		wcsncpy_s(wszFilePath,MAX_PATH,wstrFilePath,MAX_PATH);
		wcsncat_s(wszFilePath,MAX_PATH,L"\\*",2);

		WIN32_FIND_DATAW findFileData;
		HANDLE hListFile = FindFirstFileW(wszFilePath,&findFileData);
		if(INVALID_HANDLE_VALUE == hListFile) return;

		do 
		{
			if(0 == wcsncmp(findFileData.cFileName,L".",1) || 0 == wcsncmp(findFileData.cFileName,L"..",2)) continue;
			swprintf_s(wszFullPath,MAX_PATH,L"%s\\%s",wstrFilePath,findFileData.cFileName);
			if(findFileData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
			{
				if(recursive) EnumDir(wstrFilePath,recursive,pfnDoDir,pfnDoFile,pUserData);
				else if(NULL != pfnDoDir) pfnDoDir(wszFullPath,pUserData);
			}
			else if(NULL != pfnDoFile) pfnDoFile(wszFullPath,pUserData);
		}while(FindNextFileW(hListFile,&findFileData));
		FindClose(hListFile);
	}
private:
	static HMODULE m_hModule;
};

#endif
