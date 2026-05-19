#include <fstream>
#include <stdio.h>
#include <time.h>
#include <Windows.h>
#include "LicenseManager.h"
#include "SdoaEncrypt.h"
#include <cstdint>

using namespace std;

LicenseManager* LicenseManager::sm_pInstance = NULL;
wstring LicenseManager::sm_strPath;

LicenseManager::LicenseManager() 
{
	m_map.insert(make_pair("Expiration", "1"));
	m_map.insert(make_pair("UsePlainTextConfig", "0"));
	m_map.insert(make_pair("SkipUpdate", "0"));
	m_map.insert(make_pair("GenerateLog", "0"));

	LoadLicenseFile();
}

LicenseManager* LicenseManager::GetInstance()
{
	if (sm_pInstance == NULL)
	{
		sm_pInstance = new LicenseManager();
	}
	return sm_pInstance;
}

void LicenseManager::Initial(wstring strPath)
{
	sm_strPath = strPath;
}

void LicenseManager::Destroy()
{
	if (sm_pInstance != NULL)
	{
		delete sm_pInstance;
		sm_pInstance = NULL;
	}
}

string LicenseManager::ParseFile(wstring& strFileName, bool bFileEncrypted)
{
	ifstream targetfile(strFileName.c_str(), ios::in | ios::binary);
	if (!targetfile.is_open())
	{
		return "";
	}

	targetfile.seekg(0, ios::end);
	uint64_t nInLen = static_cast<uint64_t>(targetfile.tellg());
	targetfile.seekg(0, ios::beg);

	if (nInLen == 0 || nInLen > UINT_MAX)
	{
		return "";
	}

	size_t bufSize = static_cast<size_t>(nInLen) + 1;
	char* inBuffer = new char[bufSize];
	memset(inBuffer, 0, bufSize);

	targetfile.read(inBuffer, static_cast<std::streamsize>(nInLen));

	string strResult;

	if (bFileEncrypted)
	{
		char* outBuffer = new char[bufSize];
		memset(outBuffer, 0, bufSize);

		UINT nOutLen = 0;
		UINT inLen32 = static_cast<UINT>(nInLen);

		const BYTE* pKey = SodaEncrypt::GetConstantKey();
		SodaEncrypt::Decrypt(
			(BYTE*)inBuffer + sizeof(file_header_t),
			inLen32 - sizeof(file_header_t),
			pKey,
			(BYTE*)outBuffer,
			&nOutLen
		);

		strResult.assign(outBuffer, nOutLen);
		delete[] outBuffer;
	}
	else
	{
		strResult.assign(inBuffer, static_cast<size_t>(nInLen));
	}

	delete[] inBuffer;
	return strResult;
}

void LicenseManager::LoadLicenseFile()
{
	wstring strLicenseFile = sm_strPath + L"License.bin";
	
	string strBuffer = ParseFile(strLicenseFile, true);
	if (strBuffer.empty())
	{
		return;
	}

	vector<string> vecItems;
	StringHelper::Split(strBuffer.c_str(), "\n", vecItems);
	
	for (vector<string>::iterator itr = vecItems.begin(); itr != vecItems.end(); itr++)
	{
		vector<string> vec;
		StringHelper::Split((*itr).c_str(), "=", vec);

		if (vec.size() != 2)
		{
			continue;
		}

		if (vec[0] == "Expiration")
		{
			SetExpired(vec[1]);
		}
		else
		{
			m_map[vec[0]] = vec[1];
		}
	}
}
