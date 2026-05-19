#include "stdafx.h"
#include <set>
#include <algorithm>
#include <regex>
#include <string.h>
#include <time.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include "GunionCrypto.h"
#include "SafeBase64.h"
#include "UrlCode.h"

bool GunionCrypto::Decrypt3DesBase64(
	const std::string& base64Cipher,
	const std::string& key,
	std::string& outPlain)
{
	if (base64Cipher.empty() || key.empty())
		return false;

	static const unsigned char IV[8] = { 0 };

	std::string cipher = CSafeBase64::Decode(base64Cipher);
	if (cipher.empty())
		return false;

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (!ctx)
		return false;

	bool ok = false;
	do
	{
		if (!EVP_DecryptInit(
			ctx,
			EVP_des_ede3_ecb(),
			(const unsigned char*)key.data(),
			IV))
			break;

		std::vector<unsigned char> buffer(cipher.size() * 2);
		int len1 = 0;

		if (!EVP_DecryptUpdate(
			ctx,
			buffer.data(),
			&len1,
			(const unsigned char*)cipher.data(),
			cipher.size()))
			break;

		int len2 = 0;
		if (!EVP_DecryptFinal(
			ctx,
			buffer.data() + len1,
			&len2))
			break;

		outPlain.assign(
			(char*)buffer.data(),
			len1 + len2);

		ok = true;
	} while (false);

	EVP_CIPHER_CTX_free(ctx);
	return ok;
}

bool GunionCrypto::EncryptWegameInitParams3DesBase64(
	const std::string& deviceIdServer,
	const std::string& mac,
	const std::string& windowsDeviceId,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建加密明文参数
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["device_id_server"] = deviceIdServer;
	params["mac"] = mac;
	params["windows_device_id"] = windowsDeviceId;

	std::string plain;
	bool first = true;
	for (std::map<std::string, std::string>::const_iterator it = params.begin();
		it != params.end(); ++it)
	{
		if (!first)
			plain += '&';

		plain += it->first;
		plain += '=';
		plain += UrlEncoder::Encode(it->second);
		first = false;
	}

	if (plain.empty())
		return false;

	// ------------------------------------------------------------
	// 2. 3DES(EDE3/ECB) 加密
	// ------------------------------------------------------------
	static const unsigned char IV[8] = { 0 };

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (!ctx)
		return false;

	bool ok = false;
	do
	{
		if (!EVP_EncryptInit(ctx,EVP_des_ede3_ecb(),(const unsigned char*)randKey.data(),IV))
			break;

		std::vector<unsigned char> buffer(plain.size() * 2);
		int len1 = 0;

		if (!EVP_EncryptUpdate(ctx,buffer.data(),&len1,(const unsigned char*)plain.data(),plain.size()))
			break;

		int len2 = 0;
		if (!EVP_EncryptFinal(ctx,buffer.data() + len1,&len2))
			break;

		std::string cipher((char*)buffer.data(),len1 + len2);

		// --------------------------------------------------------
		// 3. Base64 编码（保持旧逻辑）
		// --------------------------------------------------------
		outEncryptedParams = CSafeBase64::Encode(cipher);
		ok = true;

	} while (false);

	EVP_CIPHER_CTX_free(ctx);
	return ok;
}

bool GunionCrypto::CalcWegameInitSignatureMd5(
	const std::string& deviceIdServer,
	const std::string& mac,
	const std::string& windowsDeviceId,
	const std::string& randKey,
	std::string& outSignature)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建签名参数（与旧实现一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["device_id_server"] = deviceIdServer;
	params["mac"] = mac;
	params["windows_device_id"] = windowsDeviceId;

	// ------------------------------------------------------------
	// 2. 排序并拼接 key=value
	// ------------------------------------------------------------
	std::set<std::string> sorted;
	for (std::map<std::string, std::string>::const_iterator it = params.begin();
		it != params.end(); ++it)
	{
		sorted.insert(it->first + "=" + it->second);
	}

	std::string raw;
	bool first = true;
	for (std::set<std::string>::const_iterator it = sorted.begin();
		it != sorted.end(); ++it)
	{
		if (!first)
			raw += '&';

		raw += *it;
		first = false;
	}

	// ------------------------------------------------------------
	// 3. 拼接 randKey + 转小写
	// ------------------------------------------------------------
	raw += randKey;
	std::transform(raw.begin(), raw.end(), raw.begin(), ::tolower);

	// ------------------------------------------------------------
	// 4. MD5 计算（输出 32 位大写 HEX）
	// ------------------------------------------------------------
	unsigned char md5Bytes[16] = { 0 };
	MD5(
		(const unsigned char*)raw.c_str(),
		raw.size(),
		md5Bytes);

	char buf[33] = { 0 };
	for (int i = 0; i < 16; ++i)
	{
		_snprintf_s(
			buf + i * 2,
			3,
			_TRUNCATE,
			"%02X",
			md5Bytes[i]);
	}

	outSignature.assign(buf);
	return true;
}

bool GunionCrypto::EncryptWegameLoginParams3DesBase64(
	const std::string& deviceId,
	const std::string& deviceIdServer,
	const std::string& thirdToken,
	const std::string& thirdUserId,
	const std::string& isLimited,
	const std::string& companyId,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建明文参数（字段必须与旧实现完全一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["deviceid"] = deviceId;
	params["device_id_server"] = deviceIdServer;
	params["thirdToken"] = thirdToken;
	params["thirdUserId"] = thirdUserId;
	params["islimited"] = isLimited;
	params["companyid"] = companyId;

	std::string input;
	bool first = true;
	for (const auto& kv : params)
	{
		if (!first)
			input += '&';

		input += kv.first;
		input += '=';
		input += UrlEncoder::Encode(kv.second);
		first = false;
	}

	// ------------------------------------------------------------
	// 2. 3DES(EDE3/ECB) 加密（保持旧 buffer 规则）
	// ------------------------------------------------------------
	static const unsigned char IV[8] = { 0 };

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (!ctx)
		return false;

	bool ok = false;
	do
	{
		if (!EVP_EncryptInit(
			ctx,
			EVP_des_ede3_ecb(),
			(const unsigned char*)randKey.data(),
			IV))
			break;

		int buffLen = input.size() <= 4 ? 9 : 2 * input.size();
		std::vector<unsigned char> buffer(buffLen);
		memset(buffer.data(), 0, buffLen);

		int len1 = 0;
		if (!EVP_EncryptUpdate(
			ctx,
			buffer.data(),
			&len1,
			(const unsigned char*)input.data(),
			input.size()))
			break;

		int len2 = 0;
		if (!EVP_EncryptFinal(
			ctx,
			buffer.data() + len1,
			&len2))
			break;

		const int outputLen = len1 + len2;
		std::string cipher((char*)buffer.data(), outputLen);

		// Base64（与旧逻辑一致）
		outEncryptedParams = CSafeBase64::Encode(cipher);
		ok = true;

	} while (false);

	EVP_CIPHER_CTX_free(ctx);
	return ok;
}

bool GunionCrypto::CalcWegameLoginSignatureMd5(
	const std::string& deviceId,
	const std::string& deviceIdServer,
	const std::string& thirdToken,
	const std::string& thirdUserId,
	const std::string& isLimited,
	const std::string& companyId,
	const std::string& randKey,
	std::string& outSignature)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建签名参数（与旧实现完全一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["deviceid"] = deviceId;
	params["device_id_server"] = deviceIdServer;
	params["thirdToken"] = thirdToken;
	params["thirdUserId"] = thirdUserId;
	params["islimited"] = isLimited;
	params["companyid"] = companyId;

	// 排序 key=value
	std::set<std::string> sorted;
	for (const auto& kv : params)
	{
		sorted.insert(kv.first + "=" + kv.second);
	}

	// 拼接 raw
	std::string raw;
	bool first = true;
	for (const auto& item : sorted)
	{
		if (!first)
			raw += '&';
		raw += item;
		first = false;
	}

	// 拼接 randKey
	raw += randKey;

	// tolower（保持旧行为）
	std::transform(raw.begin(), raw.end(), raw.begin(), ::tolower);

	// MD5
	unsigned char md5Bytes[16] = { 0 };
	MD5((const unsigned char*)raw.c_str(), raw.size(), md5Bytes);

	char buf[33] = { 0 };
	for (int i = 0; i < 16; ++i)
	{
		_snprintf_s(buf + i * 2, 3, _TRUNCATE, "%02X", md5Bytes[i]);
	}

	outSignature.assign(buf);
	return true;
}

bool GunionCrypto::EncryptWegameSmsSendParams3DesBase64(
	const std::string& phone,
	const std::string& type,
	const std::string& supportPic,
	const std::string& voiceMsg,
	const std::string& smsNew,
	const std::string& deviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建明文参数（必须与旧 CGAuthClient 完全一致）
	//    注意：旧代码使用 map → key 字典序
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["phone"] = phone;
	params["type"] = type;
	params["supportPic"] = supportPic;
	params["voiceMsg"] = voiceMsg;
	params["sms_new"] = smsNew;
	params["deviceid"] = deviceId;
	params["device_id_server"] = deviceIdServer;

	std::string input;
	bool first = true;
	for (std::map<std::string, std::string>::const_iterator it = params.begin();
		it != params.end(); ++it)
	{
		if (!first)
			input += '&';

		// 必须 URL Encode value（与旧逻辑一致）
		input += it->first + "=" + UrlEncoder::Encode(it->second);
		first = false;
	}

	// ------------------------------------------------------------
	// 2. 3DES(EDE3/ECB) 加密
	// ------------------------------------------------------------
	static const unsigned char ENCRYPT_IV[8] = { 0 };

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (!ctx)
		return false;

	bool ret = false;

	if (EVP_EncryptInit(
		ctx,
		EVP_des_ede3_ecb(),
		reinterpret_cast<const unsigned char*>(randKey.data()),
		ENCRYPT_IV))
	{
		int outLen = 0;
		int bufLen = input.size() <= 4 ? 9 : static_cast<int>(input.size() * 2);

		std::vector<unsigned char> outBuf(bufLen, 0);

		if (EVP_EncryptUpdate(
			ctx,
			outBuf.data(),
			&outLen,
			reinterpret_cast<const unsigned char*>(input.data()),
			static_cast<int>(input.size())))
		{
			int tmpLen = 0;
			if (EVP_EncryptFinal(ctx, outBuf.data() + outLen, &tmpLen))
			{
				outLen += tmpLen;

				std::string cipher(reinterpret_cast<char*>(outBuf.data()), outLen);
				outEncryptedParams = CSafeBase64::Encode(cipher);
				ret = true;
			}
		}
	}

	EVP_CIPHER_CTX_free(ctx);
	return ret;
}

bool GunionCrypto::CalcWegameSmsSendSignatureMd5(
	const std::string& phone,
	const std::string& type,
	const std::string& supportPic,
	const std::string& voiceMsg,
	const std::string& smsNew,
	const std::string& deviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建签名参数（与旧 CGAuthClient 完全一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["phone"] = phone;
	params["type"] = type;
	params["supportPic"] = supportPic;
	params["voiceMsg"] = voiceMsg;
	params["sms_new"] = smsNew;
	params["deviceid"] = deviceId;
	params["device_id_server"] = deviceIdServer;

	// ------------------------------------------------------------
	// 2. 按 key=value 排序（旧代码用 set<string>）
	// ------------------------------------------------------------
	std::set<std::string> sorted;
	for (std::map<std::string, std::string>::const_iterator it = params.begin();
		it != params.end(); ++it)
	{
		sorted.insert(it->first + "=" + it->second);
	}

	std::string raw;
	bool first = true;
	for (std::set<std::string>::const_iterator it = sorted.begin();
		it != sorted.end(); ++it)
	{
		if (!first)
			raw += '&';

		raw += *it;
		first = false;
	}

	// 3. 拼接 randKey
	raw += randKey;

	// 4. 转小写
	std::transform(raw.begin(), raw.end(), raw.begin(), ::tolower);

	// ------------------------------------------------------------
	// 5. MD5 计算
	// ------------------------------------------------------------
	unsigned char md5Bytes[16] = { 0 };
	MD5(reinterpret_cast<const unsigned char*>(raw.c_str()),
		raw.size(),
		md5Bytes);

	char hex[33] = { 0 };
	for (int i = 0; i < 16; ++i)
	{
		_snprintf_s(hex + i * 2, 3, _TRUNCATE, "%02X", md5Bytes[i]);
	}
	hex[32] = '\0';

	outSignature.assign(hex);
	return true;
}

