#include "PathHelper.h"
#include "Tracer.h"

HMODULE PathHelper::m_hModule = NULL;

string PathHelper::GetFullPath(HMODULE hModule, LPCSTR path)
{
	if(hModule == NULL)
	{
		hModule = m_hModule;
	}

	char buffer[MAX_PATH] = {0};
	GetModuleFileNameA(hModule, buffer, _countof(buffer));

	string fullPath(buffer);
	size_t pos = fullPath.rfind('\\');
	if(string::npos != pos) fullPath = fullPath.substr(0,pos);
	fullPath += "\\";

	fullPath += path;

	return fullPath;
}

wstring PathHelper::GetFullPath(HMODULE hModule, LPCWSTR path)
{
	if(hModule == NULL)
	{
		hModule = m_hModule;
	}

	WCHAR buffer[MAX_PATH] = {0};
	GetModuleFileNameW(hModule, buffer, _countof(buffer));

	wstring fullPath(buffer);
	size_t pos = fullPath.rfind('\\');
	if(wstring::npos != pos) fullPath = fullPath.substr(0,pos);
	fullPath += L"\\";
	fullPath += path;

	return fullPath;
}

wstring PathHelper::GetFileExtension(LPCWSTR fileName)
{
	wstring ext,fullPath(fileName);
	size_t pos = fullPath.rfind('.');
	if(pos != wstring::npos) ext = fullPath.substr(pos+1);
	return ext;
}

// GetBaseFileName
// This helper function returns file name without extension
wstring PathHelper::GetBaseFileName(const wstring& wstrFileName)
{
	wstring sBase = wstrFileName;
	int pos1 = wstrFileName.rfind('\\');
	if(pos1>=0)
		sBase = sBase.substr(pos1+1);
	else
	{
		int pos3 = wstrFileName.rfind('/');
		if(pos3>=0)
			sBase = sBase.substr(pos3+1);
	}

	int pos2 = sBase.rfind('.');
	if(pos2>=0)
	{
		sBase = sBase.substr(0, pos2);
	}
	return sBase;
}


wstring PathHelper::GetGamePath(HMODULE hModule)
{
	if(hModule == NULL)
	{
		hModule = m_hModule;
	}

	WCHAR buffer[MAX_PATH] = {0};
	GetModuleFileNameW(hModule, buffer, _countof(buffer));

	wstring fullPath(buffer);
	size_t pos = fullPath.rfind('\\');
	if(wstring::npos != pos) fullPath = fullPath.substr(0,pos);

	pos = fullPath.rfind('\\');
	if(wstring::npos != pos) fullPath = fullPath.substr(0,pos);
	
	return fullPath;
}

string PathHelper::GetGamePath(HMODULE hModule, LPCSTR path)
{
	if(hModule == NULL)
	{
		hModule = m_hModule;
	}

	char buffer[MAX_PATH] = {0};
	GetModuleFileNameA(hModule, buffer, _countof(buffer));

	string fullPath(buffer);
	size_t pos = fullPath.rfind('\\');
	if(string::npos != pos) fullPath = fullPath.substr(0,pos);

	pos = fullPath.rfind('\\');
	if(string::npos != pos) fullPath = fullPath.substr(0,pos);

	pos = fullPath.rfind('\\');
	if(string::npos != pos) fullPath = fullPath.substr(0,pos);

	fullPath += "\\";

	fullPath += path;

	return fullPath;
}

std::wstring PathHelper::GetFolderDirectory(const std::wstring& filePath)
{
	if (filePath.empty()) return L"";

	std::wstring fullPath(filePath);

	size_t pos = fullPath.rfind(L'\\');
	size_t pos1 = fullPath.rfind(L'/');

	// 正确处理 npos
	size_t finalPos = std::wstring::npos;

	if (pos != std::wstring::npos && pos1 != std::wstring::npos)
		finalPos = (pos > pos1) ? pos : pos1;
	else if (pos != std::wstring::npos)
		finalPos = pos;
	else
		finalPos = pos1;

	if (finalPos != std::wstring::npos)
		fullPath = fullPath.substr(0, finalPos);

	fullPath += L'\\';
	return fullPath;
}

