////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Created by Ma Guangguang
// 2009.08.25
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "ComputerInfo.h"

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <stdlib.h>
//#include <string>
#include "wininet.h"
#include <sys/stat.h>
#include <setupapi.h>
#include <diskguid.h>
#include <WinIoCtl.h>

#pragma comment(lib,"setupapi.lib")
#pragma comment(lib, "wininet.lib")

// Just for GetMacAddress function, IP Help API
#include "Iphlpapi.h"
#pragma comment(lib,"Iphlpapi.lib")
#define INFO_BUFFER_SIZE 1024

/**************************************************************************************/
// Hard Disk
// IOCTL控制码
#define  DFP_SEND_DRIVE_COMMAND   0x0007c084
//#define  DFP_SEND_DRIVE_COMMAND   CTL_CODE(IOCTL_DISK_BASE, 0x0021, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define  DFP_RECEIVE_DRIVE_DATA   0x0007c088
//#define  DFP_RECEIVE_DRIVE_DATA   CTL_CODE(IOCTL_DISK_BASE, 0x0022, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define  FILE_DEVICE_SCSI           0x0000001B
#define  IOCTL_SCSI_MINIPORT_IDENTIFY      ((FILE_DEVICE_SCSI << 16) + 0x0501)
#define  IOCTL_SCSI_MINIPORT        0x0004D008          //  see NTDDSCSI.H for definition

// ATA/ATAPI指令
#define  IDE_ATA_IDENTIFY           0xEC     // ATA的ID指令(IDENTIFY DEVICE)

// IDE命令寄存器
// typedef struct _IDEREGS
// {
// 	BYTE bFeaturesReg;       // 特征寄存器(用于SMART命令)
// 	BYTE bSectorCountReg;    // 扇区数目寄存器
// 	BYTE bSectorNumberReg;   // 开始扇区寄存器
// 	BYTE bCylLowReg;         // 开始柱面低字节寄存器
// 	BYTE bCylHighReg;        // 开始柱面高字节寄存器
// 	BYTE bDriveHeadReg;      // 驱动器/磁头寄存器
// 	BYTE bCommandReg;        // 指令寄存器
// 	BYTE bReserved;          // 保留
// } IDEREGS, *PIDEREGS, *LPIDEREGS;

// 从驱动程序返回的状态
// typedef struct _DRIVERSTATUS
// {
// 	BYTE bDriverError;      // 错误码
// 	BYTE bIDEStatus;        // IDE状态寄存器
// 	BYTE bReserved[2];      // 保留
// 	DWORD dwReserved[2];    // 保留
// } DRIVERSTATUS, *PDRIVERSTATUS, *LPDRIVERSTATUS;

// IDE设备IOCTL输入数据结构
// typedef struct _SENDCMDINPARAMS
// {
// 	DWORD cBufferSize;      // 缓冲区字节数
// 	IDEREGS irDriveRegs;    // IDE寄存器组
// 	BYTE bDriveNumber;      // 驱动器号
// 	BYTE bReserved[3];      // 保留
// 	DWORD dwReserved[4];    // 保留
// 	BYTE bBuffer[1];        // 输入缓冲区(此处象征性地包含1字节)
// } SENDCMDINPARAMS, *PSENDCMDINPARAMS, *LPSENDCMDINPARAMS;

// IDE设备IOCTL输出数据结构
// typedef struct _SENDCMDOUTPARAMS
// {
// 	DWORD cBufferSize;          // 缓冲区字节数
// 	DRIVERSTATUS DriverStatus;  // 驱动程序返回状态
// 	BYTE bBuffer[1];            // 输入缓冲区(此处象征性地包含1字节)
// } SENDCMDOUTPARAMS, *PSENDCMDOUTPARAMS, *LPSENDCMDOUTPARAMS;