bool GunionCrypto::EncryptWegamePicSmsSendParams3DesBase64(
	const std::string& phone,
	const std::string& type,
	const std::string& checkCodeGuid,
	const std::string& checkCode,
	const std::string& outInfo,
	const std::string& voiceMsg,
	const std::string& smsNew,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建明文参数（必须与旧 CGAuthClient 完全一致）
	//    旧实现：map → key 字典序
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["phone"] = phone;
	params["type"] = type;
	params["checkCodeGuid"] = checkCodeGuid;
	params["checkCode"] = checkCode;
	params["outInfo"] = outInfo;
	params["voiceMsg"] = voiceMsg;
	params["sms_new"] = smsNew;
	params["device_id_server"] = deviceIdServer;

	std::string input;
	bool first = true;
	for (const auto& kv : params)
	{
		if (!first)
			input += '&';

		// 与旧逻辑一致：仅 value 做 URL Encode
		input += kv.first;
		input += '=';
		input += UrlEncoder::Encode(kv.second);
		first = false;
	}

	// ------------------------------------------------------------
	// 2. 3DES(EDE3/ECB) 加密（保持旧 buffer 规则）
	// ------------------------------------------------------------
	static const unsigned char IV[8] = { 0 };

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (!ctx)
		return false;

	bool ok = false;
	do
	{
		if (!EVP_EncryptInit(
			ctx,
			EVP_des_ede3_ecb(),
			reinterpret_cast<const unsigned char*>(randKey.data()),
			IV))
			break;

		int bufLen = input.size() <= 4
			? 9
			: static_cast<int>(input.size() * 2);

		std::vector<unsigned char> buffer(bufLen, 0);

		int len1 = 0;
		if (!EVP_EncryptUpdate(
			ctx,
			buffer.data(),
			&len1,
			reinterpret_cast<const unsigned char*>(input.data()),
			static_cast<int>(input.size())))
			break;

		int len2 = 0;
		if (!EVP_EncryptFinal(ctx, buffer.data() + len1, &len2))
			break;

		const int outLen = len1 + len2;
		std::string cipher(reinterpret_cast<char*>(buffer.data()), outLen);

		// Base64（与旧实现一致）
		outEncryptedParams = CSafeBase64::Encode(cipher);
		ok = true;

	} while (false);

	EVP_CIPHER_CTX_free(ctx);
	return ok;
}

bool GunionCrypto::CalcWegamePicSmsSendSignatureMd5(
	const std::string& phone,
	const std::string& type,
	const std::string& checkCodeGuid,
	const std::string& checkCode,
	const std::string& outInfo,
	const std::string& voiceMsg,
	const std::string& smsNew,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建签名参数（与旧 CGAuthClient 完全一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["phone"] = phone;
	params["type"] = type;
	params["checkCodeGuid"] = checkCodeGuid;
	params["checkCode"] = checkCode;
	params["outInfo"] = outInfo;
	params["voiceMsg"] = voiceMsg;
	params["sms_new"] = smsNew;
	params["device_id_server"] = deviceIdServer;

	// ------------------------------------------------------------
	// 2. 排序 key=value（旧实现使用 set）
	// ------------------------------------------------------------
	std::set<std::string> sorted;
	for (const auto& kv : params)
	{
		sorted.insert(kv.first + "=" + kv.second);
	}

	std::string raw;
	bool first = true;
	for (const auto& item : sorted)
	{
		if (!first)
			raw += '&';

		raw += item;
		first = false;
	}

	// 3. 拼接 randKey
	raw += randKey;

	// 4. 转小写（保持旧行为）
	std::transform(raw.begin(), raw.end(), raw.begin(), ::tolower);

	// ------------------------------------------------------------
	// 5. MD5 计算 → 32 位大写 HEX
	// ------------------------------------------------------------
	unsigned char md5Bytes[16] = { 0 };
	MD5(
		reinterpret_cast<const unsigned char*>(raw.c_str()),
		raw.size(),
		md5Bytes);

	char hex[33] = { 0 };
	for (int i = 0; i < 16; ++i)
	{
		_snprintf_s(hex + i * 2, 3, _TRUNCATE, "%02X", md5Bytes[i]);
	}
	hex[32] = '\0';

	outSignature.assign(hex);
	return true;
}

bool GunionCrypto::EncryptWegameThirdAccountBindPhoneParams3DesBase64(
	const std::string& deviceId,
	const std::string& phone,
	const std::string& smscode,
	const std::string& type,
	const std::string& scene,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建明文参数（与旧 CGAuthClient 完全一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["deviceid"] = deviceId;
	params["phone"] = phone;
	params["smscode"] = smscode;
	params["type"] = type;
	params["scene"] = scene;
	params["device_id_server"] = deviceIdServer;

	std::string input;
	bool first = true;
	for (const auto& kv : params)
	{
		if (!first)
			input += '&';

		// 仅 value 进行 URL Encode（保持旧逻辑）
		input += kv.first;
		input += '=';
		input += UrlEncoder::Encode(kv.second);
		first = false;
	}

	// ------------------------------------------------------------
	// 2. 3DES(EDE3/ECB) 加密
	// ------------------------------------------------------------
	static const unsigned char IV[8] = { 0 };

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (!ctx)
		return false;

	bool ok = false;
	do
	{
		if (!EVP_EncryptInit(
			ctx,
			EVP_des_ede3_ecb(),
			reinterpret_cast<const unsigned char*>(randKey.data()),
			IV))
			break;

		int bufLen = input.size() <= 4
			? 9
			: static_cast<int>(input.size() * 2);

		std::vector<unsigned char> buffer(bufLen, 0);

		int len1 = 0;
		if (!EVP_EncryptUpdate(
			ctx,
			buffer.data(),
			&len1,
			reinterpret_cast<const unsigned char*>(input.data()),
			static_cast<int>(input.size())))
			break;

		int len2 = 0;
		if (!EVP_EncryptFinal(ctx, buffer.data() + len1, &len2))
			break;

		const int outLen = len1 + len2;
		std::string cipher(reinterpret_cast<char*>(buffer.data()), outLen);

		outEncryptedParams = CSafeBase64::Encode(cipher);
		ok = true;

	} while (false);

	EVP_CIPHER_CTX_free(ctx);
	return ok;
}

bool GunionCrypto::CalcWegameThirdAccountBindPhoneSignatureMd5(
	const std::string& deviceId,
	const std::string& phone,
	const std::string& smscode,
	const std::string& type,
	const std::string& scene,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建签名参数（与旧 CGAuthClient 完全一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["deviceid"] = deviceId;
	params["phone"] = phone;
	params["smscode"] = smscode;
	params["type"] = type;
	params["scene"] = scene;
	params["device_id_server"] = deviceIdServer;

	// ------------------------------------------------------------
	// 2. 排序 key=value
	// ------------------------------------------------------------
	std::set<std::string> sorted;
	for (const auto& kv : params)
	{
		sorted.insert(kv.first + "=" + kv.second);
	}

	std::string raw;
	bool first = true;
	for (const auto& item : sorted)
	{
		if (!first)
			raw += '&';

		raw += item;
		first = false;
	}

	// 拼接 randKey
	raw += randKey;

	// 转小写（保持旧行为）
	std::transform(raw.begin(), raw.end(), raw.begin(), ::tolower);

	// ------------------------------------------------------------
	// 3. MD5 → 32 位大写 HEX
	// ------------------------------------------------------------
	unsigned char md5Bytes[16] = { 0 };
	MD5(
		reinterpret_cast<const unsigned char*>(raw.c_str()),
		raw.size(),
		md5Bytes);

	char hex[33] = { 0 };
	for (int i = 0; i < 16; ++i)
	{
		_snprintf_s(hex + i * 2, 3, _TRUNCATE, "%02X", md5Bytes[i]);
	}
	hex[32] = '\0';

	outSignature.assign(hex);
	return true;
}

bool GunionCrypto::EncryptWegameFillRealinfoParams3DesBase64(
	const std::string& idcard,
	const std::string& nameUtf8,
	const std::string& realInfoFlowId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建明文参数（顺序 = map 字典序，与旧实现一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["idcard"] = idcard;
	params["name"] = nameUtf8;
	params["realInfoFlowId"] = realInfoFlowId;
	params["device_id_server"] = deviceIdServer;

	std::string input;
	bool first = true;
	for (const auto& kv : params)
	{
		if (!first)
			input += '&';

		// value 必须 URL Encode
		input += kv.first;
		input += '=';
		input += UrlEncoder::Encode(kv.second);
		first = false;
	}

	// ------------------------------------------------------------
	// 2. 3DES(EDE3/ECB) 加密
	// ------------------------------------------------------------
	static const unsigned char ENCRYPT_IV[8] = { 0 };

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (!ctx)
		return false;

	bool ok = false;

	if (EVP_EncryptInit(
		ctx,
		EVP_des_ede3_ecb(),
		reinterpret_cast<const unsigned char*>(randKey.data()),
		ENCRYPT_IV))
	{
		int outLen = 0;
		int bufLen = input.size() <= 4 ? 9 : static_cast<int>(input.size() * 2);

		std::vector<unsigned char> buffer(bufLen, 0);

		if (EVP_EncryptUpdate(
			ctx,
			buffer.data(),
			&outLen,
			reinterpret_cast<const unsigned char*>(input.data()),
			static_cast<int>(input.size())))
		{
			int tmpLen = 0;
			if (EVP_EncryptFinal(ctx, buffer.data() + outLen, &tmpLen))
			{
				outLen += tmpLen;

				std::string cipher(
					reinterpret_cast<char*>(buffer.data()),
					outLen);

				outEncryptedParams = CSafeBase64::Encode(cipher);
				ok = true;
			}
		}
	}

	EVP_CIPHER_CTX_free(ctx);
	return ok;
}

bool GunionCrypto::CalcWegameFillRealinfoSignatureMd5(
	const std::string& idcard,
	const std::string& nameUtf8,
	const std::string& realInfoFlowId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建签名参数（必须与旧逻辑完全一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["idcard"] = idcard;
	params["name"] = nameUtf8;
	params["realInfoFlowId"] = realInfoFlowId;
	params["device_id_server"] = deviceIdServer;

	// ------------------------------------------------------------
	// 2. 排序 key=value
	// ------------------------------------------------------------
	std::set<std::string> sorted;
	for (const auto& kv : params)
	{
		sorted.insert(kv.first + "=" + kv.second);
	}

	std::string raw;
	bool first = true;
	for (const auto& item : sorted)
	{
		if (!first)
			raw += '&';

		raw += item;
		first = false;
	}

	// ------------------------------------------------------------
	// 3. 拼接 randKey + tolower
	// ------------------------------------------------------------
	raw += randKey;
	std::transform(raw.begin(), raw.end(), raw.begin(), ::tolower);

	// ------------------------------------------------------------
	// 4. MD5
	// ------------------------------------------------------------
	unsigned char md5Bytes[16] = { 0 };
	MD5(reinterpret_cast<const unsigned char*>(raw.c_str()),
		raw.size(),
		md5Bytes);

	char hex[33] = { 0 };
	for (int i = 0; i < 16; ++i)
	{
		_snprintf_s(hex + i * 2, 3, _TRUNCATE, "%02X", md5Bytes[i]);
	}
	hex[32] = '\0';

	outSignature.assign(hex);
	return true;
}