//std::string PathHelper::GetFolderDirectory( const std::string& filePath )
//{
//	string fullPath(filePath);
//	size_t pos = fullPath.rfind('\\');
//	size_t pos1 = fullPath.rfind('\/');
//	
//	pos1 = pos1 > 1024? 0:pos1;
//	pos = pos > 1024? 0:pos;
//
//	pos = max(pos, pos1);
//	if(string::npos != pos) fullPath = fullPath.substr(0,pos);
//	fullPath += "\\";
//
//	return fullPath;
//}

std::string PathHelper::GetFolderDirectory(const std::string& filePath)
{
	if (filePath.empty())
		return "";

	std::string fullPath(filePath);

	size_t pos_backslash = fullPath.rfind('\\');
	size_t pos_slash = fullPath.rfind('/');

	size_t pos = std::string::npos;

	if (pos_backslash != std::string::npos && pos_slash != std::string::npos)
		pos = (pos_backslash > pos_slash) ? pos_backslash : pos_slash;
	else if (pos_backslash != std::string::npos)
		pos = pos_backslash;
	else
		pos = pos_slash;

	if (pos != std::string::npos)
		fullPath = fullPath.substr(0, pos);

	fullPath += '\\';
	return fullPath;
}

//std::wstring PathHelper::GetRootDirectory( const std::wstring& filePath )
//{
//	if (filePath.length() == 0) return L"";
//	wstring fullPath(filePath);
//	size_t pos = fullPath.find('\\');
//	size_t pos1 = fullPath.find('\/');
//
//	pos1 = pos1 > 1024? 0:pos1;
//	pos = pos > 1024? 0:pos;
//
//	pos = min(pos, pos1);
//	if(wstring::npos != pos ) fullPath = fullPath.substr(0,pos);
//	// fullPath += L"\\";
//
//	return fullPath;
//}

std::wstring PathHelper::GetRootDirectory(const std::wstring& filePath)
{
	if (filePath.empty())
		return L"";

	std::wstring fullPath(filePath);

	size_t pos_backslash = fullPath.find(L'\\');
	size_t pos_slash = fullPath.find(L'/');

	size_t pos = std::wstring::npos;

	if (pos_backslash != std::wstring::npos && pos_slash != std::wstring::npos)
		pos = (pos_backslash < pos_slash) ? pos_backslash : pos_slash;
	else if (pos_backslash != std::wstring::npos)
		pos = pos_backslash;
	else
		pos = pos_slash;

	if (pos != std::wstring::npos)
		return fullPath.substr(0, pos);

	return fullPath;
}

//std::string PathHelper::GetRootDirectory( const std::string& filePath )
//{
//	string fullPath(filePath);
//	size_t pos = fullPath.find('\\');
//	size_t pos1 = fullPath.find('\/');
//
//	pos1 = pos1 > 1024? 0:pos1;
//	pos = pos > 1024? 0:pos;
//
//	pos = min(pos, pos1);
//	if(string::npos != pos) fullPath = fullPath.substr(0,pos);
//	// fullPath += "\\";
//
//	return fullPath;
//}

std::string PathHelper::GetRootDirectory(const std::string& filePath)
{
	if (filePath.empty())
		return "";

	std::string fullPath(filePath);

	size_t pos_backslash = fullPath.find('\\');
	size_t pos_slash = fullPath.find('/');

	size_t pos = std::string::npos;

	if (pos_backslash != std::string::npos && pos_slash != std::string::npos)
		pos = (pos_backslash < pos_slash) ? pos_backslash : pos_slash;
	else if (pos_backslash != std::string::npos)
		pos = pos_backslash;
	else
		pos = pos_slash;

	if (pos != std::string::npos)
		return fullPath.substr(0, pos);

	return fullPath;
}

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <shlobj.h>
#include <string>
#include <vector>
#include <queue>
#include "PathHelper.h"
#include "time.h"
#include <tchar.h>

//////////////////////////////////////////////////////////////////////////
///#if 1 ... #else 范围内的下面函数全部为Unicode版本的函数

/** 获取用户Application Data目录
采用windows api SHGetSpecialFolderPath来获取
*/
std::wstring PathHelper::GetAppDataDir(void)
{
	std::wstring strAppDataPath;

	///获取用户对应的 application data 目录,例:C:\Documents and Settings\username\Application Data
	WCHAR strPath[MAX_PATH] = {0};
	BOOL bResult = SHGetSpecialFolderPathW(NULL, strPath, CSIDL_APPDATA, FALSE);

	strAppDataPath = strPath;
	return strAppDataPath;
}