// IDE的ID命令返回的数据
// 共512字节(256个WORD)，这里仅定义了一些感兴趣的项(基本上依据ATA/ATAPI-4)
typedef struct _IDINFO
{
	USHORT  wGenConfig;                 // WORD 0: 基本信息字
	USHORT  wNumCyls;                   // WORD 1: 柱面数
	USHORT  wReserved2;                 // WORD 2: 保留
	USHORT  wNumHeads;                  // WORD 3: 磁头数
	USHORT  wReserved4;                 // WORD 4: 保留
	USHORT  wReserved5;                 // WORD 5: 保留
	USHORT  wNumSectorsPerTrack;        // WORD 6: 每磁道扇区数
	USHORT  wVendorUnique[3];           // WORD 7-9: 厂家设定值
	CHAR    sSerialNumber[20];          // WORD 10-19:序列号
	USHORT  wBufferType;                // WORD 20: 缓冲类型
	USHORT  wBufferSize;                // WORD 21: 缓冲大小
	USHORT  wECCSize;                   // WORD 22: ECC校验大小
	CHAR    sFirmwareRev[8];            // WORD 23-26: 固件版本
	CHAR    sModelNumber[40];           // WORD 27-46: 内部型号
	USHORT  wMoreVendorUnique;          // WORD 47: 厂家设定值
	USHORT  wReserved48;                // WORD 48: 保留
	struct {
		USHORT  reserved1 : 8;
		USHORT  DMA : 1;                  // 1=支持DMA
		USHORT  LBA : 1;                  // 1=支持LBA
		USHORT  DisIORDY : 1;             // 1=可不使用IORDY
		USHORT  IORDY : 1;                // 1=支持IORDY
		USHORT  SoftReset : 1;            // 1=需要ATA软启动
		USHORT  Overlap : 1;              // 1=支持重叠操作
		USHORT  Queue : 1;                // 1=支持命令队列
		USHORT  InlDMA : 1;               // 1=支持交叉存取DMA
	} wCapabilities;                    // WORD 49: 一般能力
	USHORT  wReserved1;                 // WORD 50: 保留
	USHORT  wPIOTiming;                 // WORD 51: PIO时序
	USHORT  wDMATiming;                 // WORD 52: DMA时序
	struct {
		USHORT  CHSNumber : 1;            // 1=WORD 54-58有效
		USHORT  CycleNumber : 1;          // 1=WORD 64-70有效
		USHORT  UnltraDMA : 1;            // 1=WORD 88有效
		USHORT  reserved : 13;
	} wFieldValidity;                   // WORD 53: 后续字段有效性标志
	USHORT  wNumCurCyls;                // WORD 54: CHS可寻址的柱面数
	USHORT  wNumCurHeads;               // WORD 55: CHS可寻址的磁头数
	USHORT  wNumCurSectorsPerTrack;     // WORD 56: CHS可寻址每磁道扇区数
	USHORT  wCurSectorsLow;             // WORD 57: CHS可寻址的扇区数低位字
	USHORT  wCurSectorsHigh;            // WORD 58: CHS可寻址的扇区数高位字
	struct {
		USHORT  CurNumber : 8;            // 当前一次性可读写扇区数
		USHORT  Multi : 1;                // 1=已选择多扇区读写
		USHORT  reserved1 : 7;
	} wMultSectorStuff;                 // WORD 59: 多扇区读写设定
	ULONG  dwTotalSectors;              // WORD 60-61: LBA可寻址的扇区数
	USHORT  wSingleWordDMA;             // WORD 62: 单字节DMA支持能力
	struct {
		USHORT  Mode0 : 1;                // 1=支持模式0 (4.17Mb/s)
		USHORT  Mode1 : 1;                // 1=支持模式1 (13.3Mb/s)
		USHORT  Mode2 : 1;                // 1=支持模式2 (16.7Mb/s)
		USHORT  Reserved1 : 5;
		USHORT  Mode0Sel : 1;             // 1=已选择模式0
		USHORT  Mode1Sel : 1;             // 1=已选择模式1
		USHORT  Mode2Sel : 1;             // 1=已选择模式2
		USHORT  Reserved2 : 5;
	} wMultiWordDMA;                    // WORD 63: 多字节DMA支持能力
	struct {
		USHORT  AdvPOIModes : 8;          // 支持高级POI模式数
		USHORT  reserved : 8;
	} wPIOCapacity;                     // WORD 64: 高级PIO支持能力
	USHORT  wMinMultiWordDMACycle;      // WORD 65: 多字节DMA传输周期的最小值
	USHORT  wRecMultiWordDMACycle;      // WORD 66: 多字节DMA传输周期的建议值
	USHORT  wMinPIONoFlowCycle;         // WORD 67: 无流控制时PIO传输周期的最小值
	USHORT  wMinPOIFlowCycle;           // WORD 68: 有流控制时PIO传输周期的最小值
	USHORT  wReserved69[11];            // WORD 69-79: 保留
	struct {
		USHORT  Reserved1 : 1;
		USHORT  ATA1 : 1;                 // 1=支持ATA-1
		USHORT  ATA2 : 1;                 // 1=支持ATA-2
		USHORT  ATA3 : 1;                 // 1=支持ATA-3
		USHORT  ATA4 : 1;                 // 1=支持ATA/ATAPI-4
		USHORT  ATA5 : 1;                 // 1=支持ATA/ATAPI-5
		USHORT  ATA6 : 1;                 // 1=支持ATA/ATAPI-6
		USHORT  ATA7 : 1;                 // 1=支持ATA/ATAPI-7
		USHORT  ATA8 : 1;                 // 1=支持ATA/ATAPI-8
		USHORT  ATA9 : 1;                 // 1=支持ATA/ATAPI-9
		USHORT  ATA10 : 1;                // 1=支持ATA/ATAPI-10
		USHORT  ATA11 : 1;                // 1=支持ATA/ATAPI-11
		USHORT  ATA12 : 1;                // 1=支持ATA/ATAPI-12
		USHORT  ATA13 : 1;                // 1=支持ATA/ATAPI-13
		USHORT  ATA14 : 1;                // 1=支持ATA/ATAPI-14
		USHORT  Reserved2 : 1;
	} wMajorVersion;                    // WORD 80: 主版本
	USHORT  wMinorVersion;              // WORD 81: 副版本
	USHORT  wReserved82[6];             // WORD 82-87: 保留
	struct {
		USHORT  Mode0 : 1;                // 1=支持模式0 (16.7Mb/s)
		USHORT  Mode1 : 1;                // 1=支持模式1 (25Mb/s)
		USHORT  Mode2 : 1;                // 1=支持模式2 (33Mb/s)
		USHORT  Mode3 : 1;                // 1=支持模式3 (44Mb/s)
		USHORT  Mode4 : 1;                // 1=支持模式4 (66Mb/s)
		USHORT  Mode5 : 1;                // 1=支持模式5 (100Mb/s)
		USHORT  Mode6 : 1;                // 1=支持模式6 (133Mb/s)
		USHORT  Mode7 : 1;                // 1=支持模式7 (166Mb/s) ???
		USHORT  Mode0Sel : 1;             // 1=已选择模式0
		USHORT  Mode1Sel : 1;             // 1=已选择模式1
		USHORT  Mode2Sel : 1;             // 1=已选择模式2
		USHORT  Mode3Sel : 1;             // 1=已选择模式3
		USHORT  Mode4Sel : 1;             // 1=已选择模式4
		USHORT  Mode5Sel : 1;             // 1=已选择模式5
		USHORT  Mode6Sel : 1;             // 1=已选择模式6
		USHORT  Mode7Sel : 1;             // 1=已选择模式7
	} wUltraDMA;                        // WORD 88:  Ultra DMA支持能力
	USHORT    wReserved89[167];         // WORD 89-255
} IDINFO, * PIDINFO;