bool GunionCrypto::EncryptWegameGetTicketParams3DesBase64(
	int time_s,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建明文参数（map 字典序，保持旧实现一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["seq"] = std::to_string(time_s);
	params["device_id_server"] = deviceIdServer;

	std::string input;
	bool first = true;
	for (const auto& kv : params)
	{
		if (!first)
			input += '&';

		// value 必须 URL Encode
		input += kv.first;
		input += '=';
		input += UrlEncoder::Encode(kv.second);
		first = false;
	}

	// ------------------------------------------------------------
	// 2. 3DES(EDE3/ECB) 加密（IV 全 0，保持旧行为）
	// ------------------------------------------------------------
	static const unsigned char IV[8] = { 0 };

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (!ctx)
		return false;

	bool ok = false;

	if (EVP_EncryptInit(
		ctx,
		EVP_des_ede3_ecb(),
		reinterpret_cast<const unsigned char*>(randKey.data()),
		IV))
	{
		int outLen1 = 0;
		int bufLen = input.size() <= 4 ? 9 : static_cast<int>(input.size() * 2);
		std::vector<unsigned char> buffer(bufLen, 0);

		if (EVP_EncryptUpdate(
			ctx,
			buffer.data(),
			&outLen1,
			reinterpret_cast<const unsigned char*>(input.data()),
			static_cast<int>(input.size())))
		{
			int outLen2 = 0;
			if (EVP_EncryptFinal(ctx, buffer.data() + outLen1, &outLen2))
			{
				const int total = outLen1 + outLen2;
				std::string cipher(reinterpret_cast<char*>(buffer.data()), total);

				// Base64（与旧逻辑一致）
				outEncryptedParams = CSafeBase64::Encode(cipher);
				ok = true;
			}
		}
	}

	EVP_CIPHER_CTX_free(ctx);
	return ok;
}

bool GunionCrypto::CalcWegameGetTicketSignatureMd5(
	int time_s,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建签名参数（与旧实现完全一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["seq"] = std::to_string(time_s);
	params["device_id_server"] = deviceIdServer;

	// ------------------------------------------------------------
	// 2. key=value 排序（set）
	// ------------------------------------------------------------
	std::set<std::string> sorted;
	for (const auto& kv : params)
	{
		sorted.insert(kv.first + "=" + kv.second);
	}

	std::string raw;
	bool first = true;
	for (const auto& item : sorted)
	{
		if (!first)
			raw += '&';
		raw += item;
		first = false;
	}

	// ------------------------------------------------------------
	// 3. 拼接 randKey + tolower
	// ------------------------------------------------------------
	raw += randKey;
	std::transform(raw.begin(), raw.end(), raw.begin(), ::tolower);

	// ------------------------------------------------------------
	// 4. MD5 → 32 位大写 HEX
	// ------------------------------------------------------------
	unsigned char md5Bytes[16] = { 0 };
	MD5(reinterpret_cast<const unsigned char*>(raw.c_str()),
		raw.size(),
		md5Bytes);

	char hex[33] = { 0 };
	for (int i = 0; i < 16; ++i)
	{
		_snprintf_s(hex + i * 2, 3, _TRUNCATE, "%02X", md5Bytes[i]);
	}
	hex[32] = '\0';

	outSignature.assign(hex);
	return true;
}

bool GunionCrypto::EncryptGunionSmsLoginParams3DesBase64(
	const std::string& sms_new,
	const std::string& sms_type,
	const std::string& phone,
	const std::string& smsCode,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建明文参数（map 字典序，保持旧实现一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["deviceid"] = windowsDeviceId;
	params["phone"] = phone;
	params["smscode"] = smsCode;
	params["device_id_server"] = deviceIdServer;
	params["sms_new"] = sms_new;
	params["sms_type"] = sms_type;

	std::string input;
	bool first = true;
	for (const auto& kv : params)
	{
		if (!first)
			input += '&';

		// value 必须 URL Encode
		input += kv.first;
		input += '=';
		input += UrlEncoder::Encode(kv.second);
		first = false;
	}

	// ------------------------------------------------------------
	// 2. 3DES(EDE3/ECB) 加密（IV 全 0，保持旧行为）
	// ------------------------------------------------------------
	static const unsigned char IV[8] = { 0 };

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (!ctx)
		return false;

	bool ok = false;

	if (EVP_EncryptInit(
		ctx,
		EVP_des_ede3_ecb(),
		reinterpret_cast<const unsigned char*>(randKey.data()),
		IV))
	{
		int outLen1 = 0;
		int bufLen = input.size() <= 4 ? 9 : static_cast<int>(input.size() * 2);
		std::vector<unsigned char> buffer(bufLen, 0);

		if (EVP_EncryptUpdate(
			ctx,
			buffer.data(),
			&outLen1,
			reinterpret_cast<const unsigned char*>(input.data()),
			static_cast<int>(input.size())))
		{
			int outLen2 = 0;
			if (EVP_EncryptFinal(ctx, buffer.data() + outLen1, &outLen2))
			{
				const int total = outLen1 + outLen2;
				std::string cipher(reinterpret_cast<char*>(buffer.data()), total);

				// Base64（与旧逻辑一致）
				outEncryptedParams = CSafeBase64::Encode(cipher);
				ok = true;
			}
		}
	}

	EVP_CIPHER_CTX_free(ctx);
	return ok;
}

bool GunionCrypto::CalcGunionSmsLoginSignatureMd5(
	const std::string& sms_new,
	const std::string& sms_type,
	const std::string& phone,
	const std::string& smsCode,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建签名参数（与旧实现完全一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["deviceid"] = windowsDeviceId;
	params["phone"] = phone;
	params["smscode"] = smsCode;
	params["device_id_server"] = deviceIdServer;
	params["sms_new"] = sms_new;
	params["sms_type"] = sms_type;

	// ------------------------------------------------------------
	// 2. key=value 排序（set）
	// ------------------------------------------------------------
	std::set<std::string> sorted;
	for (const auto& kv : params)
	{
		sorted.insert(kv.first + "=" + kv.second);
	}

	std::string raw;
	bool first = true;
	for (const auto& item : sorted)
	{
		if (!first)
			raw += '&';
		raw += item;
		first = false;
	}

	// ------------------------------------------------------------
	// 3. 拼接 randKey + tolower
	// ------------------------------------------------------------
	raw += randKey;
	std::transform(raw.begin(), raw.end(), raw.begin(), ::tolower);

	// ------------------------------------------------------------
	// 4. MD5 → 32 位大写 HEX
	// ------------------------------------------------------------
	unsigned char md5Bytes[16] = { 0 };
	MD5(reinterpret_cast<const unsigned char*>(raw.c_str()),
		raw.size(),
		md5Bytes);

	char hex[33] = { 0 };
	for (int i = 0; i < 16; ++i)
	{
		_snprintf_s(hex + i * 2, 3, _TRUNCATE, "%02X", md5Bytes[i]);
	}
	hex[32] = '\0';

	outSignature.assign(hex);
	return true;
}

bool GunionCrypto::EncryptGunionRealNameParams3DesBase64(
	const std::string& idcard,
	const std::string& name,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建明文参数（map 字典序，保持旧实现一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["idcard"] = idcard;
	params["name"] = name;
	params["device_id_server"] = deviceIdServer;

	std::string input;
	bool first = true;
	for (const auto& kv : params)
	{
		if (!first)
			input += '&';

		// value 必须 URL Encode
		input += kv.first;
		input += '=';
		input += UrlEncoder::Encode(kv.second);
		first = false;
	}

	// ------------------------------------------------------------
	// 2. 3DES(EDE3/ECB) 加密（IV 全 0，保持旧行为）
	// ------------------------------------------------------------
	static const unsigned char IV[8] = { 0 };

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (!ctx)
		return false;

	bool ok = false;

	if (EVP_EncryptInit(
		ctx,
		EVP_des_ede3_ecb(),
		reinterpret_cast<const unsigned char*>(randKey.data()),
		IV))
	{
		int outLen1 = 0;
		int bufLen = input.size() <= 4 ? 9 : static_cast<int>(input.size() * 2);
		std::vector<unsigned char> buffer(bufLen, 0);

		if (EVP_EncryptUpdate(
			ctx,
			buffer.data(),
			&outLen1,
			reinterpret_cast<const unsigned char*>(input.data()),
			static_cast<int>(input.size())))
		{
			int outLen2 = 0;
			if (EVP_EncryptFinal(ctx, buffer.data() + outLen1, &outLen2))
			{
				const int total = outLen1 + outLen2;
				std::string cipher(reinterpret_cast<char*>(buffer.data()), total);

				// Base64（与旧逻辑一致）
				outEncryptedParams = CSafeBase64::Encode(cipher);
				ok = true;
			}
		}
	}

	EVP_CIPHER_CTX_free(ctx);
	return ok;
}

bool GunionCrypto::CalcGunionRealNameSignatureMd5(
	const std::string& idcard,
	const std::string& name,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建签名参数（与旧实现完全一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["idcard"] = idcard;
	params["name"] = name;
	params["device_id_server"] = deviceIdServer;

	// ------------------------------------------------------------
	// 2. key=value 排序（set）
	// ------------------------------------------------------------
	std::set<std::string> sorted;
	for (const auto& kv : params)
	{
		sorted.insert(kv.first + "=" + kv.second);
	}

	std::string raw;
	bool first = true;
	for (const auto& item : sorted)
	{
		if (!first)
			raw += '&';
		raw += item;
		first = false;
	}

	// ------------------------------------------------------------
	// 3. 拼接 randKey + tolower
	// ------------------------------------------------------------
	raw += randKey;
	std::transform(raw.begin(), raw.end(), raw.begin(), ::tolower);

	// ------------------------------------------------------------
	// 4. MD5 → 32 位大写 HEX
	// ------------------------------------------------------------
	unsigned char md5Bytes[16] = { 0 };
	MD5(reinterpret_cast<const unsigned char*>(raw.c_str()),
		raw.size(),
		md5Bytes);

	char hex[33] = { 0 };
	for (int i = 0; i < 16; ++i)
	{
		_snprintf_s(hex + i * 2, 3, _TRUNCATE, "%02X", md5Bytes[i]);
	}
	hex[32] = '\0';

	outSignature.assign(hex);
	return true;
}

bool GunionCrypto::EncryptGunionAutoLoginParams3DesBase64(
	const std::string& autokey,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建明文参数（map 字典序，保持旧实现一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["deviceid"] = windowsDeviceId;
	params["autokey"] = autokey;
	params["device_id_server"] = deviceIdServer;

	std::string input;
	bool first = true;
	for (const auto& kv : params)
	{
		if (!first)
			input += '&';

		// value 必须 URL Encode
		input += kv.first;
		input += '=';
		input += UrlEncoder::Encode(kv.second);
		first = false;
	}

	// ------------------------------------------------------------
	// 2. 3DES(EDE3/ECB) 加密（IV 全 0，保持旧行为）
	// ------------------------------------------------------------
	static const unsigned char IV[8] = { 0 };

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (!ctx)
		return false;

	bool ok = false;

	if (EVP_EncryptInit(
		ctx,
		EVP_des_ede3_ecb(),
		reinterpret_cast<const unsigned char*>(randKey.data()),
		IV))
	{
		int outLen1 = 0;
		int bufLen = input.size() <= 4 ? 9 : static_cast<int>(input.size() * 2);
		std::vector<unsigned char> buffer(bufLen, 0);

		if (EVP_EncryptUpdate(
			ctx,
			buffer.data(),
			&outLen1,
			reinterpret_cast<const unsigned char*>(input.data()),
			static_cast<int>(input.size())))
		{
			int outLen2 = 0;
			if (EVP_EncryptFinal(ctx, buffer.data() + outLen1, &outLen2))
			{
				const int total = outLen1 + outLen2;
				std::string cipher(reinterpret_cast<char*>(buffer.data()), total);

				// Base64（与旧逻辑一致）
				outEncryptedParams = CSafeBase64::Encode(cipher);
				ok = true;
			}
		}
	}

	EVP_CIPHER_CTX_free(ctx);
	return ok;
}