/** 获取用户MyDocument目录
采用windows api SHGetSpecialFolderPath来获取
*/
std::wstring PathHelper::GetMyDocumentDir(void)
{
	std::wstring strMyDocumentPath;

	///获取用户对应的 application data 目录,例:C:\Documents and Settings\wenhm\My Documents
	WCHAR strPath[MAX_PATH] = {0};
	BOOL bResult = SHGetSpecialFolderPathW(NULL, strPath,  CSIDL_PERSONAL, FALSE);

	strMyDocumentPath = strPath;
	return strMyDocumentPath;
}

/** 获取所有用户的Application Data目录
采用windows api SHGetSpecialFolderPath来获取
*/
std::wstring PathHelper::GetCommonAppDataDir(void)
{
	std::wstring strAppDataPath;

	/**获取the file system directory containing application data for all users目录, 
	例:C:\Documents and Settings\All Users.WINDOWS\Application Data
	*/
	WCHAR strPath[MAX_PATH] = {0};
	BOOL bResult = SHGetSpecialFolderPathW(NULL, strPath, CSIDL_COMMON_APPDATA, FALSE);

	strAppDataPath = strPath;
	return strAppDataPath;
}

/** 获取用户主目录
采用Windows Api SHGetSpecialFolderPath来获取用户主目录
*/
std::wstring PathHelper::GetHomeDir(void)
{
	std::wstring strHomePath;

	///获取用户对应的 CSIDL_PROFILE 目录, 例:C:\Documents and Settings\username
	WCHAR strPath[MAX_PATH] = {0};
	BOOL bResult = SHGetSpecialFolderPathW(NULL, strPath, CSIDL_PROFILE, FALSE);

	strHomePath = strPath;
	return strHomePath;
}


/** 获取用户临时目录
采用windows api SHGetSpecialFolderPath来获取用户临时目录
*/
std::wstring PathHelper::GetTempDir(void)
{
	std::wstring strTempPath;

	///获取用户对应的 CSIDL_PROFILE 目录, 例:C:\Documents and Settings\username
	WCHAR strPath[MAX_PATH] = {0};
	int iPathLen = ::GetTempPathW(MAX_PATH, strPath);

	strTempPath = strPath;
	return strTempPath;
}

BOOL PathHelper::CreateMultipleDirectory(const std::wstring& strPath)
{
	//存放要创建的目录字符串
	std::wstring strDir(strPath);

	//确保以'\'结尾以创建最后一个目录
	if (strDir[strDir.length() - 1] != (L'\\'))
	{
		strDir += L'\\';
	}

	//存放每一层目录字符串
	std::vector<std::wstring> vecPath;
	//一个临时变量,存放目录字符串
	std::wstring strTemp;

	//遍历要创建的字符串
	for (UINT i=0; i<strDir.length(); ++i)
	{
		if (strDir[i] != L'\\' && strDir[i] != L'/' ) 
		{	
			//如果当前字符不是'\\'
			strTemp += strDir[i];
		}
		else 
		{
			//如果当前字符是'\\',将当前层的字符串添加到数组中
			vecPath.push_back(strTemp);
			strTemp += strDir[i];
		}
	}

	//成功标志
	bool bSuccess = true;
	//遍历存放目录的数组,创建每层目录
	std::vector<std::wstring>::const_iterator iter;
	for (iter = vecPath.begin(); iter != vecPath.end(); iter++) 
	{
		DWORD dwAttr = GetFileAttributesW(iter->c_str());
		if(0xffffffff != dwAttr && (dwAttr&FILE_ATTRIBUTE_DIRECTORY)) continue;
		//如果CreateDirectory执行成功,返回true,否则返回false
		bSuccess = CreateDirectoryW(iter->c_str(), NULL) ? true : false;    
	}

	return bSuccess;
}