// SCSI驱动所需的输入输出共用的结构
typedef struct _SRB_IO_CONTROL
{
	ULONG HeaderLength;        // 头长度
	UCHAR Signature[8];        // 特征名称
	ULONG Timeout;             // 超时时间
	ULONG ControlCode;         // 控制码
	ULONG ReturnCode;          // 返回码
	ULONG Length;              // 缓冲区长度
} SRB_IO_CONTROL, * PSRB_IO_CONTROL;

// 需要引起注意的是IDINFO第57-58 WORD (CHS可寻址的扇区数)，因为不满足32位对齐的要求，不可定义为一个ULONG字段。Lynn McGuire的程序里正是由于定义为一个ULONG字段，导致该结构不可用。 

// 以下是核心代码： 

// 打开设备
// filename: 设备的“文件名”(设备路径)

HANDLE OpenDevice(LPCTSTR filename)
{
	HANDLE hDevice;

	// 打开设备
	hDevice = ::CreateFile(filename,            // 文件名
		GENERIC_READ | GENERIC_WRITE,          // 读写方式
		FILE_SHARE_READ | FILE_SHARE_WRITE,    // 共享方式
		NULL,                    // 默认的安全描述符
		OPEN_EXISTING,           // 创建方式
		0,                       // 不需设置文件属性
		NULL);                   // 不需参照模板文件

	return hDevice;
}

// 向驱动发“IDENTIFY DEVICE”命令，获得设备信息
// hDevice: 设备句柄
// pIdInfo:  设备信息结构指针
BOOL IdentifyDevice(HANDLE hDevice, PIDINFO pIdInfo)
{
	PSENDCMDINPARAMS pSCIP;      // 输入数据结构指针
	PSENDCMDOUTPARAMS pSCOP;     // 输出数据结构指针
	DWORD dwOutBytes;            // IOCTL输出数据长度
	BOOL bResult;                // IOCTL返回值

	// 申请输入/输出数据结构空间
	pSCIP = (PSENDCMDINPARAMS)::GlobalAlloc(LMEM_ZEROINIT, sizeof(SENDCMDINPARAMS) - 1);
	pSCOP = (PSENDCMDOUTPARAMS)::GlobalAlloc(LMEM_ZEROINIT, sizeof(SENDCMDOUTPARAMS) + sizeof(IDINFO) - 1);

	// 指定ATA/ATAPI命令的寄存器值
	//    pSCIP->irDriveRegs.bFeaturesReg = 0;
	//    pSCIP->irDriveRegs.bSectorCountReg = 0;
	//    pSCIP->irDriveRegs.bSectorNumberReg = 0;
	//    pSCIP->irDriveRegs.bCylLowReg = 0;
	//    pSCIP->irDriveRegs.bCylHighReg = 0;
	//    pSCIP->irDriveRegs.bDriveHeadReg = 0;
	pSCIP->irDriveRegs.bCommandReg = IDE_ATA_IDENTIFY;

	// 指定输入/输出数据缓冲区大小
	pSCIP->cBufferSize = 0;
	pSCOP->cBufferSize = sizeof(IDINFO);

	// IDENTIFY DEVICE
	bResult = ::DeviceIoControl(hDevice,        // 设备句柄
		DFP_RECEIVE_DRIVE_DATA,                 // 指定IOCTL
		pSCIP, sizeof(SENDCMDINPARAMS) - 1,     // 输入数据缓冲区
		pSCOP, sizeof(SENDCMDOUTPARAMS) + sizeof(IDINFO) - 1,    // 输出数据缓冲区
		&dwOutBytes,                // 输出数据长度
		(LPOVERLAPPED)NULL);        // 用同步I/O

	// 复制设备参数结构
	::memcpy(pIdInfo, pSCOP->bBuffer, sizeof(IDINFO));

	// 释放输入/输出数据空间
	::GlobalFree(pSCOP);
	::GlobalFree(pSCIP);

	return bResult;
}