bool GunionCrypto::CalcGunionAutoLoginSignatureMd5(
	const std::string& autokey,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建签名参数（与旧实现完全一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["deviceid"] = windowsDeviceId;
	params["autokey"] = autokey;
	params["device_id_server"] = deviceIdServer;

	// ------------------------------------------------------------
	// 2. key=value 排序（set）
	// ------------------------------------------------------------
	std::set<std::string> sorted;
	for (const auto& kv : params)
	{
		sorted.insert(kv.first + "=" + kv.second);
	}

	std::string raw;
	bool first = true;
	for (const auto& item : sorted)
	{
		if (!first)
			raw += '&';
		raw += item;
		first = false;
	}

	// ------------------------------------------------------------
	// 3. 拼接 randKey + tolower
	// ------------------------------------------------------------
	raw += randKey;
	std::transform(raw.begin(), raw.end(), raw.begin(), ::tolower);

	// ------------------------------------------------------------
	// 4. MD5 → 32 位大写 HEX
	// ------------------------------------------------------------
	unsigned char md5Bytes[16] = { 0 };
	MD5(reinterpret_cast<const unsigned char*>(raw.c_str()),
		raw.size(),
		md5Bytes);

	char hex[33] = { 0 };
	for (int i = 0; i < 16; ++i)
	{
		_snprintf_s(hex + i * 2, 3, _TRUNCATE, "%02X", md5Bytes[i]);
	}
	hex[32] = '\0';

	outSignature.assign(hex);
	return true;
}

bool GunionCrypto::EncryptGunionLogoutParams3DesBase64(
	const std::string& autokey,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建明文参数（map 字典序，保持旧实现一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["autokey"] = autokey;
	params["device_id_server"] = deviceIdServer;

	std::string input;
	bool first = true;
	for (const auto& kv : params)
	{
		if (!first)
			input += '&';

		// value 必须 URL Encode
		input += kv.first;
		input += '=';
		input += UrlEncoder::Encode(kv.second);
		first = false;
	}

	// ------------------------------------------------------------
	// 2. 3DES(EDE3/ECB) 加密（IV 全 0，保持旧行为）
	// ------------------------------------------------------------
	static const unsigned char IV[8] = { 0 };

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (!ctx)
		return false;

	bool ok = false;

	if (EVP_EncryptInit(
		ctx,
		EVP_des_ede3_ecb(),
		reinterpret_cast<const unsigned char*>(randKey.data()),
		IV))
	{
		int outLen1 = 0;
		int bufLen = input.size() <= 4 ? 9 : static_cast<int>(input.size() * 2);
		std::vector<unsigned char> buffer(bufLen, 0);

		if (EVP_EncryptUpdate(
			ctx,
			buffer.data(),
			&outLen1,
			reinterpret_cast<const unsigned char*>(input.data()),
			static_cast<int>(input.size())))
		{
			int outLen2 = 0;
			if (EVP_EncryptFinal(ctx, buffer.data() + outLen1, &outLen2))
			{
				const int total = outLen1 + outLen2;
				std::string cipher(reinterpret_cast<char*>(buffer.data()), total);

				// Base64（与旧逻辑一致）
				outEncryptedParams = CSafeBase64::Encode(cipher);
				ok = true;
			}
		}
	}

	EVP_CIPHER_CTX_free(ctx);
	return ok;
}

bool GunionCrypto::CalcGunionLogoutSignatureMd5(
	const std::string& autokey,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建签名参数（与旧实现完全一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["autokey"] = autokey;
	params["device_id_server"] = deviceIdServer;

	// ------------------------------------------------------------
	// 2. key=value 排序（set）
	// ------------------------------------------------------------
	std::set<std::string> sorted;
	for (const auto& kv : params)
	{
		sorted.insert(kv.first + "=" + kv.second);
	}

	std::string raw;
	bool first = true;
	for (const auto& item : sorted)
	{
		if (!first)
			raw += '&';
		raw += item;
		first = false;
	}

	// ------------------------------------------------------------
	// 3. 拼接 randKey + tolower
	// ------------------------------------------------------------
	raw += randKey;
	std::transform(raw.begin(), raw.end(), raw.begin(), ::tolower);

	// ------------------------------------------------------------
	// 4. MD5 → 32 位大写 HEX
	// ------------------------------------------------------------
	unsigned char md5Bytes[16] = { 0 };
	MD5(reinterpret_cast<const unsigned char*>(raw.c_str()),
		raw.size(),
		md5Bytes);

	char hex[33] = { 0 };
	for (int i = 0; i < 16; ++i)
	{
		_snprintf_s(hex + i * 2, 3, _TRUNCATE, "%02X", md5Bytes[i]);
	}
	hex[32] = '\0';

	outSignature.assign(hex);
	return true;
}

bool GunionCrypto::EncryptGunionPwdLoginParams3DesBase64(
	const std::string& phone,
	const std::string& password,
	const std::string& supportPic,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建明文参数（map 字典序，保持旧实现一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["phone"] = phone;
	params["deviceid"] = windowsDeviceId;
	params["password"] = password;
	params["supportPic"] = supportPic;
	params["device_id_server"] = deviceIdServer;

	std::string input;
	bool first = true;
	for (const auto& kv : params)
	{
		if (!first)
			input += '&';

		// value 必须 URL Encode
		input += kv.first;
		input += '=';
		input += UrlEncoder::Encode(kv.second);
		first = false;
	}

	// ------------------------------------------------------------
	// 2. 3DES(EDE3/ECB) 加密（IV 全 0，保持旧行为）
	// ------------------------------------------------------------
	static const unsigned char IV[8] = { 0 };

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (!ctx)
		return false;

	bool ok = false;

	if (EVP_EncryptInit(
		ctx,
		EVP_des_ede3_ecb(),
		reinterpret_cast<const unsigned char*>(randKey.data()),
		IV))
	{
		int outLen1 = 0;
		int bufLen = input.size() <= 4 ? 9 : static_cast<int>(input.size() * 2);
		std::vector<unsigned char> buffer(bufLen, 0);

		if (EVP_EncryptUpdate(
			ctx,
			buffer.data(),
			&outLen1,
			reinterpret_cast<const unsigned char*>(input.data()),
			static_cast<int>(input.size())))
		{
			int outLen2 = 0;
			if (EVP_EncryptFinal(ctx, buffer.data() + outLen1, &outLen2))
			{
				const int total = outLen1 + outLen2;
				std::string cipher(reinterpret_cast<char*>(buffer.data()), total);

				// Base64（与旧逻辑一致）
				outEncryptedParams = CSafeBase64::Encode(cipher);
				ok = true;
			}
		}
	}

	EVP_CIPHER_CTX_free(ctx);
	return ok;
}

bool GunionCrypto::CalcGunionPwdLoginSignatureMd5(
	const std::string& phone,
	const std::string& password,
	const std::string& supportPic,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建签名参数（与旧实现完全一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["phone"] = phone;
	params["deviceid"] = windowsDeviceId;
	params["password"] = password;
	params["supportPic"] = supportPic;
	params["device_id_server"] = deviceIdServer;

	// ------------------------------------------------------------
	// 2. key=value 排序（set）
	// ------------------------------------------------------------
	std::set<std::string> sorted;
	for (const auto& kv : params)
	{
		sorted.insert(kv.first + "=" + kv.second);
	}

	std::string raw;
	bool first = true;
	for (const auto& item : sorted)
	{
		if (!first)
			raw += '&';
		raw += item;
		first = false;
	}

	// ------------------------------------------------------------
	// 3. 拼接 randKey + tolower
	// ------------------------------------------------------------
	raw += randKey;
	std::transform(raw.begin(), raw.end(), raw.begin(), ::tolower);

	// ------------------------------------------------------------
	// 4. MD5 → 32 位大写 HEX
	// ------------------------------------------------------------
	unsigned char md5Bytes[16] = { 0 };
	MD5(reinterpret_cast<const unsigned char*>(raw.c_str()),
		raw.size(),
		md5Bytes);

	char hex[33] = { 0 };
	for (int i = 0; i < 16; ++i)
	{
		_snprintf_s(hex + i * 2, 3, _TRUNCATE, "%02X", md5Bytes[i]);
	}
	hex[32] = '\0';

	outSignature.assign(hex);
	return true;
}

bool GunionCrypto::EncryptGunionPicPwdLoginParams3DesBase64(
	const std::string& checkCodeGuid,
	const std::string& password,
	const std::string& outInfo,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建明文参数（map 字典序，保持旧实现一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["deviceid"] = windowsDeviceId;
	params["checkCodeGuid"] = checkCodeGuid;
	params["password"] = password;
	params["outInfo"] = outInfo;
	params["device_id_server"] = deviceIdServer;

	std::string input;
	bool first = true;
	for (const auto& kv : params)
	{
		if (!first)
			input += '&';

		// value 必须 URL Encode
		input += kv.first;
		input += '=';
		input += UrlEncoder::Encode(kv.second);
		first = false;
	}

	// ------------------------------------------------------------
	// 2. 3DES(EDE3/ECB) 加密（IV 全 0，保持旧行为）
	// ------------------------------------------------------------
	static const unsigned char IV[8] = { 0 };

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (!ctx)
		return false;

	bool ok = false;

	if (EVP_EncryptInit(
		ctx,
		EVP_des_ede3_ecb(),
		reinterpret_cast<const unsigned char*>(randKey.data()),
		IV))
	{
		int outLen1 = 0;
		int bufLen = input.size() <= 4 ? 9 : static_cast<int>(input.size() * 2);
		std::vector<unsigned char> buffer(bufLen, 0);

		if (EVP_EncryptUpdate(
			ctx,
			buffer.data(),
			&outLen1,
			reinterpret_cast<const unsigned char*>(input.data()),
			static_cast<int>(input.size())))
		{
			int outLen2 = 0;
			if (EVP_EncryptFinal(ctx, buffer.data() + outLen1, &outLen2))
			{
				const int total = outLen1 + outLen2;
				std::string cipher(reinterpret_cast<char*>(buffer.data()), total);

				// Base64（与旧逻辑一致）
				outEncryptedParams = CSafeBase64::Encode(cipher);
				ok = true;
			}
		}
	}

	EVP_CIPHER_CTX_free(ctx);
	return ok;
}

bool GunionCrypto::CalcGunionPicPwdLoginSignatureMd5(
	const std::string& checkCodeGuid,
	const std::string& password,
	const std::string& outInfo,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建签名参数（与旧实现完全一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["deviceid"] = windowsDeviceId;
	params["checkCodeGuid"] = checkCodeGuid;
	params["password"] = password;
	params["outInfo"] = outInfo;
	params["device_id_server"] = deviceIdServer;

	// ------------------------------------------------------------
	// 2. key=value 排序（set）
	// ------------------------------------------------------------
	std::set<std::string> sorted;
	for (const auto& kv : params)
	{
		sorted.insert(kv.first + "=" + kv.second);
	}

	std::string raw;
	bool first = true;
	for (const auto& item : sorted)
	{
		if (!first)
			raw += '&';
		raw += item;
		first = false;
	}

	// ------------------------------------------------------------
	// 3. 拼接 randKey + tolower
	// ------------------------------------------------------------
	raw += randKey;
	std::transform(raw.begin(), raw.end(), raw.begin(), ::tolower);

	// ------------------------------------------------------------
	// 4. MD5 → 32 位大写 HEX
	// ------------------------------------------------------------
	unsigned char md5Bytes[16] = { 0 };
	MD5(reinterpret_cast<const unsigned char*>(raw.c_str()),
		raw.size(),
		md5Bytes);

	char hex[33] = { 0 };
	for (int i = 0; i < 16; ++i)
	{
		_snprintf_s(hex + i * 2, 3, _TRUNCATE, "%02X", md5Bytes[i]);
	}
	hex[32] = '\0';

	outSignature.assign(hex);
	return true;
}

bool GunionCrypto::EncryptGunionThirdLoginParams3DesBase64(
	const std::string& companyId,
	const std::string& thirdTicket,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建明文参数（map 字典序，保持旧实现一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["deviceid"] = windowsDeviceId;
	params["company_id"] = companyId;
	params["third_ticket"] = thirdTicket;
	params["is_websdk"] = "1";
	params["device_id_server"] = deviceIdServer;

	std::string input;
	bool first = true;
	for (const auto& kv : params)
	{
		if (!first)
			input += '&';

		// value 必须 URL Encode
		input += kv.first;
		input += '=';
		input += UrlEncoder::Encode(kv.second);
		first = false;
	}

	// ------------------------------------------------------------
	// 2. 3DES(EDE3/ECB) 加密（IV 全 0，保持旧行为）
	// ------------------------------------------------------------
	static const unsigned char IV[8] = { 0 };

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (!ctx)
		return false;

	bool ok = false;

	if (EVP_EncryptInit(
		ctx,
		EVP_des_ede3_ecb(),
		reinterpret_cast<const unsigned char*>(randKey.data()),
		IV))
	{
		int outLen1 = 0;
		int bufLen = input.size() <= 4 ? 9 : static_cast<int>(input.size() * 2);
		std::vector<unsigned char> buffer(bufLen, 0);

		if (EVP_EncryptUpdate(
			ctx,
			buffer.data(),
			&outLen1,
			reinterpret_cast<const unsigned char*>(input.data()),
			static_cast<int>(input.size())))
		{
			int outLen2 = 0;
			if (EVP_EncryptFinal(ctx, buffer.data() + outLen1, &outLen2))
			{
				const int total = outLen1 + outLen2;
				std::string cipher(reinterpret_cast<char*>(buffer.data()), total);

				// Base64（与旧逻辑一致）
				outEncryptedParams = CSafeBase64::Encode(cipher);
				ok = true;
			}
		}
	}

	EVP_CIPHER_CTX_free(ctx);
	return ok;
}

