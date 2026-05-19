#include "stdafx.h"
#include "SafeBase64.h"

static const std::string BASE64_CHAR_SET("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");

static inline bool is_base64(unsigned char c)
{
	return (isalnum(c) || (c == '+') || (c == '/'));
}


CSafeBase64::CSafeBase64(void)
{
}

CSafeBase64::~CSafeBase64(void)
{
}

std::string CSafeBase64::Encode(const std::string &input)
{
    std::string output;
	
	unsigned char inputBytes[3];
	unsigned char outputBytes[4];
	
	int len = input.size();
	int pos = 0;
	
    int i = 0;
	while (len--)
	{
		inputBytes[i++] = input[pos++];
		if (i == 3)
		{
			outputBytes[0] = (inputBytes[0] & 0xfc) >> 2;
			outputBytes[1] = ((inputBytes[0] & 0x03)<<4) + ((inputBytes[1] & 0xf0) >> 4);
			outputBytes[2] = ((inputBytes[1] & 0x0f)<<2) + ((inputBytes[2] & 0xc0) >> 6);
			outputBytes[3] = inputBytes[2] & 0x3f;

			for (i=0; i<4; i++)
			{
				output += BASE64_CHAR_SET[outputBytes[i]];
			}
			
			i = 0;
		}
	}

	if (i)
	{
		for (int j=i; j<3; j++)
		{
			inputBytes[j] = '\0';
		}

		outputBytes[0] = (inputBytes[0] & 0xfc) >> 2;
		outputBytes[1] = ((inputBytes[0] & 0x03)<<4) + ((inputBytes[1] & 0xf0) >> 4);
		outputBytes[2] = ((inputBytes[1] & 0x0f)<<2) + ((inputBytes[2] & 0xc0) >> 6);
		outputBytes[3] = inputBytes[2] & 0x3f;

		for(int j=0; j<i+1; j++)
		{
			output += BASE64_CHAR_SET[outputBytes[j]];
		}

		while (i++<3)
		{
			output += '=';
		}
	}

	return output;
}

std::string CSafeBase64::Decode(const std::string &input)
{
	std::string output;
	
	unsigned char inputBytes[4], outputBytes[3];
	int len = (int)input.size();

    int i = 0;
    int pos = 0;
	while (len-- && (input[pos]!='=') && is_base64(input[pos]))
	{
		inputBytes[i++] = input[pos];
		pos++;
		if (i==4)
		{
			for (i=0; i<4; i++)
			{
				inputBytes[i] = (unsigned char)BASE64_CHAR_SET.find(inputBytes[i]);
			}

			outputBytes[0] = (inputBytes[0]<<2) + ((inputBytes[1] & 0x30) >> 4);
			outputBytes[1] = ((inputBytes[1] & 0xf)<<4) + ((inputBytes[2] & 0x3c) >> 2);
			outputBytes[2] = ((inputBytes[2] & 0x3)<<6) + inputBytes[3];

			for (i=0; i<3; i++)
			{
				output += outputBytes[i];
			}
			
			i = 0;
		}
	}

	if (i)
	{
		for(int j=i; j<4; j++)
		{
			inputBytes[j] = 0;
		}
		
		for (int j=0; j<4; j++)
		{
			inputBytes[j] = (unsigned char)BASE64_CHAR_SET.find(inputBytes[j]);
		}
		
		outputBytes[0] = (inputBytes[0]<<2) + ((inputBytes[1] & 0x30) >> 4);
		outputBytes[1] = ((inputBytes[1] & 0xf)<<4) + ((inputBytes[2] & 0x3c) >> 2);
		outputBytes[2] = ((inputBytes[2] & 0x3)<<6) + inputBytes[3];

		for(int j=0; (j<i-1); j++)
		{
			output += outputBytes[j];
		}
	}

	return output;
}
    
    