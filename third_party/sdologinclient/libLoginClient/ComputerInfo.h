////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Created by Ma Guangguang
// 2009.08.25
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _COMPUTER_INFO_H_
#define _COMPUTER_INFO_H_

class CComputerInfo
{
public:
	//
	// Get the description of the video card
	//		szVideoCardDesc[out]			--	Save the video card description
	//		iVideoCardDescLen[out]			--	Save the length of video card description string, including the terminating null character.
	//											If failed, the value is 0. If szVideoCardDesc is NULL input,the value is the needed length of video card description string.
	// Return value:
	//		true	--	success
	//		false	--	failed
	//
	static bool GetVideoCardDesc( char *szVideoCardDesc, int &iVideoCardDescLen );

	//
	// Get the physical model number of the hard disk drive
	//		szDiskSN[out]		--	Save the serial number, its size must larger than 20.
	//		iDiskSNLen[out]		--	Save the length of serial number string, including the terminating null character.
	//								If failed, the value is 0. If szDiskSN is NULL input, the value is -1.
	// Return value:
	//		true	--	success
	//		false	--	failed
	//
	static bool GetDiskSN( char *szDiskSN, int &iDiskSNLen );

	//
	// Get the physical model number of the hard disk drive
	//		szDiskModelNum[out]				--	Save the model number, its size must larger than 40.
	//		iDiskModuleNumberLen[out]		--	Save the length of model number string, including the terminating null character.
	//											If failed, the value is 0. If szDiskSN is NULL input, the value is -1.
	// Return value:
	//		true	--	success
	//		false	--	failed
	//
	static bool GetDiskModelNumber( char *szDiskModelNum, int &iDiskModuleNumberLen );

	//
	// Get the computer name
	//		szComputerName[out]		--	Save the computer name, it is saved in TCHARS in memory.
	//		iComputerNameLen[out]	--	Save the length of computer name string in bytes, including the terminating null character(2 bytes because the string is saved in TCHARS)..
	//									The value is double the length of computer name in TCHARS. If failed, the value is 0. If szComputerName is NULL input, the value is -1.
	// Return value:
	//		true	--	success
	//		false	--	failed
	//
	static bool GetComputerNameSD( char *szComputerName, int &iComputerNameLen );
	//
	// Get the current volume serial number
	//		szVolumeSN[out]		--	Save the volume serial number.
	//		iVolumeSNLen[out]	--	Save the length of volume SN in bytes, including the terminating null character.
	//								If successfully get the SN, the value is 9. If failed, the value is 0. If szVolumeSN is NULL input, the value is -1.
	// Return value:
	//		true	--	success
	//		false	--	failed
	//
	static bool GetVolumeSN( char *szVolumeSN, int &iVolumeSNLen );
	//
	// Get the computer user name
	//		szUsrName[out]		--	Save the computer user name, it is saved in TCHARS in memory.
	//		iUsrNameLen[out]	--	Save the length of user name in bytes, including the terminating null character(2 bytes because the string is saved in TCHARS).
	//								If failed, the value is 0. If szUsrName is NULL, the value is -1.
	// Return value:
	//		true	--	success
	//		false	--	failed
	//
	static bool GetUsrName( char *szUsrName, int &iUsrNameLen );
	//
	// Get the Mac Address, using IP Help API
	//		szMacAddress[out]	--	Save the MAC address, the formant is "XX-XX-XX-XX-XX-XX"
	//		iMacAddressLen[out]	--	Save the length of MAC address, including the terminating null character. Usually, the value is 18
	//								If failed, the value is 0. If szMacAddress is NULL, the value is -1.
	// Return value:
	//		true	--	success
	//		false	--	failed
	//
	static bool GetMacAddress( char *szMacAddress, int &iMacAddressLen, bool isSplit = true );
	//
	// Get the IP Address, using IP Help API
	//		szIP[out]	--	Save the IP address, the formant is "X.X.X.X"
	// Return value:
	//		true	--	success
	//		false	--	failed
	//
	static bool GetIPAddress( char *szIP, int &nIPLen );

	//
	// Get the OS Version
	//		szOSInfo[out]		--	Save the OS Version
	//		nOSInfoLen[in/out]	--	Save the length of OS Version, including the terminating null character
	// Return value:
	//		true	--	success
	//		false	--	failed
	//
	static bool GetOSInfo(char *szOSInfo, int &nOSInfoLen);

	//
	// Get the memory information
	//		nPhyTotal[out]			--	Save the total physical memory size
	//		nPhyAvailable[out]		--	Save the available physical memory size
	//		nVirtualTotal[out]		--	Save the total virtual memory size
	//		nVirtualAvailable[out]	--	Save the available virtual memory size
	// Return value:
	//		true	--	success
	//		false	--	failed
	//
	static bool GetMemInfo(
		uint64_t& nPhyTotalMB,
		uint64_t& nPhyAvailableMB,
		uint64_t& nVirtualTotalMB,
		uint64_t& nVirtualAvailableMB);
	
	//
	// Get the Cpu Information
	//		szCpuInfo[out]		--	Save the CPU information
	//		nCpuInfo[in/out]	--	Save the length of CPU information
	//		nNumOfProcessor[out]--	Save the number of processors
	// Return value:
	//		true	--	success
	//		false	--	failed
	//
	static bool GetCpuInfo(char *szCpuInfo, int &nCpuInfo, int &nNumOfProcessor);

	//
	// Get the Cpu ID
	//		szCpuID[out]		--	Save the CPU ID
	//		nCpuSNLen[in/out]	--	Save the length of CPU ID
	// Return value:
	//		true	--	success
	//		false	--	failed
	//
	static bool GetCpuID(char *szCpuID, int &nCpuIDLen);
public:
	static bool GetIECookiePath(char* path);
};

#endif