bool GunionCrypto::CalcGunionThirdLoginSignatureMd5(
	const std::string& companyId,
	const std::string& thirdTicket,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建签名参数（与旧实现完全一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["deviceid"] = windowsDeviceId;
	params["company_id"] = companyId;
	params["third_ticket"] = thirdTicket;
	params["is_websdk"] = "1";
	params["device_id_server"] = deviceIdServer;

	// ------------------------------------------------------------
	// 2. key=value 排序（set）
	// ------------------------------------------------------------
	std::set<std::string> sorted;
	for (const auto& kv : params)
	{
		sorted.insert(kv.first + "=" + kv.second);
	}

	std::string raw;
	bool first = true;
	for (const auto& item : sorted)
	{
		if (!first)
			raw += '&';
		raw += item;
		first = false;
	}

	// ------------------------------------------------------------
	// 3. 拼接 randKey + tolower
	// ------------------------------------------------------------
	raw += randKey;
	std::transform(raw.begin(), raw.end(), raw.begin(), ::tolower);

	// ------------------------------------------------------------
	// 4. MD5 → 32 位大写 HEX
	// ------------------------------------------------------------
	unsigned char md5Bytes[16] = { 0 };
	MD5(reinterpret_cast<const unsigned char*>(raw.c_str()),
		raw.size(),
		md5Bytes);

	char hex[33] = { 0 };
	for (int i = 0; i < 16; ++i)
	{
		_snprintf_s(hex + i * 2, 3, _TRUNCATE, "%02X", md5Bytes[i]);
	}
	hex[32] = '\0';

	outSignature.assign(hex);
	return true;
}

bool GunionCrypto::EncryptGunionLoginAreaParams3DesBase64(
	const std::string& userid,
	const std::string& area,
	const std::string& group,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建明文参数（map 字典序，保持旧实现一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["userid"] = userid;
	params["area"] = area;
	params["deviceid"] = windowsDeviceId;
	params["group"] = group;
	params["device_id_server"] = deviceIdServer;

	std::string input;
	bool first = true;
	for (const auto& kv : params)
	{
		if (!first)
			input += '&';

		// value 必须 URL Encode
		input += kv.first;
		input += '=';
		input += UrlEncoder::Encode(kv.second);
		first = false;
	}

	// ------------------------------------------------------------
	// 2. 3DES(EDE3/ECB) 加密（IV 全 0，保持旧行为）
	// ------------------------------------------------------------
	static const unsigned char IV[8] = { 0 };

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (!ctx)
		return false;

	bool ok = false;

	if (EVP_EncryptInit(
		ctx,
		EVP_des_ede3_ecb(),
		reinterpret_cast<const unsigned char*>(randKey.data()),
		IV))
	{
		int outLen1 = 0;
		int bufLen = input.size() <= 4 ? 9 : static_cast<int>(input.size() * 2);
		std::vector<unsigned char> buffer(bufLen, 0);

		if (EVP_EncryptUpdate(
			ctx,
			buffer.data(),
			&outLen1,
			reinterpret_cast<const unsigned char*>(input.data()),
			static_cast<int>(input.size())))
		{
			int outLen2 = 0;
			if (EVP_EncryptFinal(ctx, buffer.data() + outLen1, &outLen2))
			{
				const int total = outLen1 + outLen2;
				std::string cipher(reinterpret_cast<char*>(buffer.data()), total);

				// Base64（与旧逻辑一致）
				outEncryptedParams = CSafeBase64::Encode(cipher);
				ok = true;
			}
		}
	}

	EVP_CIPHER_CTX_free(ctx);
	return ok;
}

bool GunionCrypto::CalcGunionLoginAreaSignatureMd5(
	const std::string& userid,
	const std::string& area,
	const std::string& group,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建签名参数（与旧实现完全一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["userid"] = userid;
	params["area"] = area;
	params["deviceid"] = windowsDeviceId;
	params["group"] = group;
	params["device_id_server"] = deviceIdServer;

	// ------------------------------------------------------------
	// 2. key=value 排序（set）
	// ------------------------------------------------------------
	std::set<std::string> sorted;
	for (const auto& kv : params)
	{
		sorted.insert(kv.first + "=" + kv.second);
	}

	std::string raw;
	bool first = true;
	for (const auto& item : sorted)
	{
		if (!first)
			raw += '&';
		raw += item;
		first = false;
	}

	// ------------------------------------------------------------
	// 3. 拼接 randKey + tolower
	// ------------------------------------------------------------
	raw += randKey;
	std::transform(raw.begin(), raw.end(), raw.begin(), ::tolower);

	// ------------------------------------------------------------
	// 4. MD5 → 32 位大写 HEX
	// ------------------------------------------------------------
	unsigned char md5Bytes[16] = { 0 };
	MD5(reinterpret_cast<const unsigned char*>(raw.c_str()),
		raw.size(),
		md5Bytes);

	char hex[33] = { 0 };
	for (int i = 0; i < 16; ++i)
	{
		_snprintf_s(hex + i * 2, 3, _TRUNCATE, "%02X", md5Bytes[i]);
	}
	hex[32] = '\0';

	outSignature.assign(hex);
	return true;
}

bool GunionCrypto::EncryptGunionGetLoginAreaParams3DesBase64(
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建明文参数（map 字典序，保持旧实现一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["device_id_server"] = deviceIdServer;

	std::string input;
	bool first = true;
	for (const auto& kv : params)
	{
		if (!first)
			input += '&';

		// value 必须 URL Encode
		input += kv.first;
		input += '=';
		input += UrlEncoder::Encode(kv.second);
		first = false;
	}

	// ------------------------------------------------------------
	// 2. 3DES(EDE3/ECB) 加密（IV 全 0，保持旧行为）
	// ------------------------------------------------------------
	static const unsigned char IV[8] = { 0 };

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (!ctx)
		return false;

	bool ok = false;

	if (EVP_EncryptInit(
		ctx,
		EVP_des_ede3_ecb(),
		reinterpret_cast<const unsigned char*>(randKey.data()),
		IV))
	{
		int outLen1 = 0;
		int bufLen = input.size() <= 4 ? 9 : static_cast<int>(input.size() * 2);
		std::vector<unsigned char> buffer(bufLen, 0);

		if (EVP_EncryptUpdate(
			ctx,
			buffer.data(),
			&outLen1,
			reinterpret_cast<const unsigned char*>(input.data()),
			static_cast<int>(input.size())))
		{
			int outLen2 = 0;
			if (EVP_EncryptFinal(ctx, buffer.data() + outLen1, &outLen2))
			{
				const int total = outLen1 + outLen2;
				std::string cipher(reinterpret_cast<char*>(buffer.data()), total);

				// Base64（与旧逻辑一致）
				outEncryptedParams = CSafeBase64::Encode(cipher);
				ok = true;
			}
		}
	}

	EVP_CIPHER_CTX_free(ctx);
	return ok;
}

bool GunionCrypto::CalcGunionGetLoginAreaSignatureMd5(
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建签名参数（与旧实现完全一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["device_id_server"] = deviceIdServer;

	// ------------------------------------------------------------
	// 2. key=value 排序（set）
	// ------------------------------------------------------------
	std::set<std::string> sorted;
	for (const auto& kv : params)
	{
		sorted.insert(kv.first + "=" + kv.second);
	}

	std::string raw;
	bool first = true;
	for (const auto& item : sorted)
	{
		if (!first)
			raw += '&';
		raw += item;
		first = false;
	}

	// ------------------------------------------------------------
	// 3. 拼接 randKey + tolower
	// ------------------------------------------------------------
	raw += randKey;
	std::transform(raw.begin(), raw.end(), raw.begin(), ::tolower);

	// ------------------------------------------------------------
	// 4. MD5 → 32 位大写 HEX
	// ------------------------------------------------------------
	unsigned char md5Bytes[16] = { 0 };
	MD5(reinterpret_cast<const unsigned char*>(raw.c_str()),
		raw.size(),
		md5Bytes);

	char hex[33] = { 0 };
	for (int i = 0; i < 16; ++i)
	{
		_snprintf_s(hex + i * 2, 3, _TRUNCATE, "%02X", md5Bytes[i]);
	}
	hex[32] = '\0';

	outSignature.assign(hex);
	return true;
}