BOOL PathHelper::RemoveMultipleDirectory(const std::wstring& strPath)
{
	//存放要删除的目录字符串
	std::wstring strDir(strPath);

	//确保以'\'结尾以删除最后一个目录
	if (strDir[strDir.length() - 1] != (L'\\'))
	{
		strDir += L'\\';
	}

	//存放每一层目录字符串
	std::vector<std::wstring> vecPath;
	//一个临时变量,存放目录字符串
	std::wstring strTemp;
	//成功标志
	BOOL bSuccess = FALSE;

	//遍历要删除的字符串
	for (UINT i=0; i<strDir.length(); ++i)
	{
		if (strDir[i] != L'\\') 
		{	
			//如果当前字符不是'\\'
			strTemp += strDir[i];
		}
		else 
		{
			//如果当前字符是'\\',将当前层的字符串添加到数组中
			vecPath.push_back(strTemp);
			strTemp += strDir[i];
		}
	}

	//遍历存放目录的数组,删除每层目录,从最深的目录开始删除,进行逆向访问
	std::vector<std::wstring>::const_reverse_iterator iter;
	for (iter = vecPath.rbegin(); iter != vecPath.rend(); iter++) 
	{
		//如果RemoveDirectory执行成功,返回true,否则返回false
		///bool bResult = iter->find('\\');
		std::wstring::size_type iFindPos = iter->find(L'\\');		
		if (std::wstring::npos != iFindPos)
		{
			bSuccess = RemoveDirectoryW(iter->c_str()) ? true : false;
		}    
	}

	return bSuccess;
}

/** 计算目录下文件个数
	*/
int PathHelper::CountFile(const std::wstring& strPath)
{
	int count = 0;

	WIN32_FIND_DATAW wfd;
	std::wstring tmpPath = strPath + L"\\*.*";
	HANDLE hFind = FindFirstFileW(tmpPath.c_str(), &wfd);
	if (hFind == INVALID_HANDLE_VALUE) return 0;

	do 
	{
		if (wcscmp(wfd.cFileName, L".") == 0 || wcscmp(wfd.cFileName, L"..") == 0) continue;

		if ((wfd.dwFileAttributes & 16) > 0)
		{
			count += CountFile(strPath + L"\\" + wfd.cFileName);
		}
		else if(wcslen(wfd.cFileName) > 0)
		{
			count++;
		}

	} while(FindNextFileW(hFind, &wfd) != 0);

	FindClose(hFind);

	return count;
}

BOOL PathHelper::GetAllSubDir(
							const std::wstring & strDir, 
							CStdStringVector &vecSubDir, 
							const std::wstring &strDirMask
							)
{
	BOOL bResult = false ;
	/*
	下面的代码为访问目录，和操作系统相关，目前为windows操作系统的相关代码
	*/

	std::wstring strFileName;
	//存放查找的当前目录
	std::wstring strCurDirName; 
	//存放查找中碰到的中间目录
	std::wstring strSubDirName; 
	//用来模拟递归用的存储中间遇到的子目录
	std::queue<std::wstring> recursiveDirQueue; 

	strCurDirName = strDir;
	strFileName = strDir;
	strFileName += L"\\";
	strFileName += strDirMask;

	HANDLE hFindFile;
	WIN32_FIND_DATAW fileinfo;	
	hFindFile = FindFirstFileW(strFileName.c_str(), &fileinfo);

	while (INVALID_HANDLE_VALUE != hFindFile)
	{
		if ((fileinfo.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) == false) 
		{
			//文件不为子目录
			/**
			push(fileinfo.cFileName);
			printf ("The  file found is %s\n", fileinfo.cFileName);
			*/
		}
		else  
		{
			//将查找中间碰到的子目录全部存入dirQueue队列中，以后还要对该目录进行查找
			bResult = DirFilter(fileinfo.cFileName);
			if (TRUE != bResult) //不为要过滤掉的.和..目录则压入目录队列中
			{
				strSubDirName = strCurDirName;
				strSubDirName += L"\\";
				strSubDirName += fileinfo.cFileName;
				///sprintf(strSubDirName,"%s\\%s" , strCurDirName, fileinfo.cFileName);
				recursiveDirQueue.push(strSubDirName);
				vecSubDir.push_back(strSubDirName);
				//printf ("The  subdir found is %s\n", strSubDirName.c_str());
			}
		}

		bResult = FindNextFileW(hFindFile, &fileinfo);
		if (false == bResult)
		{
			/*
			if (GetLastError() == ERROR_NO_MORE_FILES) 
			{ 
			printf("No more %s files.", m_StrFileName);
			} 
			else 
			{ 
			printf("Couldn't find next file."); 
			}
			*/

			FindClose(hFindFile);

			if (recursiveDirQueue.size() > 0)
			{
				strCurDirName  = recursiveDirQueue.front( );
				recursiveDirQueue.pop();

				//sprintf(strFileName, "%s\\*", strCurDirName);
				strFileName = strCurDirName;
				strFileName += L"\\";
				strFileName += strDirMask;
				hFindFile = FindFirstFileW(strFileName.c_str(), &fileinfo);
			}
			else
			{
				break;
			}			
		}
	}

	bResult = (vecSubDir.size() > 0) ;
	return bResult;
}

