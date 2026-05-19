#ifndef _COH_URL_CODE_H_
#define _COH_URL_CODE_H_

#include <string>
#include <cstdio>
#include <cstdlib>
#include <cctype> // for isalnum
#include <sstream>
#include <iomanip>

using std::string;

class UrlEncoder
{
public:
	static std::string encode(const char *pValue)
	{
		size_t len = strlen(pValue);
		std::string ret = "";
		size_t i=0;
		for(i=0;i<len;i++)
		{
			if(isOrdinaryChar(pValue[i]))
			{
				ret += pValue[i];
			}
			else if(pValue[i] == ' ')
			{
				ret += "+";
			}
			else
			{
				char tmp[6]={0};
				sprintf(tmp,"%%%x",(unsigned char)pValue[i]);
				ret+= tmp;
			}
		}
		return ret;
	}

	static std::string encode_go_down(const char *pValue)
	{
		std::string ret;
		unsigned char ch;

		while ((ch = (unsigned char)*pValue++) != 0)
		{
			if (isOrdinaryChar(ch))
			{
				ret += ch;
			}
			else if (ch == ' ')
			{
				ret += "+";
			}
			else
			{
				char tmp[4] = {0}; // 为了放置 %XX 编码格式，预留4字节空间
				sprintf(tmp, "%%%02X", ch); // 逐个字节进行 URL 编码
				ret += tmp;
			}
		}

		return ret;
	}

	static std::string Encode(const std::string& input)
	{
		size_t len = input.size();
		std::string output;
		for (size_t i = 0; i < len; i++)
		{
			if (isOrdinaryChar(input[i]))
			{
				output += input[i];
			}
			else if (input[i] == ' ')
			{
				output += "+";
			}
			else
			{
				char tmp[6] = { 0 };
				_snprintf_s(tmp, 6, _TRUNCATE, "%%%x", (unsigned char)input[i]);
				output += tmp;
			}
		}
		return output;
	}

	// 将字符串转为 UTF-8 的百分号编码
	static std::string StarUrlEncode(const std::string& value) {
		std::ostringstream encoded;
		for (std::string::const_iterator it = value.begin(); it != value.end(); ++it) {
			unsigned char c = static_cast<unsigned char>(*it);
			// 对 ASCII 字符无需编码
			if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
				encoded << c;
			} else {
				// 对非 ASCII 字符逐字节处理
				encoded << '%' << std::uppercase
						<< std::setw(2) << std::setfill('0')
						<< std::hex << int(c);
			}
    }
    return encoded.str();
}

private:
	static bool isOrdinaryChar(char c)
	{
		char ch = tolower(c);
		if((ch>='a'&&ch<='z')||(ch>='0'&&ch<='9'))
		{
			return true;
		}
		return false;
	}
};

class URLDecoder
{
public:
	static std::string decode(const char *pValue) {
		size_t len = strlen(pValue);
		std::string ret = "";
		size_t i=0;
		for(i=0;i<len;i++) {
			if(pValue[i] == '+') {
				ret = ret + " ";
			}else if(pValue[i] == '%') {
				char tmp[4];
				char hex[4];			
				hex[0] = pValue[++i];
				hex[1] = pValue[++i];
				hex[2] = '\0';		
				sprintf(tmp,"%c",convertToDec(hex));
				ret = ret + tmp;
			}else {
				ret = ret + pValue[i];
			}
		}
		return ret;
	}

	static int convertToDec(const char* hex) {
		char buff[12];
		sprintf(buff,"%s",hex);
		int ret = 0;
		size_t len = strlen(buff);
		size_t i=0;
		size_t j=0;
		for(i=0;i<len;i++) {
			char tmp[4];
			tmp[0] = buff[i];
			tmp[1] = '\0';
			getAsDec(tmp);
			int tmp_i = atoi(tmp);
			int rs = 1;
			for(j=i;j<(len-1);j++) {
				rs *= 16;
			}
			ret += (rs * tmp_i);
		}
		return ret;
	}

	static void getAsDec(char* hex) {
		char tmp = tolower(hex[0]);
		if(tmp == 'a') {
			strcpy(hex,"10");
		}else if(tmp == 'b') {
			strcpy(hex,"11");
		}else if(tmp == 'c') {
			strcpy(hex,"12");
		}else if(tmp == 'd') {
			strcpy(hex,"13");
		}else if(tmp == 'e') {
			strcpy(hex,"14");
		}else if(tmp == 'f') {
			strcpy(hex,"15");
		}else if(tmp == 'g') {
			strcpy(hex,"16");
		}
	}

};
#endif