// 向SCSI MINI-PORT驱动发“IDENTIFY DEVICE”命令，获得设备信息
// hDevice: 设备句柄
// pIdInfo:  设备信息结构指针
BOOL IdentifyDeviceAsScsi(HANDLE hDevice, int nDrive, PIDINFO pIdInfo)
{
	PSENDCMDINPARAMS pSCIP;     // 输入数据结构指针
	PSENDCMDOUTPARAMS pSCOP;    // 输出数据结构指针
	PSRB_IO_CONTROL pSRBIO;     // SCSI输入输出数据结构指针
	DWORD dwOutBytes;           // IOCTL输出数据长度
	BOOL bResult;               // IOCTL返回值

	// 申请输入/输出数据结构空间
	pSRBIO = (PSRB_IO_CONTROL)::GlobalAlloc(LMEM_ZEROINIT,
		sizeof(SRB_IO_CONTROL) + sizeof(SENDCMDOUTPARAMS) + sizeof(IDINFO) - 1);
	pSCIP = (PSENDCMDINPARAMS)((char*)pSRBIO + sizeof(SRB_IO_CONTROL));
	pSCOP = (PSENDCMDOUTPARAMS)((char*)pSRBIO + sizeof(SRB_IO_CONTROL));

	// 填充输入/输出数据
	pSRBIO->HeaderLength = sizeof(SRB_IO_CONTROL);
	pSRBIO->Timeout = 10000;
	pSRBIO->Length = sizeof(SENDCMDOUTPARAMS) + sizeof(IDINFO) - 1;
	pSRBIO->ControlCode = IOCTL_SCSI_MINIPORT_IDENTIFY;
	memcpy_s((char*)pSRBIO->Signature, 8, "SCSIDISK", 8);

	// 指定ATA/ATAPI命令的寄存器值
	//    pSCIP->irDriveRegs.bFeaturesReg = 0;
	//    pSCIP->irDriveRegs.bSectorCountReg = 0;
	//    pSCIP->irDriveRegs.bSectorNumberReg = 0;
	//    pSCIP->irDriveRegs.bCylLowReg = 0;
	//    pSCIP->irDriveRegs.bCylHighReg = 0;
	//    pSCIP->irDriveRegs.bDriveHeadReg = 0;
	pSCIP->irDriveRegs.bCommandReg = IDE_ATA_IDENTIFY;
	pSCIP->bDriveNumber = nDrive;

	// IDENTIFY DEVICE
	bResult = ::DeviceIoControl(hDevice,    // 设备句柄
		IOCTL_SCSI_MINIPORT,                // 指定IOCTL
		pSRBIO, sizeof(SRB_IO_CONTROL) + sizeof(SENDCMDINPARAMS) - 1,    // 输入数据缓冲区
		pSRBIO, sizeof(SRB_IO_CONTROL) + sizeof(SENDCMDOUTPARAMS) + sizeof(IDINFO) - 1,    // 输出数据缓冲区
		&dwOutBytes,            // 输出数据长度
		(LPOVERLAPPED)NULL);    // 用同步I/O

	// 复制设备参数结构
	::memcpy(pIdInfo, pSCOP->bBuffer, sizeof(IDINFO));

	// 释放输入/输出数据空间
	::GlobalFree(pSRBIO);

	return bResult;
}

// 将串中的字符两两颠倒
// 原因是ATA/ATAPI中的WORD，与Windows采用的字节顺序相反
// 驱动程序中已经将收到的数据全部反过来
void AdjustString(char* str, int totalLen, int& realLen)
{
	char ch = 0;
	int i = 0;

	// 两两颠倒
	for (i = 0; i < totalLen; i += 2)
	{
		ch = str[i];
		str[i] = str[i + 1];
		str[i + 1] = ch;
	}

	// 若是右对齐的，调整为左对齐 (去掉左边的空格)
	i = 0;
	while ((i < totalLen) && (str[i] == ' '))
	{
		i++;
	}
	::memmove(str, &str[i], totalLen - i);
	realLen = totalLen - i;

	// 清空右边补过来的内容，同时去掉原有信息右边的空格
	i = totalLen - 1;
	while (i >= 0)
	{
		if (i >= realLen)
		{
			str[i] = '\0';
		}
		else if (str[i] == ' ')
		{
			str[i] = '\0';
		}
		else
		{
			realLen = i + 1;
			break;
		}
		i--;
	}
}