BOOL PathHelper::GetDirFile(
						  const std::wstring &strDir,  
						  CStdStringVector &vecFile,
						  const std::wstring &strFileMask
						  )
{
	BOOL bResult = false;
	vecFile.clear();
	/*
	下面的代码为访问目录，和操作系统相关，目前为windows操作系统的相关代码
	*/

	std::wstring strFileName;
	//存放查找的当前目录
	std::wstring strCurDirName; 
	//存放查找中碰到的中间目录
	std::wstring strSubDirName; 
	//用来模拟递归用的存储中间遇到的子目录
	std::queue<std::wstring> recursiveDirQueue; 

	strCurDirName = strDir;
	strFileName = strDir;
	strFileName += L"\\";
	strFileName += strFileMask;

	HANDLE hFindFile;
	WIN32_FIND_DATAW fileinfo;	
	hFindFile = FindFirstFileW(strFileName.c_str(), &fileinfo);

	while (INVALID_HANDLE_VALUE != hFindFile)
	{
		if ((fileinfo.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) == false) 
		{
			std::wstring strTmpFile = strCurDirName;
			strTmpFile += L"\\";
			strTmpFile += fileinfo.cFileName;
			//文件不为子目录
			vecFile.push_back(strTmpFile);
			/**
			push(fileinfo.cFileName);
			printf ("The  file found is %s\n", fileinfo.cFileName);
			*/
		}
		else  
		{
#if 0
			//将查找中间碰到的子目录全部存入dirQueue队列中，以后还要对该目录进行查找
			bResult = DirFilter(fileinfo.cFileName);
			if (true != bResult) //不为要过滤掉的.和..目录则压入目录队列中
			{
				strSubDirName += strCurDirName;
				strSubDirName += "\\";
				strSubDirName += fileinfo.cFileName;
				///sprintf(strSubDirName,"%s\\%s" , strCurDirName, fileinfo.cFileName);
				recursiveDirQueue.push(strSubDirName);
				vecSubDir.push_back(strSubDirName);
				printf ("The  subdir found is %s\n", strSubDirName.c_str());
			}
#endif
		}

		bResult = FindNextFileW(hFindFile, &fileinfo);

		if (false == bResult)
		{
			/*
			if (GetLastError() == ERROR_NO_MORE_FILES) 
			{ 
			printf("No more %s files.", m_StrFileName);
			} 
			else 
			{ 
			printf("Couldn't find next file."); 
			}
			*/

			FindClose(hFindFile);
			break;
		}
	}

	bResult = (vecFile.size() > 0) ;
	return bResult;
}

BOOL PathHelper::GetAllFile(
						  const std::wstring & strDir, 
						  CStdStringVector &vecFile,
						  const std::wstring &strDirMask, 
						  const std::wstring &strFileMask 
						  )
{
	BOOL bResult = FALSE;
	vecFile.clear();

	CStdStringVector vecSubDir;

	///获取所有子目录
	bResult = GetAllSubDir(strDir, vecSubDir, strDirMask);

	///将目录自己也压入进去以查找文件
	vecSubDir.push_back(strDir);

	if (vecSubDir.size() > 0)
	{
		CStdStringVector::iterator iter = vecSubDir.begin();
		while (iter != vecSubDir.end())
		{
			CStdStringVector vecTmpFile;
			GetDirFile(*iter, vecTmpFile, strFileMask);

			vecFile.insert(vecFile.end(), vecTmpFile.begin(), vecTmpFile.end());

			iter ++;
		}
	}

	bResult = (vecFile.size() > 0) ;
	return bResult;
}

BOOL PathHelper::DirFilter(const std::wstring &strDirName)
{
	BOOL bReturn = false;

	if ( (strDirName == L".") || ( strDirName == L".."))
	{
		bReturn = true ;
	}

	return bReturn;
}

