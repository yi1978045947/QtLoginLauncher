#ifndef _LICENSEMANAGER_H_
#define _LICENSEMANAGER_H_

#include <string>
#include <map>
#include "StringHelper.h"
#include <fstream>
#include <stdio.h>
#include <time.h>
#include <Windows.h>
#include "SdoaEncrypt.h"

using std::string;
using std::wstring;
using std::map;

#pragma pack(1)
struct file_header_t
{
	enum { kMagic = 'SDCF', };
	unsigned long magic;
	unsigned long chksum;
	unsigned long size;
};
#pragma pack()

class LicenseManager
{
	LicenseManager();
	LicenseManager(const LicenseManager&) {}
	LicenseManager& operator=(const LicenseManager&) {}
	~LicenseManager() {}
public:
	static LicenseManager* GetInstance();
	static void Initial(wstring strPath);
	static void Destroy();
public:
	template<class ValueType>
	ValueType GetValue(string strKey)
	{
		map<string,string>::iterator itr = m_map.find(strKey);
		while(itr != m_map.end())
		{
			return StringHelper::FromString<ValueType>(itr->second);
		}
		return 0;
	}
private:
	string ParseFile(wstring &strFileName, bool bFileEncrypted);
	void LoadLicenseFile();
	void SetExpired(const string& strTime)
	{
		struct tm tm1;
		time_t time1, time2;

		sscanf_s(strTime.c_str(),"%4d%2d%2d%2d%2d%2d", &tm1.tm_year, &tm1.tm_mon, &tm1.tm_mday, &tm1.tm_hour, &tm1.tm_min, &tm1.tm_sec);
		tm1.tm_year -= 1900;
		tm1.tm_mon --;
		tm1.tm_isdst=-1;

		time1 = mktime(&tm1);
		time2 = time(NULL);

		m_map["Expiration"] = StringHelper::ToString<bool>((difftime(time2,time1) >= 0));
		return;
	}
private:
	static LicenseManager* sm_pInstance;
	static wstring sm_strPath;
private:
	map<string,string> m_map;
};

#endif