// 读取IDE硬盘的设备信息，必须有足够权限
// nDrive: 驱动器号(0=第一个硬盘，1=0=第二个硬盘，......)
// pIdInfo: 设备信息结构指针
BOOL GetPhysicalDriveInfoInNT(int nDrive, PIDINFO pIdInfo)
{
	HANDLE hDevice;         // 设备句柄
	BOOL bResult;           // 返回结果
	TCHAR szFileName[20] = _T("\\\\.\\PhysicalDrive0");   // 文件名

	hDevice = ::OpenDevice(szFileName);

	if (hDevice == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	// IDENTIFY DEVICE
	bResult = ::IdentifyDevice(hDevice, pIdInfo);

	::CloseHandle(hDevice);

	return bResult;
}

// 用SCSI驱动读取IDE硬盘的设备信息，不受权限制约
// nDrive: 驱动器号(0=Primary Master, 1=Promary Slave, 2=Secondary master, 3=Secondary slave)
// pIdInfo: 设备信息结构指针
BOOL GetIdeDriveAsScsiInfoInNT(int nDrive, PIDINFO pIdInfo)
{
	HANDLE hDevice;         // 设备句柄
	BOOL bResult;           // 返回结果
	TCHAR szFileName[20] = _T("\\\\.\\Scsi0:");    // 文件名

	hDevice = ::OpenDevice(szFileName);

	if (hDevice == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	// IDENTIFY DEVICE
	bResult = ::IdentifyDeviceAsScsi(hDevice, nDrive % 2, pIdInfo);

	// 检查是不是空串
	if (pIdInfo->sModelNumber[0] == '\0')
	{
		bResult = FALSE;
	}

	::CloseHandle(hDevice);

	return bResult;
}

/*********************************************************************************************/
/*********************  The Member Functions show below  *************************************/
/*********************************************************************************************/

//
// Get the description of the video card
//		szVideoCardDesc[out]			--	Save the video card description
//		iVideoCardDescLen[out]			--	Save the length of video card description string, including the terminating null character.
//											If failed, the value is 0. If szVideoCardDesc is NULL input,the value is the needed length of video card description string.
// Return value:
//		true	--	success
//		false	--	failed
//
bool CComputerInfo::GetVideoCardDesc(char* szVideoCardDesc, int& iVideoCardDescLen)
{
	GUID guidVideoCard = { 0x4D36E968,0xE325,0x11CE,0xBF,0xC1,0x08,0x00,0x2B,0xE1,0x03,0x18 };
	HDEVINFO hDevInfo = SetupDiGetClassDevs(&guidVideoCard, 0, NULL, DIGCF_PRESENT);
	// 枚举设备信息
	static const size_t buff_size = 0x1000;
	SP_DEVINFO_DATA spDevinofData = { 0 };
	spDevinofData.cbSize = sizeof(SP_DEVINFO_DATA);
	for (DWORD iCount = 0; SetupDiEnumDeviceInfo(hDevInfo, iCount, &spDevinofData); ++iCount)
	{
		//通过设备类，以及枚举出的设备信息数据获的具体的名称
		char szBuf[buff_size] = { 0 };
		SetupDiGetDeviceRegistryPropertyA(hDevInfo, &spDevinofData, SPDRP_DEVICEDESC, NULL, (PBYTE)szBuf, buff_size, 0);
		if (0 == buff_size) continue;
		size_t bufLen = strlen(szBuf);
		if (szVideoCardDesc == NULL || (size_t)iVideoCardDescLen <= bufLen)
		{
			iVideoCardDescLen = (int)bufLen + 1;
		}
		else
		{
			memcpy(szVideoCardDesc, szBuf, bufLen);
			szVideoCardDesc[bufLen] = '\0';
			iVideoCardDescLen = (int)bufLen;
		}

		break;
	}
	SetupDiDestroyDeviceInfoList(hDevInfo);
	return TRUE;
}
//
// Get the physical serial number of the hard disk drive
//		szDiskSN[out]		--	Save the serial number, its size must larger than 20.
//		iDiskSNLen[out]		--	Save the length of serial number string, including the terminating null character.
//								If failed, the value is 0. If szDiskSN is NULL input, the value is -1.
// Return value:
//		true	--	success
//		false	--	failed
//
bool CComputerInfo::GetDiskSN(char* szDiskSN, int& iDiskSNLen)
{
	if (NULL == szDiskSN || iDiskSNLen <= 0)
	{
		iDiskSNLen = -1;
		return false;
	}

	memset(szDiskSN, 0, iDiskSNLen);

	BOOL nRet;
	PIDINFO pIdInfo = new IDINFO;
	nRet = GetPhysicalDriveInfoInNT(0, pIdInfo);
	if (!nRet)
	{
		nRet = GetIdeDriveAsScsiInfoInNT(0, pIdInfo);
	}
	if (nRet)
	{
		// 调整字符串
		::AdjustString(pIdInfo->sSerialNumber, 20, iDiskSNLen);
		memcpy(szDiskSN, pIdInfo->sSerialNumber, ++iDiskSNLen);
	}
	else
	{
		szDiskSN = "";
		iDiskSNLen = 0;
	}

	delete pIdInfo;
	return (0 == nRet) ? false : true;

}

//
// Get the physical model number of the hard disk drive
//		szDiskModelNum[out]				--	Save the model number, its size must larger than 40.
//		iDiskModuleNumberLen[out]		--	Save the length of model number string, including the terminating null character.
//											If failed, the value is 0. If szDiskSN is NULL input, the value is -1.
// Return value:
//		true	--	success
//		false	--	failed
//
bool CComputerInfo::GetDiskModelNumber(char* szDiskModelNum, int& iDiskModuleNumberLen)
{
	if (NULL == szDiskModelNum || iDiskModuleNumberLen <= 0)
	{
		iDiskModuleNumberLen = -1;
		return false;
	}

	memset(szDiskModelNum, 0, iDiskModuleNumberLen);

	BOOL nRet;
	PIDINFO pIdInfo = new IDINFO;
	nRet = GetPhysicalDriveInfoInNT(0, pIdInfo);
	if (!nRet)
	{
		nRet = GetIdeDriveAsScsiInfoInNT(0, pIdInfo);
	}
	if (nRet)
	{
		// 调整字符串
		::AdjustString(pIdInfo->sModelNumber, 40, iDiskModuleNumberLen);
		memcpy(szDiskModelNum, pIdInfo->sModelNumber, ++iDiskModuleNumberLen);
	}
	else
	{
		szDiskModelNum = "";
		iDiskModuleNumberLen = 0;
	}

	delete pIdInfo;
	return (0 == nRet) ? false : true;
}

//
// Get the computer name
//		szComputerName[out]		--	Save the computer name, it is saved in TCHARS in memory.
//		iComputerNameLen[out]	--	Save the length of computer name string in bytes, including the terminating null character(2 bytes because the string is saved in TCHARS)..
//									The value is double the length of computer name in TCHARS. If failed, the value is 0. If szComputerName is NULL input, the value is -1.
// Return value:
//		true	--	success
//		false	--	failed
//
bool CComputerInfo::GetComputerNameSD(char* szComputerName, int& iComputerNameLen)
{
	if (NULL == szComputerName || iComputerNameLen <= 0)
	{
		iComputerNameLen = -1;
		return false;
	}

	memset(szComputerName, 0, iComputerNameLen);

	BOOL nRet;
	TCHAR szInfo[INFO_BUFFER_SIZE] = { 0 };
	DWORD dwSize = INFO_BUFFER_SIZE;

	nRet = GetComputerName(szInfo, &dwSize);
	if (nRet)
	{
		// dwSize = 实际字符数（不含 '\0'）
#ifdef UNICODE
		size_t needBytes = (dwSize + 1) * sizeof(wchar_t);
#else
		size_t needBytes = (dwSize + 1) * sizeof(char);
#endif

		if ((size_t)iComputerNameLen < needBytes)
		{
			iComputerNameLen = (int)needBytes;
			return false;
		}

		memcpy(szComputerName, szInfo, needBytes);
		iComputerNameLen = (int)needBytes;
	}

	else
	{
		// Failed, clear the buffer and counter
		iComputerNameLen = 0;
		szComputerName = "";
	}

	return (0 == nRet) ? false : true;
}
//
// Get the current volume serial number
//		szVolumeSN[out]		--	Save the volume serial number.
//		iVolumeSNLen[out]	--	Save the length of volume SN in bytes, including the terminating null character.
//								If successfully get the SN, the value is 9. If failed, the value is 0. If szVolumeSN is NULL input, the value is -1.
// Return value:
//		true	--	success
//		false	--	failed
//
bool CComputerInfo::GetVolumeSN(char* szVolumeSN, int& iVolumeSNLen)
{
	if (NULL == szVolumeSN || iVolumeSNLen <= 8)
	{
		iVolumeSNLen = -1;
		return false;
	}

	memset(szVolumeSN, 0, iVolumeSNLen);

	BOOL nRet;
	DWORD dwVolumeSN;
	nRet = GetVolumeInformation(NULL, NULL, 0, &dwVolumeSN, NULL, NULL, NULL, 0);
	if (nRet)
	{
		// success
		sprintf_s(szVolumeSN, iVolumeSNLen, "%08X", dwVolumeSN);
		iVolumeSNLen = 8;
		szVolumeSN[iVolumeSNLen] = '\0';
	}
	else
	{
		// failed, clear counter and buffer
		iVolumeSNLen = 0;
		szVolumeSN = "";
	}
	return (0 == nRet) ? false : true;
}
//
// Get the computer user name
//		szUsrName[out]		--	Save the computer user name, it is saved in TCHARS in memory.
//		iUsrNameLen[out]	--	Save the length of user name in bytes, including the terminating null character(2 bytes because the string is saved in TCHARS).
//								If failed, the value is 0. If szUsrName is NULL, the value is -1.
// Return value:
//		true	--	success
//		false	--	failed
//
bool CComputerInfo::GetUsrName(char* szUsrName, int& iUsrNameLen)
{
	if (szUsrName == NULL || iUsrNameLen <= 0)
	{
		iUsrNameLen = -1;
		return false;
	}

	BOOL nRet;
	TCHAR szInfo[INFO_BUFFER_SIZE] = { 0 };
	DWORD dwSize = INFO_BUFFER_SIZE; // 字符数

	nRet = GetUserName(szInfo, &dwSize);
	if (!nRet)
	{
		szUsrName[0] = '\0';
		iUsrNameLen = 0;
		return false;
	}

	// dwSize 已包含 '\0'
#ifdef UNICODE
	size_t needBytes = dwSize * sizeof(wchar_t);
#else
	size_t needBytes = dwSize * sizeof(char);
#endif

	if ((size_t)iUsrNameLen < needBytes)
	{
		iUsrNameLen = (int)needBytes;
		return false;
	}

	memcpy(szUsrName, szInfo, needBytes);
	iUsrNameLen = (int)needBytes;

	return true;
}

//
// Get the Mac Address, using IP Help API
//		szMacAddress[out]	--	Save the MAC address, the formant is "XX-XX-XX-XX-XX-XX"
//		iMacAddressLen[out]	--	Save the length of MAC address, including the terminating null character. Usually, the value is 18
//								If failed, the value is 0. If szMacAddress is NULL, the value is -1.
// Return value:
//		true	--	success
//		false	--	failed
//
bool CComputerInfo::GetMacAddress(char* szMacAddress, int& iMacAddressLen, bool isSplit)
{
	if (NULL == szMacAddress || iMacAddressLen < 17) { iMacAddressLen = 0; return false; }

	try
	{
		memset(szMacAddress, 0, iMacAddressLen);

		PIP_ADAPTER_INFO pAdapterInfo;
		PIP_ADAPTER_INFO pAdapter = NULL;
		DWORD dwRetVal = 0;
		pAdapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));
		ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);

		// Make an initial call to GetAdaptersInfo to get
		// the necessary size into the ulOutBufLen variable
		if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
		{
			free(pAdapterInfo);
			pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
		}

		if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR)
		{
			pAdapter = pAdapterInfo;
			for (; NULL != pAdapter; pAdapter = pAdapter->Next)
			{
				if (pAdapter->IpAddressList.IpAddress.String != "")
				{
					if (isSplit)
					{
						// Get the MAC Address
						sprintf_s(szMacAddress, iMacAddressLen, "%02X-%02X-%02X-%02X-%02X-%02X",
							pAdapter->Address[0], pAdapter->Address[1], pAdapter->Address[2],
							pAdapter->Address[3], pAdapter->Address[4], pAdapter->Address[5]);
						if (!strcmp(szMacAddress, "00-00-00-00-00-00"))
						{
							continue;
						}
					}
					else
					{
						// Get the MAC Address
						sprintf_s(szMacAddress, iMacAddressLen, "%02X%02X%02X%02X%02X%02X",
							pAdapter->Address[0], pAdapter->Address[1], pAdapter->Address[2],
							pAdapter->Address[3], pAdapter->Address[4], pAdapter->Address[5]);
						if (!strcmp(szMacAddress, "000000000000"))
						{
							continue;
						}
					}
					iMacAddressLen = strlen(szMacAddress);
					break;
				}
			}
		}
		else
		{
			free(pAdapterInfo);
			return false;
		}

		free(pAdapterInfo);
		return (iMacAddressLen > 0) ? true : false;
	}
	catch (...)
	{
		return false;
	}
}

