#pragma once

#include <string.h>
#include <stdlib.h>

#ifndef min
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#endif

class SodaEncrypt
{
public:
	static bool Encrypt(unsigned char *pIn, unsigned int nInLen, const unsigned char *pKey, unsigned char *pOut, unsigned int *nOutLen)
	{
		if(NULL == pIn || NULL == pOut || NULL == pKey) return false;
		if(0 == nInLen) return true;

		*nOutLen = 0;

		// 产生随机头
		unsigned char cb1[16] = { 0 };
		unsigned int nFill = 8 + (rand() % 8);
		cb1[0] = (unsigned char)(nFill);
		for (unsigned char n = 1; n < nFill - 1; n ++)
			cb1[n] = (unsigned char)(rand() % 256);
		cb1[nFill-1] = cb1[0]/2;

		unsigned int nRead = 0;
		unsigned int nDealed = 0;
		unsigned char cb2[sizeof(cb1)];
		for ( ; nFill || nInLen; )
		{	
			// 读取nRead个数据
			nRead = sizeof(cb1) - nFill;
			nRead = min(nRead, nInLen);
			memcpy(&cb1[nFill], &pIn[nDealed], nRead);
			//	加密
			nFill += nRead;
			memset(cb2, 0, sizeof(cb2));
			for (unsigned int n = 0; n < nFill; n ++)		//	在这里 nFill 绝对是 15(完全能)
				cb2[n] = cb1[n] ^ pKey[n];
			//	输出结果
			memcpy(&pOut[*nOutLen], &cb2[0], nFill);
			//  下次
			nInLen -= nRead;
			nDealed += nRead;
			*nOutLen += nFill;
			nFill = 0;
			nRead = 0;
		};

		//	加密后的长度 = nInLen + 第一次得到的nFill[8--15]个字节,{unsigned int nFill = 8 + (rand() % 8);}
		return true;
	}
public:
	static bool Decrypt(unsigned char *pIn, unsigned int nInLen, const unsigned char *pKey, unsigned char *pOut, unsigned int *nOutLen)
	{
		*nOutLen = 0;

		unsigned char cb1[16];
		memset(cb1, 0, sizeof(cb1));
		unsigned int nDealed = 0;			//	已经处理了多少个字节
		unsigned int nFill = 0;
		unsigned int nRead = 0;
		unsigned char cb2[sizeof(cb1)];
		memset(cb2, 0, sizeof(cb2));

		//	处理加密头
		for ( ; nInLen > 0; )
		{
			// 读取
			nFill = 0;
			nRead = sizeof(cb1);
			nRead = min(nRead, nInLen);
			memcpy(&cb1[nFill], &pIn[nDealed], nRead);	
			nInLen -= nRead;
			nFill += nRead;
			nDealed += nRead;
			// 解密
			memset(cb2, 0, sizeof(cb2));
			for (unsigned int n = 0; n < nFill; n ++)
				cb2[n] = cb1[n] ^ pKey[n];
			if (cb2[0] < 8 || cb2[0] > 15 || 
				cb2[0] > nFill || cb2[cb2[0]-1] != cb2[0]/2)   // 判断是否[0] ？= 加密时设置的[cb[0]/2]
				return false;
			// 输出
			nFill -= cb2[0];
			memcpy(&pOut[*nOutLen], &cb2[cb2[0]], nFill);	
			*nOutLen += nFill;
			nFill = 0;	
			nRead = 0;
			break;
		};

		for ( ; nInLen > 0; )
		{
			// 读取
			nFill = 0;
			nRead = sizeof(cb1);
			nRead = min(nRead, nInLen);
			memcpy(&cb1[nFill], &pIn[nDealed], nRead);
			nFill += nRead;
			nInLen -= nRead;
			nDealed += nRead;
			nRead = 0;
			// 解密
			memset(cb2, 0, sizeof(cb2));
			for (unsigned int n = 0; n < nFill; n ++)
				cb2[n] = cb1[n] ^ pKey[n];
			// 输出
			memcpy(&pOut[*nOutLen], &cb2[0], nFill);
			*nOutLen += nFill;
			nFill = 0;
		};

		// 解密后的长度就是加密前真实数据的长度。
		return true;
	}
public:
	// 稳定版本,没有随机数,相同内部加密的结果必定相同
	static bool StableEncrypt(unsigned char *pIn, unsigned int nInLen, const unsigned char *pKey, unsigned char *pOut, unsigned int *nOutLen)
	{
		if(NULL == pIn || NULL == pOut || NULL == pKey) return false;
		if(0 == nInLen) return true;

		*nOutLen = 0;

		unsigned char cb1[16] = { 0 };
		unsigned int nFill = 9;								// 固定随机头为9个字节
		cb1[0] = (unsigned char)(nFill);
		for (unsigned char n = 1; n < nFill - 1; n ++)
			cb1[n] = (unsigned char)('z');						// 固定填充字节为'z'
		cb1[nFill-1] = cb1[0]/2;

		unsigned int nRead = 0;
		unsigned int nDealed = 0;
		unsigned char cb2[sizeof(cb1)];
		for ( ; nFill || nInLen; )
		{	
			// 读取nRead个数据
			nRead = sizeof(cb1) - nFill;
			nRead = min(nRead, nInLen);
			memcpy(&cb1[nFill], &pIn[nDealed], nRead);
			//	加密
			nFill += nRead;
			memset(cb2, 0, sizeof(cb2));
			for (unsigned int n = 0; n < nFill; n ++)		//	在这里 nFill 绝对是 15(完全能)
				cb2[n] = cb1[n] ^ pKey[n];
			//	输出结果
			memcpy(&pOut[*nOutLen], &cb2[0], nFill);
			//  下次
			nInLen -= nRead;
			nDealed += nRead;
			*nOutLen += nFill;
			nFill = 0;
			nRead = 0;
		};

		//	加密后的长度 = nInLen + 第一次得到的nFill[8--15]个字节,{unsigned int nFill = 8 + 13;}
		return true;
	}
public:
	static bool StableDecrypt(unsigned char *pIn, unsigned int nInLen, const unsigned char *pKey, unsigned char *pOut, unsigned int *nOutLen)
	{
		return Decrypt(pIn,nInLen,pKey,pOut,nOutLen);
	}
public:
	static const unsigned char* GetConstantKey()
	{
		static const unsigned char bEncryptKey[16] = {0x15, 0x23, 0x3D, 0x47, 0x5E, 0xD6, 0xA7, 0xFE, 0x90, 0xA5, 0xB8, 0xE1, 0xF2, 0x10};
		return bEncryptKey;
	}
};