bool GunionCrypto::EncryptGunionSpecialPwdLoginParams3DesBase64(
	const std::string& account,
	const std::string& password,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{

	// ------------------------------------------------------------
	// 1. 构建明文参数（map 字典序，保持旧实现一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["account"] = account;
	params["deviceid"] = windowsDeviceId;
	params["password"] = password;
	params["device_id_server"] = deviceIdServer;

	std::string input;
	bool first = true;
	for (const auto& kv : params)
	{
		if (!first)
			input += '&';

		// value 必须 URL Encode
		input += kv.first;
		input += '=';
		input += UrlEncoder::Encode(kv.second);
		first = false;
	}

	// ------------------------------------------------------------
	// 2. 3DES(EDE3/ECB) 加密（IV 全 0，保持旧行为）
	// ------------------------------------------------------------
	static const unsigned char IV[8] = { 0 };

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (!ctx)
		return false;

	bool ok = false;

	if (EVP_EncryptInit(
		ctx,
		EVP_des_ede3_ecb(),
		reinterpret_cast<const unsigned char*>(randKey.data()),
		IV))
	{
		int outLen1 = 0;
		int bufLen = input.size() <= 4 ? 9 : static_cast<int>(input.size() * 2);
		std::vector<unsigned char> buffer(bufLen, 0);

		if (EVP_EncryptUpdate(
			ctx,
			buffer.data(),
			&outLen1,
			reinterpret_cast<const unsigned char*>(input.data()),
			static_cast<int>(input.size())))
		{
			int outLen2 = 0;
			if (EVP_EncryptFinal(ctx, buffer.data() + outLen1, &outLen2))
			{
				const int total = outLen1 + outLen2;
				std::string cipher(reinterpret_cast<char*>(buffer.data()), total);

				// Base64（与旧逻辑一致）
				outEncryptedParams = CSafeBase64::Encode(cipher);
				ok = true;
			}
		}
	}

	EVP_CIPHER_CTX_free(ctx);
	return ok;
}

bool GunionCrypto::CalcGunionSpecialPwdLoginSignatureMd5(
	const std::string& account,
	const std::string& password,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{

	// ------------------------------------------------------------
	// 1. 构建签名参数（与旧实现完全一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["account"] = account;
	params["deviceid"] = windowsDeviceId;
	params["password"] = password;
	params["device_id_server"] = deviceIdServer;

	// ------------------------------------------------------------
	// 2. key=value 排序（set）
	// ------------------------------------------------------------
	std::set<std::string> sorted;
	for (const auto& kv : params)
	{
		sorted.insert(kv.first + "=" + kv.second);
	}

	std::string raw;
	bool first = true;
	for (const auto& item : sorted)
	{
		if (!first)
			raw += '&';
		raw += item;
		first = false;
	}

	// ------------------------------------------------------------
	// 3. 拼接 randKey + tolower
	// ------------------------------------------------------------
	raw += randKey;
	std::transform(raw.begin(), raw.end(), raw.begin(), ::tolower);

	// ------------------------------------------------------------
	// 4. MD5 → 32 位大写 HEX
	// ------------------------------------------------------------
	unsigned char md5Bytes[16] = { 0 };
	MD5(reinterpret_cast<const unsigned char*>(raw.c_str()),
		raw.size(),
		md5Bytes);

	char hex[33] = { 0 };
	for (int i = 0; i < 16; ++i)
	{
		_snprintf_s(hex + i * 2, 3, _TRUNCATE, "%02X", md5Bytes[i]);
	}
	hex[32] = '\0';

	outSignature.assign(hex);
	return true;
}

bool GunionCrypto::EncryptGunionSpecialPicPwdLoginParams3DesBase64(
	const std::string& checkCodeGuid,
	const std::string& outInfo,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{

	// ------------------------------------------------------------
	// 1. 构建明文参数（map 字典序，保持旧实现一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["deviceid"] = windowsDeviceId;
	params["checkCodeGuid"] = checkCodeGuid;
	params["outInfo"] = outInfo;
	params["device_id_server"] = deviceIdServer;

	std::string input;
	bool first = true;
	for (const auto& kv : params)
	{
		if (!first)
			input += '&';

		// value 必须 URL Encode
		input += kv.first;
		input += '=';
		input += UrlEncoder::Encode(kv.second);
		first = false;
	}

	// ------------------------------------------------------------
	// 2. 3DES(EDE3/ECB) 加密（IV 全 0，保持旧行为）
	// ------------------------------------------------------------
	static const unsigned char IV[8] = { 0 };

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (!ctx)
		return false;

	bool ok = false;

	if (EVP_EncryptInit(
		ctx,
		EVP_des_ede3_ecb(),
		reinterpret_cast<const unsigned char*>(randKey.data()),
		IV))
	{
		int outLen1 = 0;
		int bufLen = input.size() <= 4 ? 9 : static_cast<int>(input.size() * 2);
		std::vector<unsigned char> buffer(bufLen, 0);

		if (EVP_EncryptUpdate(
			ctx,
			buffer.data(),
			&outLen1,
			reinterpret_cast<const unsigned char*>(input.data()),
			static_cast<int>(input.size())))
		{
			int outLen2 = 0;
			if (EVP_EncryptFinal(ctx, buffer.data() + outLen1, &outLen2))
			{
				const int total = outLen1 + outLen2;
				std::string cipher(reinterpret_cast<char*>(buffer.data()), total);

				// Base64（与旧逻辑一致）
				outEncryptedParams = CSafeBase64::Encode(cipher);
				ok = true;
			}
		}
	}

	EVP_CIPHER_CTX_free(ctx);
	return ok;
}

bool GunionCrypto::CalcGunionSpecialPicPwdLoginSignatureMd5(
	const std::string& checkCodeGuid,
	const std::string& outInfo,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{

	// ------------------------------------------------------------
	// 1. 构建签名参数（与旧实现完全一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["deviceid"] = windowsDeviceId;
	params["checkCodeGuid"] = checkCodeGuid;
	params["outInfo"] = outInfo;
	params["device_id_server"] = deviceIdServer;

	// ------------------------------------------------------------
	// 2. key=value 排序（set）
	// ------------------------------------------------------------
	std::set<std::string> sorted;
	for (const auto& kv : params)
	{
		sorted.insert(kv.first + "=" + kv.second);
	}

	std::string raw;
	bool first = true;
	for (const auto& item : sorted)
	{
		if (!first)
			raw += '&';
		raw += item;
		first = false;
	}

	// ------------------------------------------------------------
	// 3. 拼接 randKey + tolower
	// ------------------------------------------------------------
	raw += randKey;
	std::transform(raw.begin(), raw.end(), raw.begin(), ::tolower);

	// ------------------------------------------------------------
	// 4. MD5 → 32 位大写 HEX
	// ------------------------------------------------------------
	unsigned char md5Bytes[16] = { 0 };
	MD5(reinterpret_cast<const unsigned char*>(raw.c_str()),
		raw.size(),
		md5Bytes);

	char hex[33] = { 0 };
	for (int i = 0; i < 16; ++i)
	{
		_snprintf_s(hex + i * 2, 3, _TRUNCATE, "%02X", md5Bytes[i]);
	}
	hex[32] = '\0';

	outSignature.assign(hex);
	return true;
}

bool GunionCrypto::EncryptGunionSpecialSmsSendParams3DesBase64(
	const std::string& account,
	const std::string& voiceMsg,
	const std::string& smsNew,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{

	// ------------------------------------------------------------
	// 1. 构建明文参数（必须与旧 CGAuthClient 完全一致）
	//    注意：旧代码使用 map → key 字典序
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["account"] = account;
	params["type"] = smsNew;
	params["voiceMsg"] = voiceMsg;
	params["deviceid"] = windowsDeviceId;
	params["device_id_server"] = deviceIdServer;

	std::string input;
	bool first = true;
	for (std::map<std::string, std::string>::const_iterator it = params.begin();
		it != params.end(); ++it)
	{
		if (!first)
			input += '&';

		// 必须 URL Encode value（与旧逻辑一致）
		input += it->first + "=" + UrlEncoder::Encode(it->second);
		first = false;
	}

	// ------------------------------------------------------------
	// 2. 3DES(EDE3/ECB) 加密
	// ------------------------------------------------------------
	static const unsigned char ENCRYPT_IV[8] = { 0 };

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (!ctx)
		return false;

	bool ret = false;

	if (EVP_EncryptInit(
		ctx,
		EVP_des_ede3_ecb(),
		reinterpret_cast<const unsigned char*>(randKey.data()),
		ENCRYPT_IV))
	{
		int outLen = 0;
		int bufLen = input.size() <= 4 ? 9 : static_cast<int>(input.size() * 2);

		std::vector<unsigned char> outBuf(bufLen, 0);

		if (EVP_EncryptUpdate(
			ctx,
			outBuf.data(),
			&outLen,
			reinterpret_cast<const unsigned char*>(input.data()),
			static_cast<int>(input.size())))
		{
			int tmpLen = 0;
			if (EVP_EncryptFinal(ctx, outBuf.data() + outLen, &tmpLen))
			{
				outLen += tmpLen;

				std::string cipher(reinterpret_cast<char*>(outBuf.data()), outLen);
				outEncryptedParams = CSafeBase64::Encode(cipher);
				ret = true;
			}
		}
	}

	EVP_CIPHER_CTX_free(ctx);
	return ret;
}

bool GunionCrypto::CalcGunionSpecialSmsSendSignatureMd5(
	const std::string& account,
	const std::string& voiceMsg,
	const std::string& smsNew,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{

	// ------------------------------------------------------------
	// 1. 构建签名参数（与旧 CGAuthClient 完全一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["account"] = account;
	params["type"] = smsNew;
	params["voiceMsg"] = voiceMsg;
	params["deviceid"] = windowsDeviceId;
	params["device_id_server"] = deviceIdServer;

	// ------------------------------------------------------------
	// 2. 按 key=value 排序（旧代码用 set<string>）
	// ------------------------------------------------------------
	std::set<std::string> sorted;
	for (std::map<std::string, std::string>::const_iterator it = params.begin();
		it != params.end(); ++it)
	{
		sorted.insert(it->first + "=" + it->second);
	}

	std::string raw;
	bool first = true;
	for (std::set<std::string>::const_iterator it = sorted.begin();
		it != sorted.end(); ++it)
	{
		if (!first)
			raw += '&';

		raw += *it;
		first = false;
	}

	// 3. 拼接 randKey
	raw += randKey;

	// 4. 转小写
	std::transform(raw.begin(), raw.end(), raw.begin(), ::tolower);

	// ------------------------------------------------------------
	// 5. MD5 计算
	// ------------------------------------------------------------
	unsigned char md5Bytes[16] = { 0 };
	MD5(reinterpret_cast<const unsigned char*>(raw.c_str()),
		raw.size(),
		md5Bytes);

	char hex[33] = { 0 };
	for (int i = 0; i < 16; ++i)
	{
		_snprintf_s(hex + i * 2, 3, _TRUNCATE, "%02X", md5Bytes[i]);
	}
	hex[32] = '\0';

	outSignature.assign(hex);
	return true;
}

bool GunionCrypto::EncryptGunionSpecialPicSmsSendParams3DesBase64(
	const std::string& captchaInfo,
	const std::string& smsSendGuid,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{

	// ------------------------------------------------------------
	// 1. 构建明文参数（必须与旧 CGAuthClient 完全一致）
	//    旧实现：map → key 字典序
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["smsSendGuid"] = smsSendGuid;
	params["outInfo"] = captchaInfo;
	params["device_id_server"] = deviceIdServer;

	std::string input;
	bool first = true;
	for (const auto& kv : params)
	{
		if (!first)
			input += '&';

		// 与旧逻辑一致：仅 value 做 URL Encode
		input += kv.first;
		input += '=';
		input += UrlEncoder::Encode(kv.second);
		first = false;
	}

	// ------------------------------------------------------------
	// 2. 3DES(EDE3/ECB) 加密（保持旧 buffer 规则）
	// ------------------------------------------------------------
	static const unsigned char IV[8] = { 0 };

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (!ctx)
		return false;

	bool ok = false;
	do
	{
		if (!EVP_EncryptInit(
			ctx,
			EVP_des_ede3_ecb(),
			reinterpret_cast<const unsigned char*>(randKey.data()),
			IV))
			break;

		int bufLen = input.size() <= 4
			? 9
			: static_cast<int>(input.size() * 2);

		std::vector<unsigned char> buffer(bufLen, 0);

		int len1 = 0;
		if (!EVP_EncryptUpdate(
			ctx,
			buffer.data(),
			&len1,
			reinterpret_cast<const unsigned char*>(input.data()),
			static_cast<int>(input.size())))
			break;

		int len2 = 0;
		if (!EVP_EncryptFinal(ctx, buffer.data() + len1, &len2))
			break;

		const int outLen = len1 + len2;
		std::string cipher(reinterpret_cast<char*>(buffer.data()), outLen);

		// Base64（与旧实现一致）
		outEncryptedParams = CSafeBase64::Encode(cipher);
		ok = true;

	} while (false);

	EVP_CIPHER_CTX_free(ctx);
	return ok;
}

bool GunionCrypto::CalcGunionSpecialPicSmsSendSignatureMd5(
	const std::string& captchaInfo,
	const std::string& smsSendGuid,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{

	// ------------------------------------------------------------
	// 1. 构建签名参数（与旧 CGAuthClient 完全一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["smsSendGuid"] = smsSendGuid;
	params["outInfo"] = captchaInfo;
	params["device_id_server"] = deviceIdServer;

	// ------------------------------------------------------------
	// 2. 排序 key=value（旧实现使用 set）
	// ------------------------------------------------------------
	std::set<std::string> sorted;
	for (const auto& kv : params)
	{
		sorted.insert(kv.first + "=" + kv.second);
	}

	std::string raw;
	bool first = true;
	for (const auto& item : sorted)
	{
		if (!first)
			raw += '&';

		raw += item;
		first = false;
	}

	// 3. 拼接 randKey
	raw += randKey;

	// 4. 转小写（保持旧行为）
	std::transform(raw.begin(), raw.end(), raw.begin(), ::tolower);

	// ------------------------------------------------------------
	// 5. MD5 计算 → 32 位大写 HEX
	// ------------------------------------------------------------
	unsigned char md5Bytes[16] = { 0 };
	MD5(
		reinterpret_cast<const unsigned char*>(raw.c_str()),
		raw.size(),
		md5Bytes);

	char hex[33] = { 0 };
	for (int i = 0; i < 16; ++i)
	{
		_snprintf_s(hex + i * 2, 3, _TRUNCATE, "%02X", md5Bytes[i]);
	}
	hex[32] = '\0';

	outSignature.assign(hex);
	return true;
}

bool GunionCrypto::EncryptGunionSpecialConfirmSmsSendParams3DesBase64(
	const std::string& smsSendGuid,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{

	// ------------------------------------------------------------
	// 1. 构建明文参数（必须与旧 CGAuthClient 完全一致）
	//    旧实现：map → key 字典序
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["smsSendGuid"] = smsSendGuid;
	params["device_id_server"] = deviceIdServer;

	std::string input;
	bool first = true;
	for (const auto& kv : params)
	{
		if (!first)
			input += '&';

		// 与旧逻辑一致：仅 value 做 URL Encode
		input += kv.first;
		input += '=';
		input += UrlEncoder::Encode(kv.second);
		first = false;
	}

	// ------------------------------------------------------------
	// 2. 3DES(EDE3/ECB) 加密（保持旧 buffer 规则）
	// ------------------------------------------------------------
	static const unsigned char IV[8] = { 0 };

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (!ctx)
		return false;

	bool ok = false;
	do
	{
		if (!EVP_EncryptInit(
			ctx,
			EVP_des_ede3_ecb(),
			reinterpret_cast<const unsigned char*>(randKey.data()),
			IV))
			break;

		int bufLen = input.size() <= 4
			? 9
			: static_cast<int>(input.size() * 2);

		std::vector<unsigned char> buffer(bufLen, 0);

		int len1 = 0;
		if (!EVP_EncryptUpdate(
			ctx,
			buffer.data(),
			&len1,
			reinterpret_cast<const unsigned char*>(input.data()),
			static_cast<int>(input.size())))
			break;

		int len2 = 0;
		if (!EVP_EncryptFinal(ctx, buffer.data() + len1, &len2))
			break;

		const int outLen = len1 + len2;
		std::string cipher(reinterpret_cast<char*>(buffer.data()), outLen);

		// Base64（与旧实现一致）
		outEncryptedParams = CSafeBase64::Encode(cipher);
		ok = true;

	} while (false);

	EVP_CIPHER_CTX_free(ctx);
	return ok;
}

bool GunionCrypto::CalcGunionSpecialConfirmSmsSendSignatureMd5(
	const std::string& smsSendGuid,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{

	// ------------------------------------------------------------
	// 1. 构建签名参数（与旧 CGAuthClient 完全一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["smsSendGuid"] = smsSendGuid;
	params["device_id_server"] = deviceIdServer;

	// ------------------------------------------------------------
	// 2. 排序 key=value（旧实现使用 set）
	// ------------------------------------------------------------
	std::set<std::string> sorted;
	for (const auto& kv : params)
	{
		sorted.insert(kv.first + "=" + kv.second);
	}

	std::string raw;
	bool first = true;
	for (const auto& item : sorted)
	{
		if (!first)
			raw += '&';

		raw += item;
		first = false;
	}

	// 3. 拼接 randKey
	raw += randKey;

	// 4. 转小写（保持旧行为）
	std::transform(raw.begin(), raw.end(), raw.begin(), ::tolower);

	// ------------------------------------------------------------
	// 5. MD5 计算 → 32 位大写 HEX
	// ------------------------------------------------------------
	unsigned char md5Bytes[16] = { 0 };
	MD5(
		reinterpret_cast<const unsigned char*>(raw.c_str()),
		raw.size(),
		md5Bytes);

	char hex[33] = { 0 };
	for (int i = 0; i < 16; ++i)
	{
		_snprintf_s(hex + i * 2, 3, _TRUNCATE, "%02X", md5Bytes[i]);
	}
	hex[32] = '\0';

	outSignature.assign(hex);
	return true;
}

bool GunionCrypto::EncryptGunionSpecialCheckSmsLoginParams3DesBase64(
	const std::string& smsSendGuid,
	const std::string& smsCode,
	const std::string& account,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建明文参数（必须与旧 CGAuthClient 完全一致）
	//    旧实现：map → key 字典序
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["smsSendGuid"] = smsSendGuid;
	params["account"] = account;
	params["smscode"] = smsCode;
	params["deviceid"] = windowsDeviceId;
	params["device_id_server"] = deviceIdServer;

	std::string input;
	bool first = true;
	for (const auto& kv : params)
	{
		if (!first)
			input += '&';

		// 与旧逻辑一致：仅 value 做 URL Encode
		input += kv.first;
		input += '=';
		input += UrlEncoder::Encode(kv.second);
		first = false;
	}

	// ------------------------------------------------------------
	// 2. 3DES(EDE3/ECB) 加密（保持旧 buffer 规则）
	// ------------------------------------------------------------
	static const unsigned char IV[8] = { 0 };

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (!ctx)
		return false;

	bool ok = false;
	do
	{
		if (!EVP_EncryptInit(
			ctx,
			EVP_des_ede3_ecb(),
			reinterpret_cast<const unsigned char*>(randKey.data()),
			IV))
			break;

		int bufLen = input.size() <= 4
			? 9
			: static_cast<int>(input.size() * 2);

		std::vector<unsigned char> buffer(bufLen, 0);

		int len1 = 0;
		if (!EVP_EncryptUpdate(
			ctx,
			buffer.data(),
			&len1,
			reinterpret_cast<const unsigned char*>(input.data()),
			static_cast<int>(input.size())))
			break;

		int len2 = 0;
		if (!EVP_EncryptFinal(ctx, buffer.data() + len1, &len2))
			break;

		const int outLen = len1 + len2;
		std::string cipher(reinterpret_cast<char*>(buffer.data()), outLen);

		// Base64（与旧实现一致）
		outEncryptedParams = CSafeBase64::Encode(cipher);
		ok = true;

	} while (false);

	EVP_CIPHER_CTX_free(ctx);
	return ok;
}

bool GunionCrypto::CalcGunionSpecialCheckSmsLoginSignatureMd5(
	const std::string& smsSendGuid,
	const std::string& smsCode,
	const std::string& account,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建签名参数（与旧 CGAuthClient 完全一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["smsSendGuid"] = smsSendGuid;
	params["account"] = account;
	params["smscode"] = smsCode;
	params["deviceid"] = windowsDeviceId;
	params["device_id_server"] = deviceIdServer;

	// ------------------------------------------------------------
	// 2. 排序 key=value（旧实现使用 set）
	// ------------------------------------------------------------
	std::set<std::string> sorted;
	for (const auto& kv : params)
	{
		sorted.insert(kv.first + "=" + kv.second);
	}

	std::string raw;
	bool first = true;
	for (const auto& item : sorted)
	{
		if (!first)
			raw += '&';

		raw += item;
		first = false;
	}

	// 3. 拼接 randKey
	raw += randKey;

	// 4. 转小写（保持旧行为）
	std::transform(raw.begin(), raw.end(), raw.begin(), ::tolower);

	// ------------------------------------------------------------
	// 5. MD5 计算 → 32 位大写 HEX
	// ------------------------------------------------------------
	unsigned char md5Bytes[16] = { 0 };
	MD5(
		reinterpret_cast<const unsigned char*>(raw.c_str()),
		raw.size(),
		md5Bytes);

	char hex[33] = { 0 };
	for (int i = 0; i < 16; ++i)
	{
		_snprintf_s(hex + i * 2, 3, _TRUNCATE, "%02X", md5Bytes[i]);
	}
	hex[32] = '\0';

	outSignature.assign(hex);
	return true;
}

bool GunionCrypto::EncryptGunionPayEntranceParams3DesBase64(
	const std::string& orderId,
	const std::string& productId,
	const std::string& groupId,
	const std::string& areaId,
	const std::string& extend,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建明文参数（必须与旧 CGAuthClient 完全一致）
	//    旧实现：map → key 字典序
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["areaid"] = areaId;
	params["productid"] = productId;
	params["gameorder"] = orderId;
	params["extend"] = extend;
	params["groupid"] = groupId;

	std::string input;
	bool first = true;
	for (const auto& kv : params)
	{
		if (!first)
			input += '&';

		// 与旧逻辑一致：仅 value 做 URL Encode
		input += kv.first;
		input += '=';
		input += UrlEncoder::Encode(kv.second);
		first = false;
	}

	// ------------------------------------------------------------
	// 2. 3DES(EDE3/ECB) 加密（保持旧 buffer 规则）
	// ------------------------------------------------------------
	static const unsigned char IV[8] = { 0 };

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (!ctx)
		return false;

	bool ok = false;
	do
	{
		if (!EVP_EncryptInit(
			ctx,
			EVP_des_ede3_ecb(),
			reinterpret_cast<const unsigned char*>(randKey.data()),
			IV))
			break;

		int bufLen = input.size() <= 4
			? 9
			: static_cast<int>(input.size() * 2);

		std::vector<unsigned char> buffer(bufLen, 0);

		int len1 = 0;
		if (!EVP_EncryptUpdate(
			ctx,
			buffer.data(),
			&len1,
			reinterpret_cast<const unsigned char*>(input.data()),
			static_cast<int>(input.size())))
			break;

		int len2 = 0;
		if (!EVP_EncryptFinal(ctx, buffer.data() + len1, &len2))
			break;

		const int outLen = len1 + len2;
		std::string cipher(reinterpret_cast<char*>(buffer.data()), outLen);

		// Base64（与旧实现一致）
		outEncryptedParams = CSafeBase64::Encode(cipher);
		ok = true;

	} while (false);

	EVP_CIPHER_CTX_free(ctx);
	return ok;
}

bool GunionCrypto::CalcGunionPayEntranceSignatureMd5(
	const std::string& orderId,
	const std::string& productId,
	const std::string& groupId,
	const std::string& areaId,
	const std::string& extend,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建签名参数（与旧 CGAuthClient 完全一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["areaid"] = areaId;
	params["productid"] = productId;
	params["gameorder"] = orderId;
	params["extend"] = extend;
	params["groupid"] = groupId;

	// ------------------------------------------------------------
	// 2. 排序 key=value（旧实现使用 set）
	// ------------------------------------------------------------
	std::set<std::string> sorted;
	for (const auto& kv : params)
	{
		sorted.insert(kv.first + "=" + kv.second);
	}

	std::string raw;
	bool first = true;
	for (const auto& item : sorted)
	{
		if (!first)
			raw += '&';

		raw += item;
		first = false;
	}

	// 3. 拼接 randKey
	raw += randKey;

	// 4. 转小写（保持旧行为）
	std::transform(raw.begin(), raw.end(), raw.begin(), ::tolower);

	// ------------------------------------------------------------
	// 5. MD5 计算 → 32 位大写 HEX
	// ------------------------------------------------------------
	unsigned char md5Bytes[16] = { 0 };
	MD5(
		reinterpret_cast<const unsigned char*>(raw.c_str()),
		raw.size(),
		md5Bytes);

	char hex[33] = { 0 };
	for (int i = 0; i < 16; ++i)
	{
		_snprintf_s(hex + i * 2, 3, _TRUNCATE, "%02X", md5Bytes[i]);
	}
	hex[32] = '\0';

	outSignature.assign(hex);
	return true;
}

bool GunionCrypto::EncryptGunionCheckPayOrderStatusParams3DesBase64(
	const std::string& orderId,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建明文参数（必须与旧 CGAuthClient 完全一致）
	//    旧实现：map → key 字典序
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["orderid"] = orderId;

	std::string input;
	bool first = true;
	for (const auto& kv : params)
	{
		if (!first)
			input += '&';

		// 与旧逻辑一致：仅 value 做 URL Encode
		input += kv.first;
		input += '=';
		input += UrlEncoder::Encode(kv.second);
		first = false;
	}

	// ------------------------------------------------------------
	// 2. 3DES(EDE3/ECB) 加密（保持旧 buffer 规则）
	// ------------------------------------------------------------
	static const unsigned char IV[8] = { 0 };

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (!ctx)
		return false;

	bool ok = false;
	do
	{
		if (!EVP_EncryptInit(
			ctx,
			EVP_des_ede3_ecb(),
			reinterpret_cast<const unsigned char*>(randKey.data()),
			IV))
			break;

		int bufLen = input.size() <= 4
			? 9
			: static_cast<int>(input.size() * 2);

		std::vector<unsigned char> buffer(bufLen, 0);

		int len1 = 0;
		if (!EVP_EncryptUpdate(
			ctx,
			buffer.data(),
			&len1,
			reinterpret_cast<const unsigned char*>(input.data()),
			static_cast<int>(input.size())))
			break;

		int len2 = 0;
		if (!EVP_EncryptFinal(ctx, buffer.data() + len1, &len2))
			break;

		const int outLen = len1 + len2;
		std::string cipher(reinterpret_cast<char*>(buffer.data()), outLen);

		// Base64（与旧实现一致）
		outEncryptedParams = CSafeBase64::Encode(cipher);
		ok = true;

	} while (false);

	EVP_CIPHER_CTX_free(ctx);
	return ok;
}

bool GunionCrypto::CalcGunionCheckPayOrderStatusSignatureMd5(
	const std::string& orderId,
	const std::string& randKey,
	std::string& outSignature)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建签名参数（与旧 CGAuthClient 完全一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["orderid"] = orderId;

	// ------------------------------------------------------------
	// 2. 排序 key=value（旧实现使用 set）
	// ------------------------------------------------------------
	std::set<std::string> sorted;
	for (const auto& kv : params)
	{
		sorted.insert(kv.first + "=" + kv.second);
	}

	std::string raw;
	bool first = true;
	for (const auto& item : sorted)
	{
		if (!first)
			raw += '&';

		raw += item;
		first = false;
	}

	// 3. 拼接 randKey
	raw += randKey;

	// 4. 转小写（保持旧行为）
	std::transform(raw.begin(), raw.end(), raw.begin(), ::tolower);

	// ------------------------------------------------------------
	// 5. MD5 计算 → 32 位大写 HEX
	// ------------------------------------------------------------
	unsigned char md5Bytes[16] = { 0 };
	MD5(
		reinterpret_cast<const unsigned char*>(raw.c_str()),
		raw.size(),
		md5Bytes);

	char hex[33] = { 0 };
	for (int i = 0; i < 16; ++i)
	{
		_snprintf_s(hex + i * 2, 3, _TRUNCATE, "%02X", md5Bytes[i]);
	}
	hex[32] = '\0';

	outSignature.assign(hex);
	return true;
}

bool GunionCrypto::EncryptGunionCheckActivationParams3DesBase64(
	const std::string& activation,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建明文参数（必须与旧 CGAuthClient 完全一致）
	//    旧实现：map → key 字典序
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["activation"] = activation;
	params["device_id_server"] = deviceIdServer;

	std::string input;
	bool first = true;
	for (const auto& kv : params)
	{
		if (!first)
			input += '&';

		// 与旧逻辑一致：仅 value 做 URL Encode
		input += kv.first;
		input += '=';
		input += UrlEncoder::Encode(kv.second);
		first = false;
	}

	// ------------------------------------------------------------
	// 2. 3DES(EDE3/ECB) 加密（保持旧 buffer 规则）
	// ------------------------------------------------------------
	static const unsigned char IV[8] = { 0 };

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (!ctx)
		return false;

	bool ok = false;
	do
	{
		if (!EVP_EncryptInit(
			ctx,
			EVP_des_ede3_ecb(),
			reinterpret_cast<const unsigned char*>(randKey.data()),
			IV))
			break;

		int bufLen = input.size() <= 4
			? 9
			: static_cast<int>(input.size() * 2);

		std::vector<unsigned char> buffer(bufLen, 0);

		int len1 = 0;
		if (!EVP_EncryptUpdate(
			ctx,
			buffer.data(),
			&len1,
			reinterpret_cast<const unsigned char*>(input.data()),
			static_cast<int>(input.size())))
			break;

		int len2 = 0;
		if (!EVP_EncryptFinal(ctx, buffer.data() + len1, &len2))
			break;

		const int outLen = len1 + len2;
		std::string cipher(reinterpret_cast<char*>(buffer.data()), outLen);

		// Base64（与旧实现一致）
		outEncryptedParams = CSafeBase64::Encode(cipher);
		ok = true;

	} while (false);

	EVP_CIPHER_CTX_free(ctx);
	return ok;
}

bool GunionCrypto::CalcGunionCheckActivationSignatureMd5(
	const std::string& activation,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建签名参数（与旧 CGAuthClient 完全一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["activation"] = activation;
	params["device_id_server"] = deviceIdServer;

	// ------------------------------------------------------------
	// 2. 排序 key=value（旧实现使用 set）
	// ------------------------------------------------------------
	std::set<std::string> sorted;
	for (const auto& kv : params)
	{
		sorted.insert(kv.first + "=" + kv.second);
	}

	std::string raw;
	bool first = true;
	for (const auto& item : sorted)
	{
		if (!first)
			raw += '&';

		raw += item;
		first = false;
	}

	// 3. 拼接 randKey
	raw += randKey;

	// 4. 转小写（保持旧行为）
	std::transform(raw.begin(), raw.end(), raw.begin(), ::tolower);

	// ------------------------------------------------------------
	// 5. MD5 计算 → 32 位大写 HEX
	// ------------------------------------------------------------
	unsigned char md5Bytes[16] = { 0 };
	MD5(
		reinterpret_cast<const unsigned char*>(raw.c_str()),
		raw.size(),
		md5Bytes);

	char hex[33] = { 0 };
	for (int i = 0; i < 16; ++i)
	{
		_snprintf_s(hex + i * 2, 3, _TRUNCATE, "%02X", md5Bytes[i]);
	}
	hex[32] = '\0';

	outSignature.assign(hex);
	return true;
}

bool GunionCrypto::EncryptGunionSetTutorInfoParams3DesBase64(
	const std::string& idcard,
	const std::string& name,
	const std::string& phone,
	const std::string& smscode,
	const std::string& confirmKey,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建明文参数（必须与旧 CGAuthClient 完全一致）
	//    旧实现：map → key 字典序
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["idcard"] = idcard;
	params["name"] = name;
	params["phone"] = phone;
	params["smscode"] = smscode;
	params["confirmKey"] = confirmKey;
	params["device_id_server"] = deviceIdServer;

	std::string input;
	bool first = true;
	for (const auto& kv : params)
	{
		if (!first)
			input += '&';

		// 与旧逻辑一致：仅 value 做 URL Encode
		input += kv.first;
		input += '=';
		input += UrlEncoder::Encode(kv.second);
		first = false;
	}

	// ------------------------------------------------------------
	// 2. 3DES(EDE3/ECB) 加密（保持旧 buffer 规则）
	// ------------------------------------------------------------
	static const unsigned char IV[8] = { 0 };

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (!ctx)
		return false;

	bool ok = false;
	do
	{
		if (!EVP_EncryptInit(
			ctx,
			EVP_des_ede3_ecb(),
			reinterpret_cast<const unsigned char*>(randKey.data()),
			IV))
			break;

		int bufLen = input.size() <= 4
			? 9
			: static_cast<int>(input.size() * 2);

		std::vector<unsigned char> buffer(bufLen, 0);

		int len1 = 0;
		if (!EVP_EncryptUpdate(
			ctx,
			buffer.data(),
			&len1,
			reinterpret_cast<const unsigned char*>(input.data()),
			static_cast<int>(input.size())))
			break;

		int len2 = 0;
		if (!EVP_EncryptFinal(ctx, buffer.data() + len1, &len2))
			break;

		const int outLen = len1 + len2;
		std::string cipher(reinterpret_cast<char*>(buffer.data()), outLen);

		// Base64（与旧实现一致）
		outEncryptedParams = CSafeBase64::Encode(cipher);
		ok = true;

	} while (false);

	EVP_CIPHER_CTX_free(ctx);
	return ok;
}

bool GunionCrypto::CalcGunionSetTutorInfoSignatureMd5(
	const std::string& idcard,
	const std::string& name,
	const std::string& phone,
	const std::string& smscode,
	const std::string& confirmKey,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建签名参数（与旧 CGAuthClient 完全一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["idcard"] = idcard;
	params["name"] = name;
	params["phone"] = phone;
	params["smscode"] = smscode;
	params["confirmKey"] = confirmKey;
	params["device_id_server"] = deviceIdServer;

	// ------------------------------------------------------------
	// 2. 排序 key=value（旧实现使用 set）
	// ------------------------------------------------------------
	std::set<std::string> sorted;
	for (const auto& kv : params)
	{
		sorted.insert(kv.first + "=" + kv.second);
	}

	std::string raw;
	bool first = true;
	for (const auto& item : sorted)
	{
		if (!first)
			raw += '&';

		raw += item;
		first = false;
	}

	// 3. 拼接 randKey
	raw += randKey;

	// 4. 转小写（保持旧行为）
	std::transform(raw.begin(), raw.end(), raw.begin(), ::tolower);

	// ------------------------------------------------------------
	// 5. MD5 计算 → 32 位大写 HEX
	// ------------------------------------------------------------
	unsigned char md5Bytes[16] = { 0 };
	MD5(
		reinterpret_cast<const unsigned char*>(raw.c_str()),
		raw.size(),
		md5Bytes);

	char hex[33] = { 0 };
	for (int i = 0; i < 16; ++i)
	{
		_snprintf_s(hex + i * 2, 3, _TRUNCATE, "%02X", md5Bytes[i]);
	}
	hex[32] = '\0';

	outSignature.assign(hex);
	return true;
}

bool GunionCrypto::EncryptGunionWegameChannelLoginParams3DesBase64(
	const std::string& selectLoginContext,
	const std::string& companyId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建明文参数（必须与旧 CGAuthClient 完全一致）
	//    旧实现：map → key 字典序
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["device_id_server"] = deviceIdServer;
	params["selectLoginContext"] = selectLoginContext;
	params["companyId"] = companyId;

	std::string input;
	bool first = true;
	for (const auto& kv : params)
	{
		if (!first)
			input += '&';

		// 与旧逻辑一致：仅 value 做 URL Encode
		input += kv.first;
		input += '=';
		input += UrlEncoder::Encode(kv.second);
		first = false;
	}

	// ------------------------------------------------------------
	// 2. 3DES(EDE3/ECB) 加密（保持旧 buffer 规则）
	// ------------------------------------------------------------
	static const unsigned char IV[8] = { 0 };

	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (!ctx)
		return false;

	bool ok = false;
	do
	{
		if (!EVP_EncryptInit(
			ctx,
			EVP_des_ede3_ecb(),
			reinterpret_cast<const unsigned char*>(randKey.data()),
			IV))
			break;

		int bufLen = input.size() <= 4
			? 9
			: static_cast<int>(input.size() * 2);

		std::vector<unsigned char> buffer(bufLen, 0);

		int len1 = 0;
		if (!EVP_EncryptUpdate(
			ctx,
			buffer.data(),
			&len1,
			reinterpret_cast<const unsigned char*>(input.data()),
			static_cast<int>(input.size())))
			break;

		int len2 = 0;
		if (!EVP_EncryptFinal(ctx, buffer.data() + len1, &len2))
			break;

		const int outLen = len1 + len2;
		std::string cipher(reinterpret_cast<char*>(buffer.data()), outLen);

		// Base64（与旧实现一致）
		outEncryptedParams = CSafeBase64::Encode(cipher);
		ok = true;

	} while (false);

	EVP_CIPHER_CTX_free(ctx);
	return ok;
}

bool GunionCrypto::CalcGunionWegameChannelLoginSignatureMd5(
	const std::string& selectLoginContext,
	const std::string& companyId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 1. 构建签名参数（与旧 CGAuthClient 完全一致）
	// ------------------------------------------------------------
	std::map<std::string, std::string> params;
	params["device_id_server"] = deviceIdServer;
	params["selectLoginContext"] = selectLoginContext;
	params["companyId"] = companyId;

	// ------------------------------------------------------------
	// 2. 排序 key=value（旧实现使用 set）
	// ------------------------------------------------------------
	std::set<std::string> sorted;
	for (const auto& kv : params)
	{
		sorted.insert(kv.first + "=" + kv.second);
	}

	std::string raw;
	bool first = true;
	for (const auto& item : sorted)
	{
		if (!first)
			raw += '&';

		raw += item;
		first = false;
	}

	// 3. 拼接 randKey
	raw += randKey;

	// 4. 转小写（保持旧行为）
	std::transform(raw.begin(), raw.end(), raw.begin(), ::tolower);

	// ------------------------------------------------------------
	// 5. MD5 计算 → 32 位大写 HEX
	// ------------------------------------------------------------
	unsigned char md5Bytes[16] = { 0 };
	MD5(
		reinterpret_cast<const unsigned char*>(raw.c_str()),
		raw.size(),
		md5Bytes);

	char hex[33] = { 0 };
	for (int i = 0; i < 16; ++i)
	{
		_snprintf_s(hex + i * 2, 3, _TRUNCATE, "%02X", md5Bytes[i]);
	}
	hex[32] = '\0';

	outSignature.assign(hex);
	return true;
}