bool CComputerInfo::GetIPAddress(char* szIP, int& nIPLen)
{
	if (NULL == szIP || nIPLen < 16) return false;

	try
	{
		memset(szIP, 0, nIPLen);

		PIP_ADAPTER_INFO pAdapterInfo;
		PIP_ADAPTER_INFO pAdapter = NULL;
		DWORD dwRetVal = 0;
		pAdapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));
		ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);

		// Make an initial call to GetAdaptersInfo to get
		// the necessary size into the ulOutBufLen variable
		if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
		{
			free(pAdapterInfo);
			pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
		}

		if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR)
		{
			pAdapter = pAdapterInfo;
			while (pAdapter)
			{
				if (pAdapter->IpAddressList.IpAddress.String != "")
				{
					// Get IP
					memcpy(szIP, pAdapter->IpAddressList.IpAddress.String, 16);
					nIPLen = strlen(szIP);
					if (nIPLen > 0 && strcmp(szIP, "0.0.0.0") != 0)
					{
						break;
					}
				}
				pAdapter = pAdapter->Next;
			}
		}
		else
		{
			free(pAdapterInfo);
			return false;
		}

		free(pAdapterInfo);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool CComputerInfo::GetOSInfo(char* szOSInfo, int& nOSInfoLen)
{
	if (NULL == szOSInfo || nOSInfoLen <= 0) return false;

	try
	{
		// Try calling GetVersionEx using the OSVERSIONINFOEX structure.If that fails, try using the OSVERSIONINFO structure.
#pragma warning(push)
#pragma warning(disable:4996)  // 禁用 GetVersionEx 被弃用的警告

		OSVERSIONINFOEXA osvi;
		ZeroMemory(&osvi, sizeof(OSVERSIONINFOEXA));
		osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);

		BOOL bOsVersionInfoEx;
		if (!(bOsVersionInfoEx = GetVersionEx((OSVERSIONINFO*)&osvi)))
		{
			osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
			if (!GetVersionEx((OSVERSIONINFO*)&osvi))
				return false;
		}

#pragma warning(pop)


		//Windows 7 6.1 
		//Windows Server 2008 R2 6.1 
		//Windows Server 2008 6.0 
		//Windows Vista 6.0 
		//Windows Server 2003 R2 5.2 
		//Windows Server 2003 5.2 
		//Windows XP 5.1 
		//Windows 2000 5.0 

		static const size_t buff_size = 128;
		char szOSInfoTmp[buff_size] = { 0 };
		switch (osvi.dwPlatformId)
		{
		case VER_PLATFORM_WIN32_NT:// Windows NT product family.
			if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0)
			{
				strcat_s(szOSInfoTmp, buff_size, "Microsoft Windows 2000 ");
			}
			else if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1)
			{
				strcat_s(szOSInfoTmp, buff_size, "Microsoft Windows XP ");
			}
			else if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2)
			{
				strcat_s(szOSInfoTmp, buff_size, "Microsoft Windows Server 2003 ");
			}
			else if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0)
			{
				strcat_s(szOSInfoTmp, buff_size, "Microsoft Windows Vista ");
			}
			else if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1)
			{
				strcat_s(szOSInfoTmp, buff_size, "Microsoft Windows 7 ");
			}
			else if (osvi.dwMajorVersion <= 4)
			{
				strcat_s(szOSInfoTmp, buff_size, "Microsoft Windows NT ");
			}

			break;
		case VER_PLATFORM_WIN32_WINDOWS:// Windows Me/98/95.

			if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
			{
				strcat_s(szOSInfoTmp, buff_size, "Microsoft Windows 95 ");
				if (osvi.szCSDVersion[1] == 'C' || osvi.szCSDVersion[1] == 'B')
				{
					strcat_s(szOSInfoTmp, buff_size, "OSR2 ");
				}
			}
			else if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10)
			{
				strcat_s(szOSInfoTmp, buff_size, "Microsoft Windows 98 ");
				if (osvi.szCSDVersion[1] == 'A')
				{
					strcat_s(szOSInfoTmp, buff_size, "SE ");
				}
			}
			else if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90)
			{
				strcat_s(szOSInfoTmp, buff_size, "Microsoft Windows Millennium Edition ");
			}
			break;
		case VER_PLATFORM_WIN32s:
			strcat_s(szOSInfoTmp, buff_size, "Microsoft Win32s ");
			break;
		}
		char szVersion[buff_size] = { 0 };
		sprintf_s(szVersion, buff_size, "(%d.%d.%d) ", osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);
		strcat_s(szOSInfoTmp, buff_size, szVersion);
		strcat_s(szOSInfoTmp, buff_size, osvi.szCSDVersion);

		int nLen = strlen(szOSInfoTmp);
		if (0 != nLen && nOSInfoLen > nLen)
		{
			memset(szOSInfo, 0, nOSInfoLen);
			strncpy_s(szOSInfo, nOSInfoLen, szOSInfoTmp, nLen);
			nOSInfoLen = nLen;
		}
		else return false;

		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool CComputerInfo::GetMemInfo(
	uint64_t& nPhyTotalMB,
	uint64_t& nPhyAvailableMB,
	uint64_t& nVirtualTotalMB,
	uint64_t& nVirtualAvailableMB)
{
	// MEMORYSTATUSEX 用于获取系统内存信息（64 位安全）
	MEMORYSTATUSEX memStatus = {};
	memStatus.dwLength = sizeof(memStatus);

	// 调用 WinAPI 获取内存信息
	if (!GlobalMemoryStatusEx(&memStatus))
	{
		// API 调用失败
		return false;
	}

	// 将字节(Byte)转换为 MB（1024 * 1024）
	nPhyTotalMB = memStatus.ullTotalPhys / (1024ULL * 1024ULL);
	nPhyAvailableMB = memStatus.ullAvailPhys / (1024ULL * 1024ULL);
	nVirtualTotalMB = memStatus.ullTotalVirtual / (1024ULL * 1024ULL);
	nVirtualAvailableMB = memStatus.ullAvailVirtual / (1024ULL * 1024ULL);

	return true;
}