BOOL PathHelper::IsDirExist(const std::wstring &strDirPath)
{
	WIN32_FIND_DATAW fd;
	BOOL bIsDirExist = FALSE;

	std::wstring strDirPathTemp = strDirPath;

	///确保删除掉后尾的'\'或是'/',不然FindFirstFile查找结尾带/的目录会失败的
	while (		strDirPathTemp.length() > 0 
		&&	(
		strDirPathTemp[strDirPathTemp.length() - 1] == L'\\'
		||  strDirPathTemp[strDirPathTemp.length() - 1] == L'/'
		)
		)
	{
		strDirPathTemp.erase(strDirPathTemp.length()-1);
	}

	HANDLE hFind = ::FindFirstFileW(strDirPathTemp.c_str(), &fd);
	if ((hFind != INVALID_HANDLE_VALUE) && (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
	{
		//目录存在
		bIsDirExist = TRUE;
	}
	FindClose(hFind);

	return bIsDirExist;
}

BOOL PathHelper::IsFileExist(const std::wstring &strFilePath)
{
	WIN32_FIND_DATAW fd;
	BOOL bIsFileExist = FALSE;

	std::wstring strDirPathTemp = strFilePath;

	///确保删除掉后尾的'\'或是'/',不然FindFirstFile查找结尾带/的目录会失败的
	while (		strDirPathTemp.length() > 0 
		&&	(
		strDirPathTemp[strDirPathTemp.length() - 1] == L'\\'
		||  strDirPathTemp[strDirPathTemp.length() - 1] == L'/'
		)
		)
	{
		strDirPathTemp.erase(strDirPathTemp.length()-1);
	}

	HANDLE hFind = ::FindFirstFileW(strDirPathTemp.c_str(), &fd);
	if ((hFind != INVALID_HANDLE_VALUE) && !(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
	{
		//文件存在
		bIsFileExist = TRUE;
	}
	FindClose(hFind);

	return bIsFileExist;
}

/** 删除某个目录下所有信息
@return bool.
*/
BOOL PathHelper::RemoveDir(std::wstring & strPath, const bool &bForce)
{
	BOOL bResult = false;

	if (true == bForce)
	{
		bResult = ForceRemoveDir(strPath);
	}
	else
	{
		bResult = RemoveDirectoryW(strPath.c_str());
	}

	return bResult;
}

BOOL PathHelper::RemoveDirAllFile(const std::wstring &strPath)
{
	BOOL bResult = false;

	///获取所有的*.*文件
	PathHelper::CStdStringVector vecFile;
	bResult = PathHelper::GetAllFile(strPath, vecFile, DIR_MASK_ALL, L"*.*");	

	///删除所有*.*文件
	PathHelper::CStdStringVector::iterator iter = vecFile.begin();
	while (iter != vecFile.end())
	{
		const std::wstring &strTmpFile = *iter;

		bResult = ::DeleteFileW(strTmpFile.c_str());

		iter ++;
	}

	return bResult;
}

BOOL PathHelper::RemoveDirAllSubDir(const std::wstring &strPath)
{
	///获取所有子目录
	CStdStringVector vecSubDir;
	BOOL bResult = GetAllSubDir(strPath, vecSubDir);

	///删除所有的子目录,遍历存放目录的数组,删除每层目录,从最深的目录开始删除,进行逆向访问
	if (vecSubDir.size() > 0)
	{
		CStdStringVector::const_reverse_iterator iter = vecSubDir.rbegin();
		while (iter != vecSubDir.rend())
		{
			bResult = RemoveDirectoryW(iter->c_str());

			iter ++;
		}
	}
	else
	{
		bResult = true;
	}

	return bResult;
}

BOOL PathHelper::ForceRemoveDir(const std::wstring &strPath)
{
	///删除所有*.*文件
	BOOL bResult = RemoveDirAllFile(strPath);
	
	///删除所有子目录
	bResult = RemoveDirAllSubDir(strPath);
	
	///删除自己
	bResult = RemoveDirectoryW(strPath.c_str());
		
	return bResult; 
}

//BOOL PathHelper::GetPathFromShortCut(const wstring& wstrFileName,wstring& workDir,wstring& pathName)
//{
//	IShellLink *pLink = NULL;
//	HRESULT hr = ::CoCreateInstance(CLSID_ShellLink,NULL,CLSCTX_INPROC_SERVER,IID_IShellLink,(void**)&pLink);
//	if(S_OK != hr)
//	{
//		TRACEW("create instance failed,%08x",hr);
//		::CoInitialize(NULL);
//		::OleInitialize(NULL);
//		hr = ::CoCreateInstance(CLSID_ShellLink,NULL,CLSCTX_INPROC_SERVER,IID_IShellLink,(void**)&pLink);
//	}
//	if(S_OK != hr) { TRACEE("create instance failed,%08x",hr);return FALSE; }									// 创建IShellLink对象失败
//
//	IPersistFile * ppf = NULL;
//	hr = pLink->QueryInterface(IID_IPersistFile,(void**)&ppf);
//	if(S_OK == hr)													// 查询IPersistFile成功
//	{
//		hr = ppf->Load(wstrFileName.c_str(),STGM_READ);
//		if(S_OK == hr)
//		{
//			wchar_t szBuffer[MAX_PATH+1] = { 0 };
//			hr = pLink->GetPath(szBuffer,MAX_PATH,NULL,0);
//			if(S_OK == hr) pathName = szBuffer;
//
//			pLink->GetWorkingDirectory(szBuffer,MAX_PATH);
//			if(S_OK == hr) workDir = szBuffer;
//		}
//		ppf->Release();
//	}
//
//	pLink->Release();
//	return (S_OK == hr);
//}

BOOL PathHelper::GetPathFromShortCut(
	const std::wstring& wstrFileName,
	std::wstring& workDir,
	std::wstring& pathName)
{
	HRESULT hr;

	// ① 使用 Unicode 版本
	IShellLinkW* pLink = NULL;

	hr = ::CoCreateInstance(
		CLSID_ShellLink,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IShellLinkW,
		(void**)&pLink
	);

	if (FAILED(hr))
	{
		TRACEW("create instance failed, %08x", hr);
		::CoInitialize(NULL);
		::OleInitialize(NULL);

		hr = ::CoCreateInstance(
			CLSID_ShellLink,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_IShellLinkW,
			(void**)&pLink
		);
	}

	if (FAILED(hr))
	{
		TRACEE("create instance failed, %08x", hr);
		return FALSE;
	}

	IPersistFile* ppf = NULL;
	hr = pLink->QueryInterface(IID_IPersistFile, (void**)&ppf);
	if (SUCCEEDED(hr))
	{
		hr = ppf->Load(wstrFileName.c_str(), STGM_READ);
		if (SUCCEEDED(hr))
		{
			wchar_t szBuffer[MAX_PATH + 1] = { 0 };

			hr = pLink->GetPath(szBuffer, MAX_PATH, NULL, 0);
			if (SUCCEEDED(hr))
				pathName = szBuffer;

			hr = pLink->GetWorkingDirectory(szBuffer, MAX_PATH);
			if (SUCCEEDED(hr))
				workDir = szBuffer;
		}
		ppf->Release();
	}

	pLink->Release();
	return SUCCEEDED(hr);
}

BOOL PathHelper::Standardized(const wstring& wstrFileName,wstring& outFileName)
{
	// 处理路径中的'\\'和'/'
	outFileName = L"";
	for(size_t i = 0;i < wstrFileName.size();++i)
	{
		if('/' == wstrFileName[i])
		{
			outFileName.push_back('\\');
		}
		else if('\\' == wstrFileName[i])
		{
			if(0 != i && '\\' == wstrFileName[i-1]) continue;
			else outFileName.push_back('\\');
		}
		else
		{
			outFileName.push_back(wstrFileName[i]);
		}
	}
	return TRUE;
}

// Creates a folder. If some intermediate folders in the path do not exist,
// it creates them.
BOOL PathHelper::CreateFolder(const wstring& wstrFolderName)
{  
	wstring sIntermediateFolder;

	// Skip disc drive name "X:\" if presents
	int start = wstrFolderName.find(':', 0);
	if(start >= 0)
		start += 2; 

	int pos = start;  
	for(;;)
	{
		pos = wstrFolderName.find('\\', pos);
		if(pos < 0)
		{
			sIntermediateFolder = wstrFolderName;
		}
		else
		{
			sIntermediateFolder = wstrFolderName.substr(0,pos);
		}

		BOOL bCreate = CreateDirectoryW(sIntermediateFolder.c_str(), NULL);
		if(!bCreate && GetLastError()!=ERROR_ALREADY_EXISTS)
		{
			return FALSE;
		}

		DWORD dwAttrs = GetFileAttributesW(sIntermediateFolder.c_str());
		if((dwAttrs&FILE_ATTRIBUTE_DIRECTORY)==0)
		{
			return FALSE;
		}

		if(pos==-1)
			break;

		pos++;
	}

	return TRUE;
}