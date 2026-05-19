#pragma once

#include <string>

using std::string;

class CBase64Coding
{ 
	CBase64Coding(const CBase64Coding&);
	CBase64Coding& operator=(const CBase64Coding&);
private:
	static const string CBase64Coding::Base64Table;
	static const string::size_type CBase64Coding::DecodeTable[];
public:
	CBase64Coding() { ; }
	virtual ~CBase64Coding() { ; }

	/**
	* @function	对给定的字符串数组进行Basd64编码
	* @param		source	需要编码的字符串
	* @param		len		字符串的长度
	* @retrun	编码后的字符串
	*/
	static string Encode(const char * source,int len);
	static bool Encode(const char* souce,int srclen,char* dest,int destlen);

	/**
	* @function 对给定的Base64编码进行解码
	* @param		data	需要解码的Base64字符串
	* @return	解码后的结果
	*/
	static string Decode(const string& data);
	static int Decode(const string& source,char* dest,int destlen);
};