//用来存储信息
DWORD deax;
DWORD debx;
DWORD decx;
DWORD dedx;

void ExeCPUID(DWORD veax)//初始化CPU
{
	__asm
	{
		mov eax, veax
		cpuid
		mov deax, eax
		mov debx, ebx
		mov decx, ecx
		mov dedx, edx
	}
}

bool CComputerInfo::GetCpuInfo(char* szCpuInfo, int& nCpuInfo, int& nNumOfProcessor)
{
	if (szCpuInfo == NULL || nCpuInfo <= 0)
		return false;

	const DWORD baseId = 0x80000002;
	char CPUType[49] = { 0 };

	for (DWORD t = 0; t < 3; ++t)
	{
		ExeCPUID(baseId + t);
		memcpy(CPUType + 16 * t + 0, &deax, 4);
		memcpy(CPUType + 16 * t + 4, &debx, 4);
		memcpy(CPUType + 16 * t + 8, &decx, 4);
		memcpy(CPUType + 16 * t + 12, &dedx, 4);
	}

	std::string strCpuInfo(CPUType);
	size_t needLen = strCpuInfo.size() + 1; // 包含 '\0'

	if ((size_t)nCpuInfo < needLen)
	{
		nCpuInfo = (int)needLen; // 告知调用方需要的大小
		return false;
	}

	strcpy_s(szCpuInfo, nCpuInfo, strCpuInfo.c_str());
	nCpuInfo = (int)(needLen - 1); // 返回字符串长度（不含 '\0'）

	SYSTEM_INFO si;
	GetSystemInfo(&si);
	nNumOfProcessor = (int)si.dwNumberOfProcessors;

	return true;
}

