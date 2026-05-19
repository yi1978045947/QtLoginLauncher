#pragma once
#include <string>

/**
 * @brief Gunion 初始化阶段（WeGame Init）加密 / 签名算法
 *
 * 说明：
 *   - 迁移自 CGAuthClient
 *   - 仅用于 /v1/account/initialize 初始化接口
 *   - LoginClient 负责提供完整参数
 */
class GunionCrypto
{
public:
	// ============================================================
	// 原有接口（必须保留）
	// ============================================================

	static bool Decrypt3DesBase64(
		const std::string& base64Cipher,
		const std::string& key,
		std::string& outPlain);

	// ============================================================
	// 新增接口：GunionWeGame Init 专属（参数全集）
	// ============================================================

	/**
	 * @brief WeGame 初始化接口：3DES(EDE3/ECB) 加密参数
	 *
	 * 等价于：
	 *   CGAuthClient::EncrytParams3DesCStyleDoInit
	 */
	static bool EncryptWegameInitParams3DesBase64(
		const std::string& deviceIdServer,
		const std::string& mac,
		const std::string& windowsDeviceId,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief GunionWeGame 初始化接口：计算请求签名（MD5）
	 *
	 * 等价于：
	 *   CGAuthClient::CalcSignatureCStypeDoInit
	 */
	static bool CalcWegameInitSignatureMd5(
		const std::string& deviceIdServer,
		const std::string& mac,
		const std::string& windowsDeviceId,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// GunionWeGame Login 专属（登录接口）
	// ============================================================

	/**
	 * @brief GunionWeGame 登录接口：3DES(EDE3/ECB) 加密参数
	 *
	 * 等价于：
	 *   CGAuthClient::EncrytParams3DesCStyleWegameLogin
	 *
	 * 对应接口：
	 *   /v1/cooperation/wegameLogin
	 */
	static bool EncryptWegameLoginParams3DesBase64(
		const std::string& deviceId,
		const std::string& deviceIdServer,
		const std::string& thirdToken,
		const std::string& thirdUserId,
		const std::string& isLimited,
		const std::string& companyId,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief WeGame 登录接口：计算请求签名（MD5）
	 *
	 * 等价于：
	 *   CGAuthClient::CalcSignatureCStypeWegameLogin
	 */
	static bool CalcWegameLoginSignatureMd5(
		const std::string& deviceId,
		const std::string& deviceIdServer,
		const std::string& thirdToken,
		const std::string& thirdUserId,
		const std::string& isLimited,
		const std::string& companyId,
		const std::string& randKey,
		std::string& outSignature);


	// ============================================================
	// GunionWeGame SmsSend 专属（短信发送接口）
	// ============================================================

	/**
	 * @brief GunionWeGame 短信发送接口：3DES(EDE3/ECB) 加密参数
	 *
	 * 等价于：
	 *   CGAuthClient::EncrytParams3DesCStyleWegameSmsSend
	 *
	 * 对应接口：
	 *   /v1/basic/smssend
	 *
	 * 协议参数说明：
	 *   - phone               手机号码
	 *   - type                短信类型（如："5"，由旧协议约定）
	 *   - supportPic          是否支持图片验证码（"0" / "1"）
	 *   - voiceMsg            语音提示文案（允许为空）
	 *   - sms_new             是否使用新短信流程（"0" / "1"）
	 *   - deviceid            客户端设备标识
	 *   - device_id_server    服务端下发的新设备标识
	 */
	static bool EncryptWegameSmsSendParams3DesBase64(
		const std::string& phone,
		const std::string& type,
		const std::string& supportPic,
		const std::string& voiceMsg,
		const std::string& smsNew,
		const std::string& deviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief GunionWeGame 短信发送接口：计算请求签名（MD5）
	 *
	 * 等价于：
	 *   CGAuthClient::CalcSignatureCStypeWegameSmsSend
	 *
	 * 对应接口：
	 *   /v1/basic/smssend
	 *
	 * 签名规则：
	 *   1. 将所有参与签名的参数按 key 字典序排序
	 *   2. 使用 key=value 形式拼接，中间以 '&' 分隔
	 *   3. 拼接当前 Gunion 会话 randKey
	 *   4. 转换为小写
	 *   5. 计算 MD5，输出 32 位大写 HEX 字符串
	 */
	static bool CalcWegameSmsSendSignatureMd5(
		const std::string& phone,
		const std::string& type,
		const std::string& supportPic,
		const std::string& voiceMsg,
		const std::string& smsNew,
		const std::string& deviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// WeGame PicSmsSend 专属（带图形验证码短信发送接口）
	// ============================================================

	/**
	 * @brief WeGame 带图形验证码短信发送接口：3DES(EDE3/ECB) 加密参数
	 *
	 * 等价于：
	 *   CGAuthClient::EncrytParams3DesCStyleWegameCheckPicSend
	 *
	 * 对应接口：
	 *   /v1/basic/picCheckSmsSend2
	 */
	static bool EncryptWegamePicSmsSendParams3DesBase64(
		const std::string& phone,
		const std::string& type,
		const std::string& checkCodeGuid,
		const std::string& check_code,
		const std::string& outInfo,
		const std::string& voiceMsg,
		const std::string& smsNew,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief WeGame 带图形验证码短信发送接口：计算请求签名（MD5）
	 *
	 * 等价于：
	 *   CGAuthClient::CalcSignatureCStypeWegameCheckPicSend
	 *
	 * 签名规则：
	 *   - 参数按 key 排序
	 *   - key=value 拼接
	 *   - 拼接 randKey
	 *   - 转小写
	 *   - MD5 → 32 位大写 HEX
	 */
	static bool CalcWegamePicSmsSendSignatureMd5(
		const std::string& phone,
		const std::string& type,
		const std::string& checkCodeGuid,
		const std::string& check_code,
		const std::string& outInfo,
		const std::string& voiceMsg,
		const std::string& smsNew,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// WeGame ThirdAccountBindPhone 专属（三方账号绑定手机号）
	// ============================================================

	/**
	 * @brief WeGame 三方账号绑定手机号接口：3DES(EDE3/ECB) 加密参数
	 *
	 * 等价于：
	 *   CGAuthClient::EncrytParams3DesCStyleWegameThirdAccountBindPhone
	 *
	 * 对应接口：
	 *   /v1/account/thirdAccountBindPhone
	 */
	static bool EncryptWegameThirdAccountBindPhoneParams3DesBase64(
		const std::string& deviceId,
		const std::string& phone,
		const std::string& smscode,
		const std::string& type,
		const std::string& scene,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief WeGame 三方账号绑定手机号接口：计算请求签名（MD5）
	 *
	 * 等价于：
	 *   CGAuthClient::CalcSignatureCStypeWegameThirdAccountBindPhone
	 *
	 * 签名规则：
	 *   - 参数按 key 字典序排序
	 *   - key=value 拼接
	 *   - 拼接 randKey
	 *   - 转小写
	 *   - MD5 → 32 位大写 HEX
	 */
	static bool CalcWegameThirdAccountBindPhoneSignatureMd5(
		const std::string& deviceId,
		const std::string& phone,
		const std::string& smscode,
		const std::string& type,
		const std::string& scene,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// WeGame FillRealinfo（实名补填接口）
	// ============================================================

	/**
	 * @brief WeGame 实名补填接口：3DES(EDE3/ECB) 加密参数
	 *
	 * 等价于：
	 *   CGAuthClient::EncrytParams3DesCStyleWegameFillRealinfo
	 *
	 * 对应接口：
	 *   /v1/account/fillrealinfo
	 */
	static bool EncryptWegameFillRealinfoParams3DesBase64(
		const std::string& idcard,
		const std::string& nameUtf8,
		const std::string& realInfoFlowId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief WeGame 实名补填接口：计算请求签名（MD5）
	 *
	 * 等价于：
	 *   CGAuthClient::CalcSignatureCStyleWegameFillRealinfo
	 */
	static bool CalcWegameFillRealinfoSignatureMd5(
		const std::string& idcard,
		const std::string& nameUtf8,
		const std::string& realInfoFlowId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// WeGame GetTicket / GetPayTicket
	// ============================================================

	/**
	 * @brief WeGame 获取票据接口：3DES(EDE3/ECB) 加密参数
	 *
	 * 等价于：
	 *   CGAuthClient::EncrytParams3DesCStyleWegameGetTicket
	 *
	 * 对应接口：
	 *   /v1/open/getticket2
	 */
	static bool EncryptWegameGetTicketParams3DesBase64(
		int time_s,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief WeGame 获取票据接口：计算请求签名（MD5）
	 *
	 * 等价于：
	 *   CGAuthClient::CalcSignatureCStyleWegameGetTicket
	 */
	static bool CalcWegameGetTicketSignatureMd5(
		int time_s,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion SmsLogin（短信验证码登录）
	// ============================================================

	/**
	 * @brief Gunion 短信登录接口：3DES(EDE3/ECB) 加密参数
	 *
	 *
	 * 对应接口：
	 *   /v1/cooperation/smsLogin
	 *
	 * 加密字段顺序：
	 *   sms_new
	 *   sms_type
	 *   phone
	 *   smsCode
	 *   windowsDeviceId
	 *   deviceIdServer
	 *
	 * 处理流程：
	 *   1. 拼接 key=value 形式字符串
	 *   2. 使用 randKey 进行 3DES(EDE3/ECB) 加密
	 *   3. Base64 编码
	 *   4. URL Encode
	 */
	static bool EncryptGunionSmsLoginParams3DesBase64(
		const std::string& sms_new,
		const std::string& sms_type,
		const std::string& phone,
		const std::string& smsCode,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);


	/**
	 * @brief Gunion 短信登录接口：计算请求签名（MD5）
	 *
	 *
	 * 签名字段顺序：
	 *   sms_new
	 *   sms_type
	 *   phone
	 *   smsCode
	 *   windowsDeviceId
	 *   deviceIdServer
	 *   randKey
	 *
	 * 规则：
	 *   1. 直接拼接 value
	 *   2. 不带 & 不带 =
	 *   3. 计算 MD5
	 */
	static bool CalcGunionSmsLoginSignatureMd5(
		const std::string& sms_new,
		const std::string& sms_type,
		const std::string& phone,
		const std::string& smsCode,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion RealName（实名认证）
	// ============================================================

	/**
	 * @brief Gunion 实名认证接口：3DES(EDE3/ECB) 加密参数
	 *
	 *
	 * 对应接口：
	 *   /v1/cooperation/realName
	 *
	 * 加密字段顺序：
	 *   idcard
	 *   name
	 *   windowsDeviceId
	 *   deviceIdServer
	 *
	 * 处理流程：
	 *   1. 拼接 key=value 形式字符串
	 *   2. 使用 randKey 进行 3DES(EDE3/ECB) 加密
	 *   3. Base64 编码
	 *   4. URL Encode
	 */
	static bool EncryptGunionRealNameParams3DesBase64(
		const std::string& idcard,
		const std::string& name,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief Gunion 实名认证接口：计算请求签名（MD5）
	 *
	 *
	 * 签名字段顺序：
	 *   idcard
	 *   name
	 *   windowsDeviceId
	 *   deviceIdServer
	 *   randKey
	 *
	 * 规则：
	 *   1. 直接拼接 value
	 *   2. 不带 & 不带 =
	 *   3. 计算 MD5
	 */
	static bool CalcGunionRealNameSignatureMd5(
		const std::string& idcard,
		const std::string& name,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion AutoLogin（自动登录）
	// ============================================================

	/**
	 * @brief Gunion 自动登录接口：3DES(EDE3/ECB) 加密参数
	 *
	 *
	 * 对应接口：
	 *   /v1/cooperation/autoLogin
	 *
	 * 加密字段顺序：
	 *   autokey
	 *   windowsDeviceId
	 *   deviceIdServer
	 *
	 * 处理流程：
	 *   1. 拼接 key=value 形式字符串
	 *   2. 使用 randKey 进行 3DES(EDE3/ECB) 加密
	 *   3. Base64 编码
	 *   4. URL Encode
	 */
	static bool EncryptGunionAutoLoginParams3DesBase64(
		const std::string& autokey,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);


	/**
	 * @brief Gunion 自动登录接口：计算请求签名（MD5）
	 *
	 *
	 * 签名字段顺序：
	 *   autokey
	 *   windowsDeviceId
	 *   deviceIdServer
	 *   randKey
	 *
	 * 规则：
	 *   1. 直接拼接 value
	 *   2. 不带 & 不带 =
	 *   3. 计算 MD5
	 */
	static bool CalcGunionAutoLoginSignatureMd5(
		const std::string& autokey,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion Logout（登出）
	// ============================================================

	/**
	 * @brief Gunion 登出接口：3DES(EDE3/ECB) 加密参数
	 *
	 *
	 * 对应接口：
	 *   /v1/cooperation/logout
	 *
	 * 加密字段顺序：
	 *   autokey
	 *   windowsDeviceId
	 *   deviceIdServer
	 *
	 * 处理流程：
	 *   1. 拼接 key=value 形式字符串
	 *   2. 使用 randKey 进行 3DES(EDE3/ECB) 加密
	 *   3. Base64 编码
	 *   4. URL Encode
	 */
	static bool EncryptGunionLogoutParams3DesBase64(
		const std::string& autokey,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief Gunion 登出接口：计算请求签名（MD5）
	 *
	 *
	 * 签名字段顺序：
	 *   autokey
	 *   deviceIdServer
	 *   randKey
	 *
	 * 规则：
	 *   1. 直接拼接 value
	 *   2. 不带 & 不带 =
	 *   3. 计算 MD5
	 */
	static bool CalcGunionLogoutSignatureMd5(
		const std::string& autokey,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion PwdLogin（账号密码登录）
	// ============================================================

	/**
	 * @brief Gunion 账号密码登录：3DES(EDE3/ECB) 加密参数
	 *
	 *
	 * 对应接口：
	 *   /v1/cooperation/wegameLogin
	 *
	 * 加密字段顺序：
	 *   phone
	 *   password
	 *   supportPic
	 *   windowsDeviceId
	 *   deviceIdServer
	 *
	 * 处理流程：
	 *   1. 拼接 key=value 形式字符串
	 *   2. 使用 randKey 进行 3DES(EDE3/ECB) 加密
	 *   3. Base64 编码
	 *   4. URL Encode
	 */
	static bool EncryptGunionPwdLoginParams3DesBase64(
		const std::string& phone,
		const std::string& password,
		const std::string& supportPic,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief Gunion 账号密码登录：计算请求签名（MD5）
	 *
	 *
	 * 签名字段顺序：
	 *   phone
	 *   password
	 *   supportPic
	 *   windowsDeviceId
	 *   deviceIdServer
	 *   randKey
	 *
	 * 规则：
	 *   1. 直接拼接 value
	 *   2. 不带 & 不带 =
	 *   3. 计算 MD5
	 */
	static bool CalcGunionPwdLoginSignatureMd5(
		const std::string& phone,
		const std::string& password,
		const std::string& supportPic,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion PicPwdLogin（图形验证码密码登录）
	// ============================================================

	/**
	 * @brief Gunion 图形验证码密码登录：3DES(EDE3/ECB) 加密参数
	 *
	 *
	 * 对应接口：
	 *   v1/account/checkCodeLogin
	 *
	 * 加密字段顺序：
	 *   outInfo
	 *   checkCodeGuid
	 *   password
	 *   windowsDeviceId
	 *   deviceIdServer
	 *
	 * 处理流程：
	 *   1. 拼接 key=value 形式字符串
	 *   2. 使用 randKey 进行 3DES(EDE3/ECB) 加密
	 *   3. Base64 编码
	 *   4. URL Encode
	 */
	static bool EncryptGunionPicPwdLoginParams3DesBase64(
		const std::string& checkCodeGuid,
		const std::string& password,
		const std::string& outInfo,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief Gunion 图形验证码密码登录：计算请求签名（MD5）
	 *
	 *
	 * 签名字段顺序：
	 *   outInfo
	 *   checkCodeGuid
	 *   password
	 *   windowsDeviceId
	 *   deviceIdServer
	 *   randKey
	 *
	 * 规则：
	 *   1. 直接拼接 value
	 *   2. 不带 & 不带 =
	 *   3. 计算 MD5
	 */
	static bool CalcGunionPicPwdLoginSignatureMd5(
		const std::string& checkCodeGuid,
		const std::string& password,
		const std::string& outInfo,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion ThirdLogin第三方登录（QQ/WX/WB）
	// ============================================================
	/**
	 * @brief Gunion 第三方登录：3DES(EDE3/ECB) 加密参数
	 *
	 *
	 * 对应接口：
	 *   /v1/account/thirdAccountTicketLogin
	 *
	 * 加密字段顺序：
	 *   companyId
	 *   thirdTicket
	 *   windowsDeviceId
	 *   deviceIdServer
	 *
	 * 处理流程：
	 *   1. 拼接 key=value 形式字符串
	 *   2. 使用 randKey 进行 3DES(EDE3/ECB) 加密
	 *   3. Base64 编码
	 *   4. URL Encode
	 */
	static bool EncryptGunionThirdLoginParams3DesBase64(
		const std::string& companyId,
		const std::string& thirdTicket,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief WeGame 第三方登录：计算请求签名（MD5）
	 *
	 * 等价于：
	 *   CGAuthClient::CalcSignatureCStyleWegameThirdLogin
	 *
	 * 签名字段顺序：
	 *   companyId
	 *   thirdTicket
	 *   windowsDeviceId
	 *   deviceIdServer
	 *   randKey
	 *
	 * 规则：
	 *   1. 直接拼接 value
	 *   2. 不带 & 不带 =
	 *   3. 计算 MD5
	 */
	static bool CalcGunionThirdLoginSignatureMd5(
		const std::string& companyId,
		const std::string& thirdTicket,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);


	// ============================================================
	// Gunion LoginArea（登记区服列表）
	// ============================================================
	/**
	 * @brief Gunion 登记区服列表接口：3DES(EDE3/ECB) 加密参数
	 *
	 *
	 * 对应接口：
	 *   v1/basic/loginarea
	 *
	 * 加密字段顺序：
	 *   userid
	 *   area
	 *   group
	 *   windowsDeviceId
	 *   deviceIdServer
	 *
	 * 处理流程：
	 *   1. 拼接 key=value 形式字符串
	 *   2. 使用 randKey 进行 3DES(EDE3/ECB) 加密
	 *   3. Base64 编码
	 *   4. URL Encode
	 */
	static bool EncryptGunionLoginAreaParams3DesBase64(
		const std::string& userid,
		const std::string& area,
		const std::string& group,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief Gunion 登记区服列表接口：计算请求签名（MD5）
	 *
	 *
	 * 签名字段顺序：
	 *   userid
	 *   area
	 *   group
	 *   windowsDeviceId
	 *   deviceIdServer
	 *   randKey
	 *
	 * 规则：
	 *   1. 直接拼接 value
	 *   2. 不带 & 不带 =
	 *   3. 计算 MD5
	 */
	static bool CalcGunionLoginAreaSignatureMd5(
		const std::string& userid,
		const std::string& area,
		const std::string& group,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion GetLoginArea（获取区服列表）
	// ============================================================
	/**
	 * @brief Gunion 获取区服列表接口：3DES(EDE3/ECB) 加密参数
	 *
	 *
	 * 对应接口：
	 *   v1/basic/getAreaList
	 *
	 * 加密字段顺序：
	 *   deviceIdServer
	 *
	 * 处理流程：
	 *   1. 拼接 key=value 形式字符串
	 *   2. 使用 randKey 进行 3DES(EDE3/ECB) 加密
	 *   3. Base64 编码
	 *   4. URL Encode
	 */
	static bool EncryptGunionGetLoginAreaParams3DesBase64(
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief Gunion 获取区服列表接口：计算请求签名（MD5）
	 *
	 * 等价于：
	 *   CGAuthClient::CalcSignatureCStyleWegameGetLoginArea
	 *
	 * 签名字段顺序：
	 *   deviceIdServer
	 *   randKey
	 *
	 * 规则：
	 *   1. 直接拼接 value
	 *   2. 不带 & 不带 =
	 *   3. 计算 MD5
	 */
	static bool CalcGunionGetLoginAreaSignatureMd5(
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion SpecialPwdLogin（个性账号密码登录）
	// ============================================================
	/**
	 * @brief Gunion 个性账号密码登录：3DES(EDE3/ECB) 加密参数
	 *
	 * 等价于：
	 *   CGAuthClient::EncrytParams3DesCStyleWegameSpecialPwdLogin
	 *
	 * 对应接口：
	 *   /v1/accountUnification/login
	 *
	 * 加密字段顺序：
	 *   account
	 *   password
	 *   windowsDeviceId
	 *   deviceIdServer
	 *
	 * 处理流程：
	 *   1. 拼接 key=value 形式字符串
	 *   2. 使用 randKey 进行 3DES(EDE3/ECB) 加密
	 *   3. Base64 编码
	 *   4. URL Encode
	 */
	static bool EncryptGunionSpecialPwdLoginParams3DesBase64(
		const std::string& account,
		const std::string& password,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief Gunion 个性账号密码登录：计算请求签名（MD5）
	 *
	 *
	 * 签名字段顺序：
	 *   account
	 *   password
	 *   windowsDeviceId
	 *   deviceIdServer
	 *   randKey
	 *
	 * 规则：
	 *   1. 直接拼接 value
	 *   2. 不带 & 不带 =
	 *   3. 计算 MD5
	 */
	static bool CalcGunionSpecialPwdLoginSignatureMd5(
		const std::string& account,
		const std::string& password,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion SpecialPicPwdLogin（个性账号图形验证码密码登录）
	// ============================================================
	/**
	 * @brief Gunion 个性账号图形验证码密码登录：3DES(EDE3/ECB) 加密参数
	 *
	 *
	 * 对应接口：
	 *   /v1/accountUnification/picCheckLogin
	 *
	 * 加密字段顺序：
	 *   checkCodeGuid
	 *   outInfo
	 *   windowsDeviceId
	 *   deviceIdServer
	 *
	 * 处理流程：
	 *   1. 拼接 key=value 形式字符串
	 *   2. 使用 randKey 进行 3DES(EDE3/ECB) 加密
	 *   3. Base64 编码
	 *   4. URL Encode
	 */
	static bool EncryptGunionSpecialPicPwdLoginParams3DesBase64(
		const std::string& checkCodeGuid,
		const std::string& outInfo,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief Gunion 性账号图形验证码密码登录：计算请求签名（MD5）
	 *
	 *
	 * 签名字段顺序：
	 *   checkCodeGuid
	 *   outInfo
	 *   windowsDeviceId
	 *   deviceIdServer
	 *   randKey
	 *
	 * 规则：
	 *   1. 直接拼接 value
	 *   2. 不带 & 不带 =
	 *   3. 计算 MD5
	 */
	static bool CalcGunionSpecialPicPwdLoginSignatureMd5(
		const std::string& checkCodeGuid,
		const std::string& outInfo,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion SpecialSmsSend（个性账号短信发送）
	// ============================================================
	/**
	 * @brief Gunion 个性账号短信发送接口：3DES(EDE3/ECB) 加密参数
	 *
	 *
	 * 对应接口：
	 *   /v1/cooperation/specialSmsSend
	 *
	 * 加密字段顺序：
	 *   account
	 *   voiceMsg
	 *   smsNew
	 *   windowsDeviceId
	 *   deviceIdServer
	 *
	 * 处理流程：
	 *   1. 拼接 key=value 形式字符串
	 *   2. 使用 randKey 进行 3DES(EDE3/ECB) 加密
	 *   3. Base64 编码
	 *   4. URL Encode
	 */
	static bool EncryptGunionSpecialSmsSendParams3DesBase64(
		const std::string& account,
		const std::string& voiceMsg,
		const std::string& smsNew,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief Gunion 个性账号短信发送接口：计算请求签名（MD5）
	 *
	 *
	 * 签名字段顺序：
	 *   account
	 *   voiceMsg
	 *   smsNew
	 *   windowsDeviceId
	 *   deviceIdServer
	 *   randKey
	 *
	 * 规则：
	 *   1. 直接拼接 value
	 *   2. 不带 & 不带 =
	 *   3. 计算 MD5
	 */
	static bool CalcGunionSpecialSmsSendSignatureMd5(
		const std::string& account,
		const std::string& voiceMsg,
		const std::string& smsNew,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion SpecialSmsSend（个性账号图形验证码短信发送）
	// ============================================================
	/**
	 * @brief Gunion 个性账号图形验证码短信发送接口：3DES(EDE3/ECB) 加密参数
	 *
	 *
	 * 对应接口：
	 *   /v1/cooperation/specialPicSmsSend
	 *
	 * 加密字段顺序：
	 *   captchaInfo
	 *   smsSendGuid
	 *   deviceIdServer
	 *
	 * 处理流程：
	 *   1. 拼接 key=value 形式字符串
	 *   2. 使用 randKey 进行 3DES(EDE3/ECB) 加密
	 *   3. Base64 编码
	 *   4. URL Encode
	 */
	static bool EncryptGunionSpecialPicSmsSendParams3DesBase64(
		const std::string& captchaInfo,
		const std::string& smsSendGuid,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief Gunion 个性账号图形验证码短信发送接口：计算请求签名（MD5）
	 *
	 *
	 * 签名字段顺序：
	 *   captchaInfo
	 *   smsSendGuid
	 *   deviceIdServer
	 *   randKey
	 *
	 * 规则：
	 *   1. 直接拼接 value
	 *   2. 不带 & 不带 =
	 *   3. 计算 MD5
	 */
	static bool CalcGunionSpecialPicSmsSendSignatureMd5(
		const std::string& captchaInfo,
		const std::string& smsSendGuid,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion SpecialConfirmSmsSend（个性账号短信确认发送）
	// ============================================================

	/**
	 * @brief Gunion 个性账号短信确认发送接口：3DES(EDE3/ECB) 加密参数
	 *
	 *
	 * 对应接口：
	 *   /v1/accountUnification/confirmSmsSend
	 *
	 * 加密字段顺序：
	 *   smsSendGuid
	 *   deviceIdServer
	 */
	static bool EncryptGunionSpecialConfirmSmsSendParams3DesBase64(
		const std::string& smsSendGuid,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief Gunion 个性账号短信确认发送接口：计算请求签名（MD5）
	 *
	 * 签名字段顺序：
	 *   smsSendGuid
	 *   deviceIdServer
	 *   randKey
	 */
	static bool CalcGunionSpecialConfirmSmsSendSignatureMd5(
		const std::string& smsSendGuid,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion SpecialCheckSmsLogin（个性账号短信登录校验）
	// ============================================================

	/**
	 * @brief Gunion 个性账号短信登录校验接口：3DES(EDE3/ECB) 加密参数
	 *
	 *
	 * 对应接口：
	 *   /v1/cooperation/specialCheckSmsLogin
	 *
	 * 加密字段顺序：
	 *   smsSendGuid
	 *   smsCode
	 *   account
	 *   windowsDeviceId
	 *   deviceIdServer
	 */
	static bool EncryptGunionSpecialCheckSmsLoginParams3DesBase64(
		const std::string& smsSendGuid,
		const std::string& smsCode,
		const std::string& account,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);


	/**
	 * @brief Gunion 个性账号短信登录校验接口：计算请求签名（MD5）
	 *
	 * 签名字段顺序：
	 *   smsSendGuid
	 *   smsCode
	 *   account
	 *   windowsDeviceId
	 *   deviceIdServer
	 *   randKey
	 */
	static bool CalcGunionSpecialCheckSmsLoginSignatureMd5(
		const std::string& smsSendGuid,
		const std::string& smsCode,
		const std::string& account,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion PayEntrance（支付入口）
	// ============================================================
	/**
	 * @brief Gunion 支付入口接口：3DES(EDE3/ECB) 加密参数
	 *
	 *
	 * 对应接口：
	 *   /v1/pay/payEntrance
	 *
	 * 加密字段顺序：
	 *   orderId
	 *   productId
	 *   groupId
	 *   areaId
	 *   extend
	 *   isSandboxAccount
	 *   deviceIdServer
	 *
	 *
	 * 处理流程：
	 *   1. 拼接 key=value 形式字符串
	 *   2. 使用 randKey 进行 3DES(EDE3/ECB) 加密
	 *   3. Base64 编码
	 *   4. URL Encode
	 */
	static bool EncryptGunionPayEntranceParams3DesBase64(
		const std::string& orderId,
		const std::string& productId,
		const std::string& groupId,
		const std::string& areaId,
		const std::string& extend,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief Gunion 支付入口接口：计算请求签名（MD5）
	 *
	 *
	 * 签名字段顺序：
	 *   orderId
	 *   productId
	 *   groupId
	 *   areaId
	 *   extend
	 *   deviceIdServer
	 *   randKey
	 *
	 * 规则：
	 *   1. 直接拼接 value
	 *   2. 不带 & 不带 =
	 *   3. 计算 MD5
	 */
	static bool CalcGunionPayEntranceSignatureMd5(
		const std::string& orderId,
		const std::string& productId,
		const std::string& groupId,
		const std::string& areaId,
		const std::string& extend,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion CheckPayOrderStatus（查询支付订单状态）
	// ============================================================
	/**
	 * @brief Gunion 查询支付订单状态接口：3DES(EDE3/ECB) 加密参数
	 *
	 *
	 * 对应接口：
	 *   /v1/pay/checkPayOrderStatus
	 *
	 * 加密字段顺序：
	 *   orderId
	 *
	 *
	 * 处理流程：
	 *   1. 拼接 key=value 形式字符串
	 *   2. 使用 randKey 进行 3DES(EDE3/ECB) 加密
	 *   3. Base64 编码
	 *   4. URL Encode
	 */
	static bool EncryptGunionCheckPayOrderStatusParams3DesBase64(
		const std::string& orderId,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief Gunion 查询支付订单状态接口：计算请求签名（MD5）
	 *
	 * 等价于：
	 *   CGAuthClient::CalcSignatureCStyleGunionCheckPayOrderStatus
	 *
	 * 签名字段顺序：
	 *   orderId
	 *   randKey
	 *
	 * 规则：
	 *   1. 直接拼接 value
	 *   2. 不带 & 不带 =
	 *   3. 计算 MD5
	 */
	static bool CalcGunionCheckPayOrderStatusSignatureMd5(
		const std::string& orderId,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion CheckActivation（激活码校验）
	// ============================================================

	/**
	 * @brief Gunion 激活码校验接口：3DES(EDE3/ECB) 加密参数
	 *
	 *
	 * 对应接口：
	 *   /v1/activation/check
	 *
	 * 加密字段顺序：
	 *   activation
	 *   deviceIdServer
	 */
	static bool EncryptGunionCheckActivationParams3DesBase64(
		const std::string& activation,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);


	/**
	 * @brief Gunion 激活码校验接口：计算请求签名（MD5）
	 *
	 * 签名字段顺序：
	 *   activation
	 *   deviceIdServer
	 *   randKey
	 */
	static bool CalcGunionCheckActivationSignatureMd5(
		const std::string& activation,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion SetTutorInfo（监护人信息设置）
	// ============================================================

	/**
	 * @brief Gunion 监护人信息设置接口：3DES(EDE3/ECB) 加密参数
	 *
	 * 等价于：
	 *   CGAuthClient::EncrytParams3DesCStyleGunionSetTutorInfo
	 *
	 * 对应接口：
	 *   /v1/tutor/setInfo
	 *
	 * 加密字段顺序：
	 *   idcard
	 *   name
	 *   phone
	 *   smscode
	 *   confirmKey
	 *   deviceIdServer
	 */
	static bool EncryptGunionSetTutorInfoParams3DesBase64(
		const std::string& idcard,
		const std::string& name,
		const std::string& phone,
		const std::string& smscode,
		const std::string& confirmKey,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief Gunion 监护人信息设置接口：计算请求签名（MD5）
	 *
	 * 签名字段顺序：
	 *   idcard
	 *   name
	 *   phone
	 *   smscode
	 *   confirmKey
	 *   deviceIdServer
	 *   randKey
	 */
	static bool CalcGunionSetTutorInfoSignatureMd5(
		const std::string& idcard,
		const std::string& name,
		const std::string& phone,
		const std::string& smscode,
		const std::string& confirmKey,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion ChannelLogin（渠道登录）
	// ============================================================
	/**
	 * @brief Gunion 渠道登录接口：3DES(EDE3/ECB) 加密参数
	 *
	 *
	 * 对应接口：
	 *   v1/account/twiceSelectLogin
	 *
	 * 加密字段顺序：
	 *   selectLoginContext
	 *   companyId
	 *   deviceIdServer
	 *
	 * 处理流程：
	 *   1. 拼接 key=value 形式字符串
	 *   2. 使用 randKey 进行 3DES(EDE3/ECB) 加密
	 *   3. Base64 编码
	 *   4. URL Encode
	 */
	static bool EncryptGunionWegameChannelLoginParams3DesBase64(
		const std::string& selectLoginContext,
		const std::string& companyId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief WeGame 渠道登录接口：计算请求签名（MD5）
	 *
	 *
	 * 签名字段顺序：
	 *   selectLoginContext
	 *   companyId
	 *   deviceIdServer
	 *   randKey
	 *
	 * 规则：
	 *   1. 直接拼接 value
	 *   2. 不带 & 不带 =
	 *   3. 计算 MD5
	 */
	static bool CalcGunionWegameChannelLoginSignatureMd5(
		const std::string& selectLoginContext,
		const std::string& companyId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);
};