bool CComputerInfo::GetCpuID(char* szCpuID, int& nCpuIDLen)
{
	// 参数检查：调用方必须提供缓冲区
	if (szCpuID == NULL || nCpuIDLen <= 0)
		return false;

	static const size_t BUFF_SIZE = 0x100;

	char szVendorId[BUFF_SIZE] = { 0 };
	char buff[BUFF_SIZE] = { 0 };

	// ----------------------------
	// Step 1: CPUID(0) 读取 CPU Vendor ID
	// EBX + EDX + ECX = 12 字节厂商字符串
	// ----------------------------
	__asm
	{
		xor eax, eax
		cpuid
		mov dword ptr szVendorId, ebx
		mov dword ptr szVendorId[4], edx
		mov dword ptr szVendorId[8], ecx
	}

	// ----------------------------
	// Step 2: CPUID(1) 读取 EAX / EDX（特征 ID）
	// ----------------------------
	__asm
	{
		mov eax, 01h
		xor edx, edx
		cpuid
		mov dedx, edx
		mov deax, eax
	}

	// 拼接 "-EDX-EAX"
	sprintf_s(buff, BUFF_SIZE, "-%08X-%08X", dedx, deax);
	strncat_s(szVendorId, BUFF_SIZE, buff, _TRUNCATE);

	// ----------------------------
	// Step 3: CPUID(3) 读取序列号相关寄存器（旧 CPU 才支持）
	// ----------------------------
	__asm
	{
		mov eax, 03h
		xor ecx, ecx
		xor edx, edx
		cpuid
		mov dedx, edx
		mov decx, ecx
	}

	// 拼接 "-EDX-ECX"
	sprintf_s(buff, BUFF_SIZE, "-%08X-%08X", dedx, decx);
	strncat_s(szVendorId, BUFF_SIZE, buff, _TRUNCATE);

	// ----------------------------
	// Step 4: 拷贝到输出缓冲区
	// ----------------------------
	std::string strCpuId(szVendorId);
	size_t needLen = strCpuId.size() + 1; // 包含 '\0'

	if ((size_t)nCpuIDLen < needLen)
	{
		// 告诉调用方需要的最小长度
		nCpuIDLen = (int)needLen;
		return false;
	}

	strcpy_s(szCpuID, nCpuIDLen, strCpuId.c_str());
	nCpuIDLen = (int)(needLen - 1); // 返回字符串长度（不含 '\0'）

	return true;
}

bool CComputerInfo::GetIECookiePath(char* path)
{
	if (NULL == path) return false;
	HKEY hKey = NULL;
	long lRet = RegOpenKey(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders"), &hKey);

	if (ERROR_SUCCESS != lRet) return 0;

	char buf[1024] = { 0 };
	DWORD nBufSize = sizeof(buf);
	DWORD dwType = 0;
	lRet = RegQueryValueExA(hKey, "Cookies", NULL, &dwType, (unsigned char*)buf, &nBufSize);
	RegCloseKey(hKey);

	if (ERROR_SUCCESS != lRet) return false;

	char buffer[MAX_PATH];
	ExpandEnvironmentStringsA(buf, buffer, MAX_PATH);
	sprintf_s(path, MAX_PATH, "%s\\", buffer);
	return true;
}

