#pragma once

#include "SdoBaseClient.h"
#include "HttpThread.h"
#include "MsgProcess.h"
#include <sstream>
/**
 * ===============================================================
 * 【LoginClient —— 登录器业务核心类】
 * ===============================================================
 *
 * LoginClient 是整个 Sdo 登录体系的核心调度中心：
 *
 * 功能职责：
 * ---------------------------------------------------------------
 *  1. 负责管理所有登录行为：静密、动态码、扫码、实名、下行短信等
 *  2. 构造 HttpRequest 并通过 HttpThread/CurlHttpThread 发送到服务器
 *  3. 执行服务器协议：/login /dynamic /geetest /sms /push 等等
 *  4. 解析服务器返回 JSON，并转发到对应业务回调
 *  5. 管理产品信息、AppID/AreaID/GroupID/Locale 等业务变量
 *  6. 保持网络状态（host1、host2、host3、host4 的 Failover 续连）
 *  7. 管理动态 Key、公钥、SessionKey 会话变量
 *  8. 管理所有回调对象：登录、短信、账号、订单、广告等全部回调
 *
 * LoginClient = 登录器的“大脑”
 * ---------------------------------------------------------------
 * 是整个业务流程的主控制器，所有请求 → Response → 回调 全靠它。
 *
 * 生命周期：
 * ---------------------------------------------------------------
 *   LoginClient client;
 *   client.Initialize(...);    // 必须先初始化
 *   client.SetXXXCallback();   // 设置回调
 *   client.StaticLogin(...);   // 开始登录流程
 *
 *
 * ===============================================================
 * ↓↓↓ 下面开始超详细接口注释（第 1 段：Initialize 系列）↓↓↓
 * ===============================================================
 */

 // 前向声明，避免在头文件中引入 GunionHandshake.h
class GunionHandshake;

class LoginClient
{
	friend class HttpThread;        // 旧版 WinInet 线程可访问私有成员
	friend class CurlHttpThread;    // 新版 curl 线程可访问私有成员

public:
	LoginClient(bool verifyCert = true);
	~LoginClient();

	/**
	 * @brief 登录器初始化（完整版本，必须先调用）
	 *
	 * 【功能说明】
	 * -------------------------------------------------------------
	 * 这是整个登录器 SDK 的入口初始化接口。
	 * 必须在任何登录操作之前调用本函数。
	 *
	 * 主要工作：
	 *   - 解析 serverAddr / backupServerAddr
	 *   - 初始化产品配置（AppID/AreaID/GroupID/Locale/Tag）
	 *   - 配置 ProductID / Version（服务端验证用）
	 *   - 初始化安全级别 customSecurityLevel
	 *   - 创建 HttpThread / CurlHttpThread 后台线程
	 *   - 设置所有登录相关回调
	 *
	 *
	 * 【参数详解】
	 * -------------------------------------------------------------
	 * const char* serverAddr
	 *     主服务器地址，例如：
	 *         "http://login.sdo.com:80"
	 *     会被 ParseHttpAddr 自动解析为 hostName + port
	 *
	 * const char* backupServerAddr
	 *     备用服务器地址，与 serverAddr 格式一致。
	 *     当主服务器失败时自动切换。
	 *
	 * int appId
	 *     产品 AppID，例如 FF14、传奇4、QQGame 等。
	 *
	 * int areaId
	 *     所属大区 ID（国服/台服/韩服等）。
	 *
	 * int groupId
	 *     渠道组（不同渠道包可能不同）。
	 *
	 * int locale
	 *     登录器语言环境（0=简体，1=繁中，2=韩文...）
	 *
	 * int tag
	 *     登录器启动标记（渠道/灰度/SBox 模式等）。
	 *
	 * int productId
	 *     服务器用于区分不同游戏产品的 ID。
	 *
	 * const char* productVersion
	 *     当前登录器版本号，例如 "1.9.1"。
	 *     服务端用于风控、数据统计等逻辑。
	 *
	 * int customSecurityLevel
	 *     安全级别：
	 *        0 = 默认
	 *        1 = 高安全
	 *        2 = 强校验（严格检查密码/验证码/设备）
	 *
	 * SdoBase_CheckCodeLoginCallback checkCodeLoginCallback
	 *     图形验证码登录回调。
	 *
	 * SdoBase_DynamicLoginCallback dynamicLoginCallback
	 *     动态密码登录回调。
	 *
	 * SdoBase_FcmLoginCallback fcmLoginCallback
	 *     实名认证登录回调。
	 *
	 * SdoBase_LoginResultCallback loginResultCallback
	 *     标准登录结果（成功/失败）回调。
	 *
	 * SdoBase_GetDynamicKeyCallback getDynamicKeyCallback
	 *     获取动态 Key 的回调（动态登录依赖公钥）。
	 *
	 * SdoBase_SendPhoneCheckCodeCallback phoneCheckCodeCallback
	 *     发送手机验证码的回调。
	 *
	 * SdoBase_CheckAccounTypeCallback checkAccountCallback
	 *     判断账号类型（邮箱/手机/通行证）的回调。
	 *
	 * SdoBase_GetQrCodeCallback getCodeKeyCallback
	 *     二维码登录请求二维码内容回调。
	 *
	 * SdoBase_GetLoginStateCallback loginStatusCallback
	 *     查询当前登录状态回调（扫码/Push 登录用）。
	 *
	 * SdoBase_ExtendLoginStateCallback extendLoginStateCallback
	 *     扩展登录状态（特殊登录场景）回调。
	 *
	 * SdoBase_LogoutCallback logoutCallback
	 *     登出回调。
	 *
	 * SdoBase_SendPushMessageCallback sendPushMessageCallback
	 *     推送登录（手机通知验证登录）回调。
	 *
	 *
	 * 【返回值】
	 * -------------------------------------------------------------
	 *  0 = 初始化成功
	 * -1 = 参数错误
	 * -2 = 后台线程创建失败
	 * -3 = 服务器地址解析失败
	 *
	 * 【调用顺序要求】
	 * -------------------------------------------------------------
	 * 必须最先调用：
	 *
	 *      LoginClient lc;
	 *      lc.Initialize(...);
	 *
	 * 之后才能调用所有登录类 API，否则全部失败：
	 *
	 *      lc.StaticLogin(...);
	 *      lc.DynamicLogin(...);
	 *      lc.SsoLogin(...);
	 *      lc.PushMessageLogin(...);
	 *
	 */
	int Initialize(const char* serverAddr, const char* backupServerAddr,
		int appId, int areaId, int groupId, int locale, int tag,
		int productId, const char* productVersion, int customSecurityLevel,
		SdoBase_CheckCodeLoginCallback checkCodeLoginCallback,
		SdoBase_DynamicLoginCallback dynamicLoginCallback,
		SdoBase_FcmLoginCallback fcmLoginCallback,
		SdoBase_LoginResultCallback loginResultCallback,
		SdoBase_GetDynamicKeyCallback getDynamicKeyCallback,
		SdoBase_SendPhoneCheckCodeCallback phoneCheckCodeCallback,
		SdoBase_CheckAccounTypeCallback checkAccountCallback,
		SdoBase_GetQrCodeCallback getCodeKeyCallback,
		SdoBase_GetLoginStateCallback loginStatusCallback,
		SdoBase_ExtendLoginStateCallback extendLoginStateCallback,
		SdoBase_LogoutCallback logoutCallback,
		SdoBase_SendPushMessageCallback sendPushMessageCallback,
		bool enableGunionPreInit, const char* gunionServerAddr, const char* gunionBackupServerAddr
	);

	/**
	 * @brief 初始化简化版本（不绑定回调）
	 *
	 * 【功能说明】
	 * -------------------------------------------------------------
	 * 与 Initialize() 完整版几乎一致，但不包括回调绑定。
	 * 适用于：
	 *   - 某些场景需要先初始化网络环境，再分步骤设置回调。
	 *   - 用于第三方集成、SDK 外层回调二次封装等场景。
	 *
	 * 用法示例：
	 * -------------------------------------------------------------
	 *     LoginClient lc;
	 *     lc.Initialize2(...);
	 *
	 *     // 再手动设置各回调
	 *     lc.SetLoginResultCallback(...);
	 *     lc.SetDynamicLoginCallback(...);
	 *     ...
	 *
	 * 返回值意义与 Initialize() 相同。
	 */
	int Initialize2(const char* serverAddr, const char* backupServerAddr,
		int appId, int areaId, int groupId, int locale, int tag,
		int productId, const char* productVersion, int customSecurityLevel);

	// ============================================================
	//         （回调区域）所有业务回调函数的设置接口
	// ============================================================
	// 说明：
	//   LoginClient 不直接处理登录UI，而是通过回调把结果推给上层。
	//   所有回调均由外部业务（Unity、UE、Duilib、C# 前端）设置。
	//
	//   CurlHttpThread/HttpThread 完成网络通信 →
	//   LoginClient::ProcessResponse 解析 JSON →
	//   调用对应回调，将最终业务结果传给上层。
	//
	//   这些接口一个都不能省，否则会影响不同游戏/渠道的登录流程。
	// ============================================================


	/** 图形验证码登录（CheckCodeLogin）结果回调 */
	void SetCheckCodeLoginCallback(SdoBase_CheckCodeLoginCallback callback)
	{
		m_checkCodeLoginCallback = callback;
	}

	/** 动态口令牌（DynamicLogin）结果回调 */
	void SetDynamicLoginCallback(SdoBase_DynamicLoginCallback callback)
	{
		m_dynamicLoginCallback = callback;
	}

	/** 实名登录（FcmLogin）回调 */
	void SetFcmLoginCallback(SdoBase_FcmLoginCallback callback)
	{
		m_fcmLoginCallback = callback;
	}

	/** 最终登录结果回调（所有登录方式都会走这里） */
	void SetLoginResultCallback(SdoBase_LoginResultCallback callback)
	{
		m_loginResultCallback = callback;
	}

	/** 获取动态 Key（GetDynamicKey）回调 */
	void SetGetDynamicKeyCallback(SdoBase_GetDynamicKeyCallback callback)
	{
		m_getDynamicKeyCallback = callback;
	}

	/** 获取短信验证码（SendPhoneCheckCode）回调 */
	void SetSendPhoneCheckCodeCallback(SdoBase_SendPhoneCheckCodeCallback callback)
	{
		m_phoneCheckCodeCallback = callback;
	}

	/** 检查账号类型（手机号/邮箱/普通账号）回调 */
	void SetCheckAccountTypeCallback(SdoBase_CheckAccounTypeCallback callback)
	{
		m_checkAccountCallback = callback;
	}

	/** 获取二维码（扫码登录）回调 */
	void SetGetQrCodeCallback(SdoBase_GetQrCodeCallback callback)
	{
		m_getCodeKeyCallback = callback;
	}

	/** 获取登录态（GetLoginState）回调 */
	void SetGetLoginStateCallback(SdoBase_GetLoginStateCallback callback)
	{
		m_loginStatusCallback = callback;
	}

	/** 延长登录态（ExtendLoginState）回调 */
	void SetExtendLoginStateCallback(SdoBase_ExtendLoginStateCallback callback)
	{
		m_extendLoginStateCallback = callback;
	}

	/** 登出（Logout）回调 */
	void SetLogoutCallback(SdoBase_LogoutCallback callback)
	{
		m_logoutCallback = callback;
	}

	/** 发送一键登录短信（SendPushMessage）回调 */
	void SetSendPushMessageCallback(SdoBase_SendPushMessageCallback callback)
	{
		m_sendPushMessageCallback = callback;
	}

	/** 获取推送登录状态（GetPushMessageStatus）回调 */
	void SetGetPushMessageStatusCallback(SdoBase_GetPushMessageStatusCallback callback)
	{
		m_getPushMessageStatusCallback = callback;
	}

	/** 取消一键登录（CancelPushMessageLogin）回调 */
	void SetCancelPushMessageCallback(SdoBase_CancelPushMessageCallback callback)
	{
		m_cancelPushMessageCallback = callback;
	}

	/** 获取 Session 状态（下行短信业务使用） */
	void SetGetSessionIdStatesCallBack(SdoBase_GetSessionIdStatesCallBack callback)
	{
		m_getSessionIdStatesCallBack = callback;
	}

	/** 踢下线验证第一步回调（KickoffAccountVerify） */
	void SetKickoffAccountVerifyCallback(SdoBase_KickoffAccountVerifyCallback callback)
	{
		m_callbackKickoffAccountVerify = callback;
	}

	/** 踢下线操作最终结果回调 */
	void SetKickoffAccountResultCallback(SdoBase_KickoffAccountResultCallback callback)
	{
		m_callbackKickoffAccountResult = callback;
	}

	/** 获取 Ticket（TGT）回调，用于 WeGame/庆余年等需要二次票据的场景 */
	void SetGetTicketCallback(SdoBase_GetTicketCallback callback)
	{
		m_GetTicketCallBack = callback;
	}

	/** 创建 WeGame 订单回调（支付系统） */
	void SetCreateWeGameOrderCallback(SdoBase_CreateWeGameOrderCallback callback)
	{
		m_CreateWeGameOrderCallback = callback;
	}

	/** 设置授权码登录获取 Ticket 的回调 */
	void SetCreateAuthCodeCallback(SdoBase_CreateAuthCodeTicketCallback callback)
	{
		m_CreateAuchCodeCallback = callback;
	}

	/** 设置三方登录灵活皮肤回调（动态展示 UI） */
	void SetThirdLoginSkinCallback(SdoBase_CreateThirdLoginSkinCallback callback)
	{
		m_CreateThirdLoginSkinCallback = callback;
	}

	// ----------------------------- 传奇4相关 -----------------------------

	/** 传奇4创建订单回调 */
	void SetCreateCQ4OrderCallback(SdoBase_CreateCQ4OrderCallback callback)
	{
		m_CreateCQ4OrderCallback = callback;
	}

	/** 传奇4查询订单信息回调 */
	void SetCreateCQ4QueryCallback(SdoBase_CreateCQ4QueryCallback callback)
	{
		m_CreateCQ4QueryCallback = callback;
	}

	// ----------------------------- Steam 渠道 -----------------------------

	/** Steam 手游渠道订单创建 */
	void SetCreateSteamChannelOrderCallback(SdoBase_CreateSteamChannelOrderCallback callback)
	{
		m_CreateSteamChannelOrderCallback = callback;
	}

	// ----------------------------- QQGame 渠道 -----------------------------

	/** QQGame 创建订单回调 */
	void SetCreateQQGameOrderCallback(SdoBase_CreateQQGameOrderCallback callback)
	{
		m_CreateQQGameOrderCallback = callback;
	}

	/** QQGame 是否已登录（状态识别） */
	void SetQQGameIsLoginCallback(SdoBase_QQGameIsLoginCallback callback)
	{
		m_QQGameIsLoginCallback = callback;
	}

	// ----------------------------- WeGame 状态 -----------------------------

	/** WeGame 状态检测回调（如是否受限） */
	void SetWeGameStatusCallback(SdoBase_WeGameStatusCallback callback)
	{
		m_WeGameStatusCallback = callback;
	}

	// ----------------------------- 联想/乐信渠道 -----------------------------

	/** Lenovo/乐信订单创建回调 */
	void SetCreateLxOrderCallback(SdoBase_CreateLxOrderCallback callback)
	{
		m_CreateLxOrderCallback = callback;
	}

	// ======================================================================
	//                      回调函数设置接口
	// ======================================================================
	// 所有 “Set 回调” 函数都是用来注册业务层的事件回调，
	// 通常由业务在初始化阶段调用。每个回调都会在对应的
	// 服务端响应（ProcessResponse → ProcessXXX）之后触发。
	// ======================================================================

	/**
	 * 设置获取账号信息回调
	 * 回调触发时机：GetAccountInfo() 请求返回后
	 */
	void SetGetAccountInfoCallback(SdoBase_GetAccountInfoCallback callback)
	{
		m_getAccountInfoCallback = callback;
	}

	/**
	 * 设置获取登录历史记录回调
	 * 回调触发时机：GetLoginHistory() 请求返回后
	 */
	void SetGetLoginHistoryCallback(SdoBase_GetLoginHistoryCallback callback)
	{
		m_getLoginHistoryCallback = callback;
	}

	/**
	 * 设置获取登录用户信息回调（昵称、头像等）
	 */
	void SetGetLoginUserInfoCallback(SdoBase_GetLoginUserInfoCallback callback)
	{
		m_getLoginUserInfoCallback = callback;
	}

	/**
	 * 设置修改用户登录信息回调（修改备注名等）
	 */
	void SetSetLoginUserInfoCallback(SdoBase_SetLoginUserInfoCallback callback)
	{
		m_setLoginUserInfoCallback = callback;
	}

	/**
	 * 设置获取区服列表（AreaInfo）回调
	 */
	void SetGetLoginAreaInfoCallback(SdoBase_GetLoginAreaInfoCallback callback)
	{
		m_getLoginAreaInfoCallback = callback;
	}

	/**
	 * 设置新版用户隐私协议回调
	 * 对应 UserPrivacyConfig() 返回结果
	 */
	void SetUserPrivacyConfigCallback(SdoBase_UserPrivacyConfigCallback callback)
	{
		m_UserPrivacyConfigCallback = callback;
	}

	/**
	 * 设置新版本（精简）隐私协议回调
	 * 对应 NewVersionUserPrivacyConfig() 返回数据
	 */
	void SetNewVersionUserPrivacyConfigCallback(
		SdoBase_NewVersionUserPrivacyConfigCallback callback)
	{
		m_NewVersionUserPrivacyConfigCallback = callback;
	}

	/**
	 * 设置人脸识别初始化回调（启动人脸识别流程）
	 */
	void SetFaceVerifyInitCallback(SdoBase_FaceVerifyInitCallback callback)
	{
		m_FaceVerifyInitCallback = callback;
	}

	/**
	 * 设置获取人脸识别动作码结果回调
	 */
	void SetGetFaceCodeResultCallback(SdoBase_GetFaceCodeResultCallback callback)
	{
		m_GetFaceCodeResultCallback = callback;
	}

	/**
	 * 设置人脸识别动作上报回调（SendAction）
	 */
	void SetSendActionCallback(SdoBase_SendActionCallback callback)
	{
		m_SendActionCallback = callback;
	}

	/**
	 * 设置卫士通 UE SDK 初始化回调
	 */
	void SetUeInitClientCallback(SdoBase_UeInitClientCallback callback)
	{
		m_UeInitClientCallback = callback;
	}

	///////////////////////////////////////////////////////////////////////////////
	// 获取登录器功能相关配置（ClientConfig）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 设置 获取登录器功能配置（GetClientConfig）的回调函数
	 */
	void SetGetClientConfigCallback(SdoBase_GetClientConfigCallback callback)
	{
		m_GetClientConfigCallback = callback;
	}

	///////////////////////////////////////////////////////////////////////////////
	// 下行短信登录（GoDown 系列）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 设置 GoDownConfigInit（下行短信初始化）的回调
	 */
	void SetGoDownConfigCallback(SdoBase_GoDownConfigInitCallback callback)
	{
		m_GoDownConfigInitCallback = callback;
	}

	/**
	 * @brief 设置 GoDownSendSmsCheckCode（带图验短信发送）的回调
	 */
	void SetGoDownSendSmsCheckCodeCallBack(
		SdoBase_GoDownConfigSendSmsCheckCodeCallback callback)
	{
		m_GoDownSendSmsCheckCodeCallback = callback;
	}

	/**
	 * @brief 设置 GoDownconfirmSendSms（点击发送验证码）的回调
	 */
	void SetGoDownconfirmSendSmsCallBack(
		SdoBase_GoDownconfirmSendSmsCallback callback)
	{
		m_GoDownconfirmSendSmsCallback = callback;
	}

	/**
	 * @brief 设置 GoDownconfirmLogin（最终登录）的回调
	 */
	void SetGoDownconfirmLoginCallBack(
		SdoBase_GoDownconfirmLoginCallback callback)
	{
		m_GoDownconfirmLoginCallback = callback;
	}

	///////////////////////////////////////////////////////////////////////////////
	// 快速登录（FastLogin 系列）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 设置 FastLogin（快速登录）的回调
	 */
	void SetFastLoginLoginCallBack(
		SdoBase_FastLoginLoginCallback callback)
	{
		m_FastLoginLoginCallback = callback;
	}

	///////////////////////////////////////////////////////////////////////////////
	// 新版 GetTicket（New Version）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 设置新版 GetTicket（GetTicketNew）的回调
	 */
	void SetGetTicketLoginCallBack(
		SdoBase_NewVersionGetTicketCallback callback)
	{
		m_GetTicketLoginCallback = callback;
	}

	///////////////////////////////////////////////////////////////////////////////
	// 其他场景：未绑定安全手机提示
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 设置“未绑定安全手机号提示”的回调
	 */
	void SetOtherLoginSecurityPhoneCallBack(
		SdoBase_OtherLoginSecurityPhoneCallback callback)
	{
		m_BindSecurityPhoneLoginCallback = callback;
	}

	///////////////////////////////////////////////////////////////////////////////
	// 快速登录 - 主动删除账号
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 设置快速登录主动删除账号回调
	 */
	void SetFastLoginActiveDeleteAccountCallBack(
		SdoBase_FastLoginActiveDeleteAccountLoginCallback callback)
	{
		m_FastLoginActiveDeleteAccountLoginCallback = callback;
	}

	///////////////////////////////////////////////////////////////////////////////
	// 检查账号登录类型（短信/实名/人脸等策略）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 设置“检查账号登录方式（CheckAccountLoginType）”回调
	 */
	void SetCheckAccountTypeLoginCallback(
		SdoBase_CheckAccountTypeLoginCallback callback)
	{
		m_CheckAccountTypeLoginCallback = callback;
	}

	///////////////////////////////////////////////////////////////////////////////
	// 星座运势计算（娱乐功能）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 设置星座运势计算回调
	 */
	void SetCalculateCallback(SdoBase_CalculateCallback callback)
	{
		m_CalculateCallback = callback;
	}

	///////////////////////////////////////////////////////////////////////////////
	// 广告系统初始化（InitAdv）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 设置广告系统初始化回调
	 */
	void SetInitAdvCallback(SdoBase_InitAdvCallback callback)
	{
		m_InitAdvCallback = callback;
	}

	///////////////////////////////////////////////////////////////////////////////
	// 监护人信息补填短信（GuardDian 系列）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 设置监护人短信发送回调（无图验）
	 */
	void SetGuardDianSendSmsCallBack(SdoBase_GuardDianSendSmsCallback callback)
	{
		m_GuardDianSendSmsCallback = callback;
	}

	/**
	 * @brief 设置监护人带图验短信发送回调
	 */
	void SetGuardDianSendSmsCheckCodeCallback(
		SdoBase_GuardDianSendSmsCheckCodeCallback callback)
	{
		m_GuardDianSendSmsCheckCodeCallback = callback;
	}

	/**
	 * @brief 设置监护人提交短信验证结果回调
	 */
	void SetGuardDianConfrimSendSmsResultCallBack(
		SdoBase_GuardDianConfrimSendSmsResultCallback callback)
	{
		m_GuardDianConfrimSendSmsResultCallback = callback;
	}

	/**
	 * @brief 设置监护人整体业务结果回调（成功/失败/提示文案）
	 */
	void SetGuardDianResultCallBack(SdoBaseGuardDianCallback callback)
	{
		m_GuardDianCallback = callback;
	}

	/**
	 * @brief 设置人身核验获取Url回调
	 */
	void SetFaceRecognitionUrlCallBack(SdoBaseSetFaceRecognitionUrlCallback callback)
	{
		m_FaceRecognitionUrlCallback = callback;
	}

	/**
	 * @brief 设置人身核验票据验证状态回调
	 */
	void SetQueryFaceVerifyTicketStatusCallBack(SdoBaseSetQueryFaceVerifyTicketStatusCallback callback)
	{
		m_QueryFaceVerifyTicketStatusCallback = callback;
	}

	/**
	 * @brief 设置人身核验信息收集获取Url回调
	 */
	void SetFaceHolderRegistrationUrlCallBack(SdoBaseSetFaceHolderRegistrationUrlCallback callback)
	{
		m_HolderRegistrationUrlCallback = callback;
	}

	/**
	 * @brief 设置激活码回调结果回调
	 */
	void SetActiveLoginCallBack(SdoBaseActiveCodeLoginCallback callback)
	{
		m_ActiveLoginCallback = callback;
	}

	/**
	 * @brief 设置需要展示激活界面的回调
	 */
	void SetShowActiveLoginDlgCallBack(SdoBaseShowActiveCodeLoginDlgCallback callback)
	{
		m_ShowActiveLoginDlgCallback = callback;
	}

	/**
	 * @brief 设置 GunionWegame 初始化回调
	 *
	 * 回调参数包含：
	 *   - 各平台 AppId / Key（微信 / QQ / 微博）
	 *   - 品牌信息（游戏名 / Logo / 品牌名）
	 *   - 语音提示文案
	 *   - 登录按钮样式
	 *   - token / randKey
	 *
	 * @param callback 初始化完成后的回调函数
	 */
	void SetCreateGunionWegameInitCallback(
		SdoBase_CreateGunionWegameInitCallback callback)
	{
		m_CreateGunionWegameInitCallback = callback;
	}

	/**
	 * @brief 设置 GunionWegame 登录回调
	 *
	 * 回调返回：
	 *   - 登录结果
	 *   - 用户 ID
	 *   - 登录票据
	 *   - 是否新用户
	 *   - 是否需要绑定手机及相关策略
	 *
	 * @param callback 登录结果回调
	 */
	void SetCreateGunionWegameLoginCallback(
		SdoBase_CreateGunionWegameLoginCallback callback)
	{
		m_CreateGunionWegameLoginCallback = callback;
	}

	/**
	 * @brief 设置 GunionWegame 短信发送回调
	 *
	 * 用于普通短信 / 语音短信发送结果回调
	 *
	 * @param callback 短信发送结果回调
	 */
	void SetCreateGunionWegameSmsSendCallback(
		SdoBase_CreateGunionWegameSmsSendCallback callback)
	{
		m_CreateGunionWegameSmsSendCallback = callback;
	}

	/**
	 * @brief 设置 GunionWegame 带图验短信发送回调
	 *
	 * 回调中包含：
	 *   - 图形验证码 URL
	 *   - 验证码尺寸
	 *   - 是否为极验
	 *
	 * @param callback 带图验短信发送回调
	 */
	void SetCreateGunionWegamePicSmsSendCallback(
		SdoBase_CreateGunionWegamePicSmsSendCallback callback)
	{
		m_CreateGunionWegamePicSmsSendCallback = callback;
	}

	/**
	 * @brief 设置 GunionWegame 三方账号绑定手机回调
	 *
	 * 回调返回：
	 *   - 绑定手机号
	 *   - 盛趣账号 ID
	 *   - 三方昵称
	 *   - 实名流程 ID
	 *
	 * @param callback 三方账号绑定结果回调
	 */
	void SetCreateGunionWegameThirdAccountBindPhoneCallback(
		SdoBase_CreateGunionWegameThirdAccountBindPhoneCallback callback)
	{
		m_CreateGunionWegameThirdAccountBindPhoneCallback = callback;
	}

	/**
	 * @brief 设置 GunionWegame 实名回调
	 *
	 * 用于三方登录后补填实名信息的结果通知
	 *
	 * @param callback 实名结果回调
	 */
	void SdoBase_SetCreatGunionWegameFillRealinfoCallback(
		SdoBase_CreateGunionWegameFillRealinfoCallback callback)
	{
		m_CreateGunionWegameFillRealinfoCallback = callback;
	}

	/**
	 * @brief 设置 GunionWegame 获取登录票据回调
	 *
	 * @param callback 获取票据结果回调
	 */
	void SdoBase_SetCreatCheckGetTicketCallback(
		SdoBase_CreateGunionWegameGetTicketCallback callback)
	{
		m_CreateGunionWegameGetTicketCallback = callback;
	}

	/**
	 * @brief 设置 GunionWegame 获取支付票据回调
	 *
	 * @param callback 获取支付票据结果回调
	 */
	void SdoBase_SetCreatCheckGetPayTicketCallback(
		SdoBase_CreateGunionWegameGetPayTicketCallback callback)
	{
		m_CreateGunionWegameGetPayTicketCallback = callback;
	}

	/*
	========================================================
		GHOME_PC 回调注册接口（LoginClient 内部）
		说明：
		1. 本区域仅负责保存回调函数指针
		2. 不执行任何业务逻辑
		3. 实际触发发生在网络请求完成之后
	=========================================================
	*/
	/**
	 * @brief 隐私协议跳转回调
	 *
	 * 业务说明：
	 *   - 获取隐私协议跳转URL
	 *   - 客户端根据URL打开Web页面
	 */
	void SetCreateDoPrivateCallback(SdoBase_CreateDoPrivateCallback callback)
	{
		m_CreateDoPirvateCallback = callback;
	}

	/**
	 * @brief 短信发送回调
	 *
	 * 业务说明：
	 *   - 短信发送
	 */
	void SetCreateDoSmsSendCallback(SdoBase_CreateDoSmsSendCallback callback)
	{
		m_CreateDoSmsSendCallback = callback;
	}

	/**
	 * @brief 短信带图验发送回调
	 *
	 * 业务说明：
	 *   - 短信验证码
	 */
	void SetCreateCheckPicSmsSendCallback(SdoBase_CreateCheckPicSmsSendCallback callback)
	{
		m_CreateCheckPicSmsSendCallback = callback;
	}

	/**
	 * @brief 短信登录回调
	 *
	 * 业务说明：
	 *   - 短信验证码登录
	 *   - 返回userid / ticket / autokey 等
	 */
	void SetCreateCheckSmsLoginCallback(SdoBase_CreateCheckSmsLoginCallback callback)
	{
		m_CreateCheckSmsLoginCallback = callback;
	}

	/**
	 * @brief 实名认证回调
	 *
	 * 业务说明：
	 *   - 返回年龄、是否成年
	 *   - 是否需要监护人确认
	 */
	void SdoBase_SetCreatCheckRealNameCallback(SdoBase_CreateCheckRealNameCallback callback)
	{
		m_CreateCheckRealNameCallback = callback;
	}

	/**
	 * @brief 自动登录回调
	 */
	void SdoBase_SetCreatCheckAutoLoginCallback(SdoBase_CreateCheckAutoLoginCallback callback)
	{
		m_CreateCheckAutoLoginCallback = callback;
	}

	/**
	 * @brief 登出回调
	 */
	void SdoBase_SetCreatCheckLogOutCallback(SdoBase_CreateCheckLogoutCallback callback)
	{
		m_CreateCheckLogoutCallback = callback;
	}

	/**
	 * @brief 密码登录回调
	 */
	void SdoBase_SetCreatCheckPwdLoginCallback(SdoBase_CreateDoPwdLoginCallback callback)
	{
		m_CreateCheckPwdLoginCallback = callback;
	}

	/**
	 * @brief 图验密码登录回调
	 */
	void SdoBase_SetCreatCheckPicPwdLoginCallback(SdoBase_CreateCheckPicPwdLoginCallback callback)
	{
		m_CreateCheckPicPwdLoginCallback = callback;
	}

	/**
	 * @brief 第三方登录回调
	 *
	 * 业务说明：
	 *   - QQ / 微信 / 微博 / Wegame 登录
	 */
	void SdoBase_SetCreatCheckThirdLoginCallback(SdoBase_CreateCheckThirdLoginCallback callback)
	{
		m_CreateCheckThirdLoginCallback = callback;
	}

	/**
	 * @brief 登录区服记录回调
	 */
	void SdoBase_SetCreatCheckLoginAreaLoginCallback(SdoBase_CreateCheckLoginAreaCallback callback)
	{
		m_CreateCheckSetLoginAreaLoginCallback = callback;
	}

	/**
	 * @brief 获取区服配置回调
	 */
	void SdoBase_SetCreatCheckGetLoginAreaLoginCallback(SdoBase_CreateCheckGetLoginAreaCallback callback)
	{
		m_CreateCheckGetLoginAreaLoginCallback = callback;
	}

	/**
	 * @brief 个性账号静密登录回调
	 */
	void SdoBase_SetCreatCheckSpecialPwdLoginCallback(SdoBase_CreateDoSpecialPwdLoginCallback callback)
	{
		m_CreateCheckSpecialPwdLoginCallback = callback;
	}

	/**
	 * @brief 个性账号图验静密登录回调
	 */
	void SdoBase_SetCreatCheckSpecialPicPwdLoginCallback(SdoBase_CreateCheckSpecialPicPwdLoginCallback callback)
	{
		m_CreateCheckSpecialPicPwdLoginCallback = callback;
	}

	//======================================================
	// 个性账号短信体系
	//======================================================

	/**
	 * @brief 个性账号短信发送回调
	 *
	 * 业务说明：
	 *   - 个性账号发送短信验证码
	 *   - 可能返回图验信息（普通图验或极验）
	 *   - 返回 smsSendGuid / lastSafePhone 等安全信息
	 */
	void SetCreateDoSpecialSmsSendCallback(
		SdoBase_CreateDoSpecialSmsSendCallback callback)
	{
		m_CreateDoSpecialSmsSendCallback = callback;
	}

	/**
	 * @brief 个性账号图验短信发送回调
	 *
	 * 业务说明：
	 *   - 个性账号在图验校验通过后发送短信
	 *   - 返回图验地址、宽高、类型等信息
	 */
	void SetCreateCheckSpecialPicSmsSendCallback(
		SdoBase_CreateCheckSpecialPicSmsSendCallback callback)
	{
		m_CreateCheckSpecialPicSmsSendCallback = callback;
	}

	/**
	 * @brief 个性账号安全手机确认短信发送回调
	 *
	 * 业务说明：
	 *   - 个性账号触发安全手机验证
	 *   - 用于高风险登录场景
	 */
	void SetCreateDoSpecialConfirmSmsSendCallback(
		SdoBase_CreateDoSpecialConfirmSmsSendCallback callback)
	{
		m_CreateDoSpecialConfirmSmsSendCallback = callback;
	}

	/**
	 * @brief 个性账号短信登录回调
	 *
	 * 业务说明：
	 *   - 个性账号通过短信验证码登录
	 *   - 返回 userid / ticket / autokey 等核心登录信息
	 */
	void SetCreatDoSpecialCheckSmsLoginCallback(
		SdoBase_CreateDoSpecialCheckSmsLoginCallback callback)
	{
		m_CreateDoSpecialCheckSmsLoginCallback = callback;
	}

	//======================================================
	// 支付体系
	//======================================================

	/**
	 * @brief 统一收银台支付回调
	 *
	 * 业务说明：
	 *   - 创建支付订单成功后触发
	 *   - 返回支付URL、订单号等信息
	 *   - 客户端跳转至收银台页面
	 */
	void SdoBase_SetCreatPayEntranceCallback(
		SdoBase_CreatePayEntranceCallback callback)
	{
		m_CreateDoPayEntranceCallback = callback;
	}

	/**
	 * @brief 支付订单状态查询回调
	 *
	 * 业务说明：
	 *   - 查询支付订单结果
	 *   - 返回订单支付状态（成功/失败/处理中）
	 */
	void SdoBase_SetCreatCreateCheckPayOrderStatusCallback(
		SdoBase_CreateCheckPayOrderStatus callback)
	{
		m_CreateDoCheckPayOrderStatuseCallback = callback;
	}

	/**
	 * @brief 激活码校验回调
	 *
	 * 业务说明：
	 *   - 校验激活码是否合法
	 *   - 返回成功或失败信息
	 */
	void SdoBase_SetCreatCreateCheckActivationCallback(
		SdoBase_CreateCheckActivation callback)
	{
		m_CreateDoCheckActivationCallback = callback;
	}

	//======================================================
	// 协议与合规体系
	//======================================================

	/**
	 * @brief 获取游戏协议内容回调
	 *
	 * 业务说明：
	 *   - 获取服务协议URL与HTML内容
	 *   - 获取隐私协议URL与HTML内容
	 *   - 用于客户端展示完整协议页面
	 */
	void SetCreatGetAgreementUrlContentCallback(
		SdoBase_CreateCheckGameAgreementUrlContentCallback callback)
	{
		m_CreateCheckGameAgreementUrlContentCallback = callback;
	}

	/**
	 * @brief 监护人信息收集回调
	 *
	 * 业务说明：
	 *   - 未成年人实名认证后
	 *   - 需要收集监护人信息
	 *   - 返回是否需要确认、是否限制登录等
	 */
	void SdoBase_SetCreatCheckSetTutorInfoCallback(
		SdoBase_CreateCheckSetTutorInfoCallback callback)
	{
		m_CreateCheckSetTutorInfoCallback = callback;
	}

	/**
	 * @brief 设置 GunionWegame 登录回调
	 *
	 * 业务说明：
	 *   - GunionWegame 登录结果返回时触发
	 *   - 返回 userid / ticket / autokey 等登录信息
	 *
	 * @param callback 回调函数指针
	 */
	void SdoBase_SetCreatCheckGunionWegameLoginCallback(
		SdoBase_CreateGhomeGunionWegameLoginCallback callback)
	{
		m_CreateCheckGunionWegameLoginCallback = callback;
	}

	/**
	 * @brief 设置 Gunion 获取公钥回调
	 *
	 * 业务说明：
	 *   - Gunion 获取公钥结果返回时触发
	 *   - 返回 publicKey
	 *
	 * @param callback 回调函数指针
	 */
	void SdoBase_SetCreatCheckGunionGetPublicKeyCallback(
		SdoBase_CreateGunionGetPublicKeyCallback callback)
	{
		m_CreateCheckGunionGetPublicKeyCallback = callback;
	}

	/**
	 * @brief 设置 Gunion 握手回调
	 *
	 * 业务说明：
	 *   - Gunion 握手结果返回时触发
	 *   - 返回 token
	 *
	 * @param callback 回调函数指针
	 */
	void SdoBase_SetCreatCheckGunionHandShakeCallback(
		SdoBase_CreateGunionHandShakeCallback callback)
	{
		m_CreateCheckGunionHandShakeCallback = callback;
	}

	// ============================================================
	//                    登录流程相关接口
	// ============================================================
	// 本区域是登录器业务的核心入口，每个函数代表一种登录方式或登录前置操作。
	//
	// CurlHttpThread/HttpThread 会将请求发送到服务器，
	// 服务器返回结果后 LoginClient::ProcessResponse 会解析 JSON，
	// 并最终触发对应的回调函数（在上一段设置）。
	//
	// 注意：所有接口参数均不能删除或修改，因为兼容旧版本产品（FF14、传奇4、WeGame、Woool 等）。
	// VS2019 已全面支持 C++11，因此默认参数由 NULL ==>nullptr
	// ============================================================

	/**
	 * 设置 HTTP 超时时间
	 * @param timeout         整体超时（毫秒）
	 * @param secTimeout      连接阶段超时（毫秒）
	 */
	int SetTimeout(int timeout, int secTimeout);

	/**
	 * 修改应用信息（AppId、区域、分组）
	 * 用于切换大区 / 游戏版本 / 产品环境。
	 */
	int ModifyAppInfo(int appId, int areaId, int groupId);

	/**
	 * 主动获取 dynamicKey（登录前置步骤）
	 * 需要配合回调：SdoBase_GetDynamicKeyCallback
	 */
	int GetDynamicKey();

	// ----------------------------------------------------------------------
	//                          静态口令登录（账号 + 密码）
	// ----------------------------------------------------------------------

	/**
	 * 静态登录（账号 + 密码方式）
	 * @param inputUserId        用户账号
	 * @param password           密码
	 * @param accountDomain      账号类型（手机号/邮箱/普通账号）
	 * @param autoLoginFlag      是否自动登录
	 * @param autoLoginKeepTime  自动登录有效期
	 * @param isSupportGeetest   是否支持极验
	 * @param keepLoginFlag      登录成功后是否保存为快速登录账号
	 */
	int StaticLogin(const char* inputUserId, const char* password, int accountDomain,
		int autoLoginFlag, int autoLoginKeepTime,
		int isSupportGeetest, int keepLoginFlag);

	/** 同 StaticLogin，但用于部分渠道/分支版本兼容 */
	int StaticLogin2(const char* inputUserId, const char* password, int accountDomain,
		int autoLoginFlag, int autoLoginKeepTime,
		int isSupportGeetest, int keepLoginFlag);

	/**
	 * 静态登录（支持游戏账号类型）
	 * @param inputUserType   游戏自定义账号类型 ID
	 * @param scene           登录场景（如绑定手机、云游戏等）默认 nullptr
	 */
	int StaticLoginWithGameAccount(const char* inputUserId, const char* password, int accountDomain,
		int autoLoginFlag, int autoLoginKeepTime,
		int isSupportGeetest, int inputUserType,
		int keepLoginFlag, const char* scene = nullptr);

	// ----------------------------------------------------------------------
	//                          WeGame 登录
	// ----------------------------------------------------------------------
	/**
	 * WeGame 平台登录
	 * @param rail_id              WeGame 用户标识
	 * @param rail_session_ticket  WeGame SessionTicket
	 * @param is_limited           是否是受限账号
	 */
	int WeGameLogin(const char* rail_id, const char* rail_session_ticket, bool is_limited);

	/** 重载：新增 company_id（庆余年 / 逆水寒等场景） */
	int WeGameLogin(const char* rail_id, const char* rail_session_ticket,
		bool is_limited, int company_id);

	// ----------------------------------------------------------------------
	//                          QQGame 登录
	// ----------------------------------------------------------------------
	/**
	 * QQGame 渠道登录
	 * @param openid          QQ用户唯一ID
	 * @param openkey         QQ登录态凭证
	 * @param is_limited      是否受限
	 * @param company_id      游戏项目 ID
	 */
	int QQGameLogin(const char* openid, const char* openkey,
		bool is_limited, int company_id);


	// ----------------------------------------------------------------------
	//                三方登录（Lenovo / 顺网 / 第三方平台）
	// ----------------------------------------------------------------------
	/**
	 * 三方登录（一般渠道：联想/顺网等使用）
	 * @param third_token     第三方平台 Token
	 * @param companyId       游戏业务 ID
	 * @param scene           登录场景（可选）
	 * @param phone           手机号（可选）
	 * @param smsCode         短信验证码（可选）
	 * @param extend          扩展参数 JSON（可选）
	 * @param szIsLimited     是否受限（可选）
	 */
	int ThirdLogin(const char* third_token, const char* companyId,
		const char* scene = nullptr,
		const char* phone = nullptr,
		const char* smsCode = nullptr,
		const char* extend = nullptr,
		const char* szIsLimited = nullptr);

	// ----------------------------------------------------------------------
	//                      三方登录（腾讯系：QQ/WX/WB）
	// ----------------------------------------------------------------------
	/**
	 * 三方登录（QQ / 微信 / 微博）
	 * @param third_token     OAuth Token
	 * @param companyId       游戏业务 ID
	 * @param scene           登录场景
	 * @param phone           手机号
	 * @param smsCode         短信验证码
	 * @param extend          扩展 JSON
	 * @param szIsLimited     是否受限
	 */
	int ThirdForLogin(const char* third_token, const char* companyId,
		const char* scene = nullptr,
		const char* phone = nullptr,
		const char* smsCode = nullptr,
		const char* extend = nullptr,
		const char* szIsLimited = nullptr);

	// ======================================================================
	//                二、验证码 / 动态口令 / 实名 / 自动登录 系列接口
	// ======================================================================
	// 这些接口属于登录流程中的中间步骤，通常是 GetDynamicKey 成功后触发。
	// 每个接口成功后会进入 ProcessResponse，由映射表 m_mapResponseFunc
	// 调用对应的 ProcessXXX 方法，然后最终触发业务回调。
	// ======================================================================

	/**
	 * 登录（带图验）/ 极验验证码登录
	 * ------------------------------------------------------------------
	 * 通常用于极验场景（Geetest challenge / validate / seccode）
	 * 或登录带图形验证码的流程。
	 *
	 * @param checkCode     图形验证码 / 数字验证码
	 * @param challenge     极验 challenge（服务器下发）
	 * @param validate      极验 validate（客户端提交）
	 * @param seccode       极验 seccode（客户端提交）
	 * @param outinfo       业务扩展数据，例如日志字段、子场景信息
	 * @param keepLoginFlag 登录成功后是否进入快速登录列表
	 * @param captchaInfo   v3 或 v4 图验扩展字段 JSON（默认为 nullptr）
	 */
	int CheckCodeLogin(const char* checkCode,
		const char* challenge,
		const char* validate,
		const char* seccode,
		const char* outinfo,
		int keepLoginFlag,
		const char* captchaInfo = nullptr);

	/**
	 * 动态密码登录（安全卡 / 令牌 / 手机动态密码）
	 * ------------------------------------------------------------------
	 * 用于 RSA 动态Key + 动态口令登录流程。
	 *
	 * @param password        动态密码
	 * @param keepLoginFlag   是否加入快速登录列表
	 */
	int DynamicLogin(const char* password, int keepLoginFlag);

	/**
	 * 实名认证登录（防沉迷实名流程）
	 * ------------------------------------------------------------------
	 * 中国大陆真实姓名 + 身份证号 + 邮箱登录方式。
	 *
	 * @param realName        姓名
	 * @param idCard          身份证号
	 * @param email           邮箱
	 * @param keepLoginFlag   是否加入快速登录列表
	 */
	int FcmLogin(const char* realName,
		const char* idCard,
		const char* email,
		int keepLoginFlag);

	/**
	 * 自动登录（AutoLogin）
	 * ------------------------------------------------------------------
	 * 用于快速登录列表中的“自动登录票据”。
	 *
	 * @param autoLoginSessionKey   服务器返回的快速登录 Key
	 */
	int AutoLogin(const char* autoLoginSessionKey);

	// ======================================================================
	//                    三、SSO 单点登录 / 快速登录
	// ======================================================================

	/**
	 * SSO 登录（基于 TGT 的二次登录）
	 * ------------------------------------------------------------------
	 * 常用于 Launcher 启动游戏 / H5 页面扫码等流程。
	 *
	 * @param tgt           TGT 主票据（Ticket Granting Ticket）
	 * @param scene         登录场景（如 "bindPhone" / "payment"）
	 */
	int SsoLogin(const char* tgt, const char* scene);

	/**
	 * 重载版本，支持自定义 appId（兼容旧版产品）
	 */
	int SsoLogin(const char* tgt, const char* scene, int appId);

	/**
	 * 获取授权码（AuthCode）
	 * ------------------------------------------------------------------
	 * 类似 OAuth，用于在不同业务线（如游戏/平台）之间授权登录。
	 *
	 * @param tgt       TGT 主票据
	 * @param scene     授权场景
	 */
	int GetAuthCodeRequest(const char* tgt, const char* scene);

	/**
	 * 超快速登录（无需密码 / 无需验证码）
	 * ------------------------------------------------------------------
	 * 部分产品（如新版本 Woool）提供的内部快速登录逻辑。
	 */
	int FastLogin();

	/**
	 * 手机短信验证码登录（不带图验）
	 * ------------------------------------------------------------------
	 * 用于“短信验证码”直接登录流程。
	 *
	 * @param checkCode         6 位短信验证码
	 */
	int PhoneCheckCodeLogin(const char* checkCode);

	/**
	 * 扫码登录（QR Code）
	 * ------------------------------------------------------------------
	 * 用于手机扫码 → PC 登录的场景。
	 *
	 * @param autoLoginFlag       是否执行快速登录保存
	 * @param autoLoginKeepTime   快速登录有效期（秒）
	 * @param keepLoginFlag       是否加入快速登录列表
	 */
	int QrCodeLogin(int autoLoginFlag, int autoLoginKeepTime, int keepLoginFlag);

	/**
	 * 退出登录（清除 session / 清除 TGT）
	 * ------------------------------------------------------------------
	 * 用于登录器主动退出、切换账号等操作。
	 */
	int Logout();

	// ======================================================================
	//                    四、账号类型 / 手机验证码 / 状态查询系列接口
	// ======================================================================
	// 这些接口主要用于登录前的准备动作、状态查询、短信验证码、
	// 推送消息（手机推送登录）、会话状态、二维码等。
	// ----------------------------------------------------------------------

	/**
	 * 检测账号类型（邮箱 / 手机号 / 通行证账号）
	 * ------------------------------------------------------------------
	 * 用于客户端自动判断用户输入的 ID 属于哪种登录方式。
	 *
	 * 服务端可能返回：
	 *   - 0：通行证账号
	 *   - 1：手机号
	 *   - 2：邮箱
	 *   - ... （不同业务不同）
	 *
	 * 回调：m_checkAccountCallback
	 *
	 * @param inputUserId  用户输入的账号（手机号、邮箱或通行证 ID）
	 */
	int CheckAccoutType(const char* inputUserId);

	/**
	 * 发送手机验证码短信
	 * ------------------------------------------------------------------
	 * 用于短信登录、绑定手机、风控验证等场景。
	 *
	 * @param inputUserId  手机号或账号
	 * @param type         短信类型（1=登录，2=找回密码…）
	 *
	 * 回调：m_phoneCheckCodeCallback
	 */
	int SendPhoneCheckCode(const char* inputUserId, int type);

	/**
	 * 查询 SessionId 状态
	 * ------------------------------------------------------------------
	 * 用于部分扫码/风控场景，通过 SessionId 查询当前状态：
	 *   - 是否已扫码
	 *   - 是否已确认
	 *   - 是否已登录成功
	 *
	 * 回调：m_getSessionIdStatesCallBack
	 */
	int GetSessionIdStates(const char* sessioId);

	/**
	 * 获取二维码（扫码登录）
	 * ------------------------------------------------------------------
	 * PC 获取二维码 → 手机 App 扫码
	 *
	 * 回调：m_getCodeKeyCallback
	 */
	int GetQrCode();

	/**
	 * 查询登录状态（一般用于扫码登录轮询）
	 * ------------------------------------------------------------------
	 * - 手机扫码
	 * - 手机确认登录
	 * - PC 检测状态
	 *
	 * 回调：m_loginStatusCallback
	 */
	int GetLoginState();

	/**
	 * 查询登录状态（带 TGT）
	 * ------------------------------------------------------------------
	 * 场景：扫码登录或 SSO 登录后，需要根据 TGT 再次确认。
	 */
	int GetLoginState2(const char* tgt);

	/**
	 * 延迟登录态
	 * ------------------------------------------------------------------
	 * 某些业务（如多地登录、锁号场景）需要扩展状态查询。
	 *
	 * 回调：m_extendLoginStateCallback
	 */
	int ExtendLoginState(const char* tgt);

	// ======================================================================
	//                           五、授权码验证
	// ======================================================================

	/**
	 * 授权码校验（AuthCode → Ticket）
	 * ------------------------------------------------------------------
	 * 类似 OAuth2 中的 Authorization Code → Access Token。
	 *
	 * 用于某些业务：
	 *   - 支付操作
	 *   - 第三方授权换票据
	 *   - 二次授权
	 *
	 * 回调：m_CreateAuchCodeCallback（或特定回调）
	 */
	int CheckAuthCodeRequest(const char* authCode);

	// ======================================================================
	//                六、TGT（Ticket Granting Ticket）管理
	// ======================================================================

	/**
	 * 获取当前已存在的 TGT（上一次登录成功后缓存）
	 * ------------------------------------------------------------------
	 * 用于自动登录、支付、SSO、扫码确认等场景。
	 */
	string GetTgt();

	/**
	 * 获取当前已存在的 Token（上一次Gunion登录成功后缓存）
	 * ------------------------------------------------------------------
	 * 用于扫码支付确认等场景。
	 */
	string GetToken();

	/**
	 * 获取当前已存在的 标志位,是否走gunion逻辑
	 * ------------------------------------------------------------------
	 * 用于扫码支付确认等场景。
	 */
	bool GetEnableGunionPreInit();

	/**
	 * 设置 TGT（手动覆盖）
	 * ------------------------------------------------------------------
	 * 场景：某些产品从外部（如游戏）传递 TGT 进来。
	 */
	void SetTgt(const string& tgt);

	// ======================================================================
	//                  七、推送一键登录（Push Message Login）
	// ======================================================================

	/**
	 * 查询推送消息状态
	 * ------------------------------------------------------------------
	 * 用于“推送登录”业务：
	 *   - 用户输入账号
	 *   - 后端向该账号绑定手机号推送登录通知
	 *   - 用户手机确认后，PC 登录
	 *
	 * @param inputUserId  用户账号
	 *
	 * 回调：m_getPushMessageStatusCallback
	 */
	int GetPushMessageStatus(const char* inputUserId);

	/**
	 * 推送一键消息登录（无需密码/验证码）
	 * ------------------------------------------------------------------
	 * 叨鱼“手机确认登录”，需用户手机确认。
	 *
	 * @param autoLoginFlag       是否开启快速登录
	 * @param autoLoginKeepTime   快速登录有效期
	 * @param keepLoginFlag       登录成功是否加入快速列表
	 *
	 * 回调：m_sendPushMessageCallback → loginResultCallback
	 */
	int PushMessageLogin(int autoLoginFlag,
		int autoLoginKeepTime,
		int keepLoginFlag);

	/**
	 * 发送一键登录请求
	 * ------------------------------------------------------------------
	 * PC 请求后：
	 *   → 服务器向用户手机推送一条“是否允许登录”通知
	 *   → 手机 App 点击确认后，PC 自动完成登录
	 *
	 * @param inputUserId   用户账号（手机号/邮箱/通行证）
	 * @param scene         扩展场景（默认为 nullptr）
	 *
	 * 回调：m_sendPushMessageCallback
	 */
	int SendPushMessage(const char* inputUserId,
		const char* scene = nullptr);

	/**
	 * 取消一键登录（用户主动取消）
	 * ------------------------------------------------------------------
	 * PC 端点击“取消登录”按钮时调用。
	 *
	 * 回调：m_cancelPushMessageCallback
	 */
	int CancelPushMessageLogin();

	// ======================================================================
	//                         八、网络代理与 Rlt 登录
	// ======================================================================

	/**
	 * 设置是否启用代理（影响 HttpThread / CurlHttpThread）
	 * ------------------------------------------------------------------
	 * @param enable  1=使用系统代理，0=不使用任何代理（直连）
	 */
	int ProxyEnable(int enable);

	/**
	 * RLT 登录（登录即注册）
	 * ------------------------------------------------------------------
	 * 网页的登录即注册
	 *
	 * @param vkey      业务侧提供的 RLT key
	 */
	int RltLogin(const char* vkey);

	// ======================================================================
	//              一、账号信息 / 登录历史 / 用户资料 / 区服 数据查询
	// ======================================================================

	/**
	 * 获取账号信息
	 * ------------------------------------------------------------------
	 * 用于查询账号的基础信息：
	 *   - 绑定手机号
	 *   - 绑定邮箱
	 *   - 注册时间
	 *   - 状态（封号/冻结等）
	 *
	 * 回调：m_getAccountInfoCallback
	 */
	int GetAccountInfo(const char* tgt);


	/**
	 * 获取账号登录历史
	 * ------------------------------------------------------------------
	 * 可用于：
	 *   - 展示最近登录设备
	 *   - 风控验证
	 *   - 登录位置提示
	 *
	 * @param tgt            当前登录用户的 TGT
	 * @param queryNumber    查询数量（如最近 10 条）
	 *
	 * 回调：m_getLoginHistoryCallback
	 */
	int GetLoginHistory(const char* tgt, int queryNumber);


	/**
	 * 查询用户登录资料（可显示在登录器界面）
	 * ------------------------------------------------------------------
	 * 内容一般包括：
	 *   - 昵称
	 *   - 头像
	 *   - 个性签名
	 *
	 * 回调：m_getLoginUserInfoCallback
	 */
	int GetLoginUserInfo(const char* tgt);


	/**
	 * 修改用户资料（目前常用于设置备注名）
	 * ------------------------------------------------------------------
	 * @param tgt        登录 TGT
	 * @param notename   新的备注名
	 *
	 * 回调：m_setLoginUserInfoCallback
	 */
	int SetLoginUserInfo(const char* tgt, const char* notename);


	/**
	 * 获取区服信息（Area / Group）
	 * ------------------------------------------------------------------
	 * 用于游戏区服展示：
	 *   - AppId → 服务器分区
	 *   - 游戏列表中的分区结构
	 *
	 * 回调：m_getLoginAreaInfoCallback
	 */
	int GetLoginAreaInfo(int nAppId);

	// ======================================================================
	//           账号组查询 / 账号组登录（多个子账号体系）
	// ======================================================================

	/**
	 * 查询账号组信息（账号关联的小号体系）
	 * ------------------------------------------------------------------
	 * 用途：
	 *   - 玩家一个大号下绑定多个子账号
	 *   - 不同游戏分不同子账号体系
	 *
	 * 回调：ProcessGetAccountGroupResponse
	 */
	int GetAccountGroup(const char* tgt);

	/**
	 * 账号组登录（用于绑定的子账号登录）
	 * ------------------------------------------------------------------
	 * @param tgt               主账号 TGT
	 * @param sndaId            要登录的子账号 ID
	 * @param autoLoginFlag     是否加入自动登录列表
	 * @param autoLoginKeepTime 快速登录有效期（秒）
	 */
	int AccountGroupLogin(const char* tgt,
		const char* sndaId,
		int autoLoginFlag,
		int autoLoginKeepTime);

	// ======================================================================
	//           三、第三方轮询登录 / 第三方直接登录（多平台）
	// ======================================================================

	/**
	 * 第三方轮询登录（Polling）
	 * ------------------------------------------------------------------
	 * 用于：
	 *   - 手机扫码后手机 App 轮询是否授权成功
	 *   - Lenovo / 网维平台自动检测
	 *
	 * @param companyId         第三方公司 ID
	 * @param autoLoginFlag     是否加入快速登录
	 * @param autoLoginKeepTime 快速登录有效期（秒）
	 */
	int ThirdPartyPollingLogin(const char* companyId,
		int autoLoginFlag,
		int autoLoginKeepTime);

	/**
	 * 第三方直接登录（Lenovo / 顺网 / 第三方 SDK 登录）
	 * ------------------------------------------------------------------
	 * @param companyId     平台公司 ID
	 * @param token         第三方下发的登录 token
	 * @param autoLoginFlag 是否加入快速登录列表
	 * @param autoLoginKeepTime 快速登录有效期
	 * @param scene         扩展字段（如绑定手机的场景，不需要可传 nullptr）
	 * @param phone         扩展手机字段（某些第三方会传）
	 * @param smsCode       扩展验证码字段
	 */
	int ThirdPartyLogin(const char* companyId,
		const char* token,
		int autoLoginFlag,
		int autoLoginKeepTime,
		const char* scene = nullptr,
		const char* phone = nullptr,
		const char* smsCode = nullptr);

	/**
	 * 云游戏登录
	 * ------------------------------------------------------------------
	 * 云游戏平台将玩家登录数据转发给登录中心，用于无客户端登录。
	 *
	 * @param tgt        来自云游戏平台的 TGT
	 * @param scene      扩展字段，默认 nullptr
	 */
	int CloudGameLogin(const char* tgt, const char* scene = nullptr);

	// ======================================================================
	//                        短信 / MiGu / 图验短信
	// ======================================================================

	/**
	 * 发送咪咕短信（Telecom 运营商合作业务）
	 */
	int SendMiGuSms(const char* phone);

	/**
	 * 发送短信（通用接口）
	 * ------------------------------------------------------------------
	 * @param smsSessionKey   短信会话 Key
	 * @param phone           手机号
	 * @param smsType         短信类型（登录 / 绑定 / 风控）
	 * @param scene           扩展场景（默认 nullptr）
	 */
	int SendSms(const char* smsSessionKey,
		const char* phone,
		int smsType,
		const char* scene = nullptr);

	/**
	 * 图验 + 短信校验（验证码 + 图形验证码）
	 * ------------------------------------------------------------------
	 * 用于：
	 *   - 图形验证码通过后 → 发送短信
	 *   - 防止短信轰炸、IP 限频等风控场景
	 *
	 * @param smsSessionKey   会话 Key
	 * @param checkCode       图形验证码
	 * @param captchaInfo     图验扩展 JSON（默认 nullptr）
	 */
	int CheckCodeToSendSms(const char* smsSessionKey,
		const char* checkCode,
		const char* captchaInfo = nullptr);

	// ======================================================================
	//                       短信+自动注册（SmsLogin）
	// ======================================================================

	/**
	 * 短信一键注册 / 一键登录（融合型接口）
	 * ------------------------------------------------------------------
	 * 业务背景：
	 *   新版本中，短信验证码登录可能直接创建账号（自动注册），
	 *   无需用户输入密码。此接口同时支持自动登录。
	 *
	 * @param smsSessionKey        短信会话 key
	 * @param smsCode              用户输入的短信验证码
	 * @param autoLoginFlag        是否启用自动登录（快速登录）
	 * @param autoLoginKeepTime    自动登录有效期（秒）
	 * @param keepLoginFlag        是否加入快速登录列表
	 *
	 * 回调：loginResultCallback
	 */
	int SmsLogin(const char* smsSessionKey,
		const char* smsCode,
		int autoLoginFlag,
		int autoLoginKeepTime,
		int keepLoginFlag);

	// ======================================================================
	//                 用户隐私协议（旧版 + 新版）
	// ======================================================================

	/**
	 * 用户隐私协议（旧版）
	 * ------------------------------------------------------------------
	 * @param scene                   调用场景
	 * @param privacypolicyversion    隐私政策版本号
	 * @param serviceAgreementVersion 服务协议版本号
	 *
	 * 回调：m_UserPrivacyConfigCallback
	 */
	int UserPrivacyConfig(const char* scene,
		int privacypolicyversion,
		int serviceAgreementVersion);

	/**
	 * 用户隐私协议（新版接口）
	 * ------------------------------------------------------------------
	 * 参数更简化，只保留最关键的部分。
	 *
	 * 回调：m_NewVersionUserPrivacyConfigCallback
	 */
	int NewVersionUserPrivacyConfig(const char* scene,
		int privacypolicyversion);

	// ======================================================================
	//                        人脸识别系统
	// ======================================================================

	/**
	 * 初始化人脸识别流程
	 * ------------------------------------------------------------------
	 * 用途：
	 *   - 拉取人脸识别 session
	 *   - 获取动作序列（眨眼、张嘴、转头等）
	 *   - 服务器下发 contextId，供后续步骤使用
	 *
	 * @param scene    人脸识别场景（如防沉迷、实名校验）
	 *
	 * 回调：m_FaceVerifyInitCallback
	 */
	int FaceVerifyInit(const char* scene);

	/**
	 * 获取人脸识别动作码结果
	 * ------------------------------------------------------------------
	 * 手机端识别动作后回调 PC：
	 *   - 成功：用户通过验证
	 *   - 失败：动作不匹配/识别失败
	 *
	 * @param contextId   FaceVerifyInit 返回的识别上下文 ID
	 *
	 * 回调：m_GetFaceCodeResultCallback
	 */
	int GetFaceCodeResult(const char* contextId);

	/**
	 * 人脸识别动作上报（SendAction）
	 * ------------------------------------------------------------------
	 * 用于将“用户进行了某个动作”反馈给服务器，例如：
	 *   - action = 1：张嘴
	 *   - action = 2：眨眼
	 *   - action = 3：抬头
	 *
	 * @param contextId   FaceVerifyInit 返回的 contextId
	 * @param action      用户动作序号
	 *
	 * 回调：m_SendActionCallback
	 */
	int SendAction(const char* contextId, int action);

	// ======================================================================
	//                卫士通 UE 安全加解密（国产密码体系）
	// ======================================================================

	/**
	 * UE SDK 初始化
	 * ------------------------------------------------------------------
	 * 此接口通常用于国产安全体系：
	 *   - 客户端完整性校验
	 *   - 反作弊安全链路
	 *   - 加密环境初始化
	 *
	 * @param hash   客户端哈希（或设备指纹）
	 *
	 * 回调：m_UeInitClientCallback
	 */
	int UeInitClient(const char* hash);

	/**
	 * UE 安全上报接口
	 * ------------------------------------------------------------------
	 * 用于将客户端运行状态 / 指纹信息 / 风控数据上报至服务器。
	 *
	 * @param szExtendData   JSON 扩展数据
	 */
	int UeReport(const char* szExtendData);

	// ======================================================================
	//                   Steam 支付（端游 / 手游）
	// ======================================================================

	/**
	 * Steam 端游支付结果通知（PC 端）
	 * ------------------------------------------------------------------
	 * 用途：
	 *   当 Steam 游戏端完成支付后（如 DLC / 充值），
	 *   客户端需要通知服务器进行订单确认（票据验证）。
	 *
	 * @param szSteamId   Steam 用户 64 位数字 ID
	 * @param szOrderId   商户订单号
	 */
	int SendSteamPayResult(const char* szSteamId, const char* szOrderId);


	/**
	 * Steam 手游渠道支付结果通知
	 * ------------------------------------------------------------------
	 * 用于手游端（Steam 移动渠道），会附带 ticket。
	 *
	 * @param szChannel   渠道标识（例如 steam_android）
	 * @param szTicket    SDK 获取的用户临时票据
	 * @param szOrderId   商户订单号
	 */
	int SendSteamChannelPayResult(const char* szChannel,
		const char* szTicket,
		const char* szOrderId);

	// ======================================================================
	//                       WeGame（包括庆余年）
	// ======================================================================

	/**
	 * 获取 WeGame 登录票据（庆余年接口要求）
	 * ------------------------------------------------------------------
	 * 用途：
	 *   WeGame 某些游戏（如庆余年）需要额外的 ticket，
	 *   用于创建订单或登录验证。
	 *
	 * 回调：m_GetTicketCallBack
	 */
	int GetTicket();

	/**
	 * 创建 WeGame 订单
	 * ------------------------------------------------------------------
	 * 典型业务：
	 *   - 用户在 WeGame 中购买道具
	 *   - SDK 需要生成订单
	 *   - 服务器返回支付参数
	 *
	 * @param ticket      登录票据（由 GetTicket 获取）
	 * @param szOrderId   订单号
	 * @param szProductId 商品 ID
	 * @param szGroupId   分组（区组）
	 * @param szAreaId    区服 ID
	 * @param szExtend    扩展字段（JSON）
	 * @param szSign      签名
	 * @param channel     渠道，固定为 wegame
	 *
	 * 回调：m_CreateWeGameOrderCallback
	 */
	int CreateWeGameOrder(const char* ticket,
		const char* szOrderId,
		const char* szProductId,
		const char* szGroupId,
		const char* szAreaId,
		const char* szExtend,
		const char* szSign,
		const char* channel);

	/**
	 * 查询 WeGame 状态（登录态 / 封禁 / 冻结等）
	 *
	 * 回调：m_WeGameStatusCallback
	 */
	int WeGameStatus();

	// ======================================================================
	//                        传奇 4（Mir4）订单
	// ======================================================================

	/**
	 * 创建传奇 4 订单
	 * ------------------------------------------------------------------
	 * 业务：
	 *   - Mir4 国际版 / 国内特殊区服使用
	 *   - 提交商品 + 签名 + ticket 创建订单
	 */
	int CreateCQ4Order(const char* ticket,
		const char* szOrderId,
		const char* szProductId,
		const char* szGroupId,
		const char* szAreaId,
		const char* szExtend,
		const char* szSign,
		const char* channel,
		int iSSandboxAccount);

	/**
	 * 创建传奇 4 查询订单（查询充值状态）
	 *
	 * 回调：m_CreateCQ4QueryCallback
	 */
	int CreateCQ4Query(const char* ticket,
		const char* szOrderId,
		const char* szSign,
		const char* channel,
		int iSSandboxAccount);

	// ======================================================================
	//               三方灵活动态皮肤展示（登录 UI 动态换肤）
	// ======================================================================

	/**
	 * 获取三方登录灵活皮肤配置
	 * ------------------------------------------------------------------
	 * 用途：
	 *   - 获取 QQ / WX / WB / Lenovo 等三方的动态皮肤
	 *   - 用于登录 UI 根据渠道自动加载不同皮肤包
	 *
	 * 回调：m_CreateThirdLoginSkinCallback
	 */
	int GetThirdLoginSkin();

	// ======================================================================
	//                           联想（LX）支付
	// ======================================================================

	/**
	 * 创建联想渠道支付订单
	 * ------------------------------------------------------------------
	 * @param szLenovoTgt 联想登录返回的 tgt
	 * @param szRole      游戏角色
	 *
	 * 回调：m_CreateLxOrderCallback
	 */
	int CreateLxOrder(const char* ticket,
		const char* szOrderId,
		const char* szProductId,
		const char* szGroupId,
		const char* szAreaId,
		const char* szExtend,
		const char* szSign,
		const char* channel,
		const char* szLenovoTgt,
		const char* szRole);

	// ======================================================================
	//					Steam（手游渠道）标准下单接口
	// ======================================================================

	/**
	 * Steam 手游创建订单（与 SteamChannelPayResult 配套）
	 *
	 * 回调：m_CreateSteamChannelOrderCallback
	 */
	int CreateSteamChannelOrder(const char* ticket,
		const char* szOrderId,
		const char* szProductId,
		const char* szGroupId,
		const char* szAreaId,
		const char* szExtend,
		const char* szSign,
		const char* channel);

	// ======================================================================
	//                       QQGame QQ 游戏中心支付
	// ======================================================================

	/**
	 * 创建 QQGame 支付订单
	 * ------------------------------------------------------------------
	 * 必须包含 openid / openkey / pfkey（区服标识）
	 *
	 * 回调：m_CreateQQGameOrderCallback
	 */
	int CreateQQGameOrder(const char* ticket,
		const char* szOrderId,
		const char* szProductId,
		const char* szGroupId,
		const char* szAreaId,
		const char* szExtend,
		const char* szOpenId,
		const char* szOpenKey,
		const char* szPfKey,
		const char* szSign,
		const char* channel);

	/**
	 * QQGame 是否已登录检测
	 * ------------------------------------------------------------------
	 * 用于判断是否需要重新登录 QQGame。
	 *
	 * 回调：m_QQGameIsLoginCallback
	 */
	int QQGameIsLogin(const char* openid,
		const char* openkey,
		int company_id);

	// ======================================================================
	//                      踢下线（安全相关）
	// ======================================================================

	/**
	 * 发起踢下线验证码校验
	 * ------------------------------------------------------------------
	 * 用途：
	 *   某些业务要求用户验证身份后才能踢下线。
	 *
	 * 回调：m_callbackKickoffAccountVerify
	 */
	int KickoffAccountVerify(const char* tgt);

	/**
	 * 使用保护码进行踢下线登录
	 */
	int KickoffProtectCodeLogin(const char* tgt,
		const char* protectCode);

	/**
	 * 使用短信校验码进行踢下线
	 *
	 * @param nKickoffAppid  目标 AppID
	 * @param nKickoffAreaid 目标区服
	 */
	int KickoffCheckCodeLogin(const char* tgt,
		const char* checkCode,
		int nKickoffAppid,
		int nKickoffAreaid);

	// ======================================================================
	//                         通用 Key-Value 存取接口
	// ======================================================================

	/**
	 * 设置 SDK 内部可访问的通用 Key-Value
	 * ------------------------------------------------------------
	 * 用途：
	 *   - 写入一些动态参数（如自定义扩展字段、调试参数）
	 *   - 内部以 m_mapValList 存储
	 *
	 * @param keyVal    参数名
	 * @param val       参数值
	 */
	int SetValue(const char* keyVal, const char* val);

	/**
	 * 获取 SetValue 写入的 Key-Value
	 *
	 * @param keyVal  参数名
	 * @param val     输出缓存（由调用者分配）
	 */
	int GetValue(const char* keyVal, char* val);

	/**
	 * 设置任意回调（通过 Key 名称）
	 * ------------------------------------------------------------
	 * 用途：
	 *   - 动态注册一些业务回调函数
	 *   - 存储在 m_mapCallback 中
	 *
	 * @param funcName   回调函数名称
	 * @param func       函数指针
	 */
	int SetCallBack(const char* funcName, const void* func);

	// ======================================================================
	//                    等待服务器响应 / 取消请求
	// ======================================================================

	/**
	 * 阻塞等待 HTTP 响应
	 * ------------------------------------------------------------
	 * 内部实现：
	 *   - LoginClient 在收到网络线程返回时 SetEvent(m_waitEvent)
	 *   - 此方法在 UI 线程阻塞等待
	 *
	 * @param timeout   超时时间（毫秒）
	 */
	int WaitResponse(int timeout);

	/**
	 * 取消当前请求（调用 HTTP 线程 cancel）
	 * ------------------------------------------------------------
	 * 内部调用：
	 *   - HttpThread::Cancel()
	 *   - CurlHttpThread::Cancel()
	 */
	int Cancel();

	// ======================================================================
	//                          营销推广信息
	// ======================================================================

	/**
	 * 获取推广信息（如广告、推广活动）
	 *
	 * 回调：ProcessGetPromotionInfoResponse
	 */
	int GetPromotionInfo();

	/**
	 * 推广信息确认（例如“已阅读”）
	 *
	 * @param days  有效期天数
	 */
	int PromotionInfoConfirm(int days);

	// ======================================================================
	//                      PublicKey / DynamicKey 相关
	// ======================================================================

	/**
	 * 获取服务器 PublicKey（公钥，用于加解密）
	 *
	 * 回调：ProcessGetPublicKeyResponse
	 */
	int GetPublicKey();

	/**
	 * 返回 PublicKey 的 HTTP 请求对象（内部使用）
	 */
	HttpRequest* GetPublicKeyRequest();

	/**
	 * 获取动态密钥（DynamicKey）专用 Request
	 * ------------------------------------------------------------
	 * @param publicKey  已获取到的公钥
	 */
	HttpRequest* GetDynamicKeyRequest(string publicKey);

	/**
	 * 解析 PublicKey 服务器返回的数据（JSON → 字段解析）
	 *
	 * @param reqeustCode  请求码 REQ_GetPublicKey
	 * @param keyValues    JSON 解析后的键值表
	 * @param publicKey    输出变量：解析后的公钥
	 */
	int ParesePublicKey(int reqeustCode,
		map<string, string>& keyValues,
		string& publicKey);

	// ======================================================================
	//                     参数系统（Req/Resp Params）
	// ======================================================================

	/**
	 * 设置请求参数（内部用于构建统一的参数 Map）
	 *
	 * 存储到：m_mapReqParams
	 */
	int SetParam(const char* keyVal, const char* val);

	/**
	 * 获取请求或响应参数
	 * ------------------------------------------------------------
	 * 用途：
	 *   - 对外提供访问 m_mapRespParams
	 */
	int GetParam(const char* keyVal, char* val);

	// ======================================================================
	//                PushMessage 登录状态（智能推送一键登录）
	// ======================================================================

	/**
	 * 清除 PushMessage 验证码状态，
	 * 用于保证下一次短信登录逻辑正常。
	 */
	void ClearPushMessageVerifyCheckCodeStatus();

	// ======================================================================
	//                     VKey / UserAccount 相关接口
	// ======================================================================

	/**
	 * 获取客户端 VKey（某些服务需要的二次验证 Key）
	 *
	 * 回调：ProcessGetClientVKeyResponse
	 *
	 * @param tgt  用户登录票据
	 */
	int GetClientVKey(const char* tgt);

	/**
	 * 发送用户账号给服务器（例如上报登录账号）
	 *
	 * 回调：ProcessSendUserAccountResponse
	 */
	int SendUserAccount(const char* inputUserId);

	// ======================================================================
	//                             UserData
	// ======================================================================

	/**
	 * 设置用户自定义数据（透传到 HttpThread userData）
	 *
	 * @param userData 任意指针（SDK 内部不管理生命周期）
	 */
	int SetUserData(const void* userData);

	/**
	 * 获取与本次请求关联的用户自定义数据
	 */
	void* GetUserData();

	///////////////////////////////////////////////////////////////////////////////
	// 登录器功能相关配置（ClientConfig）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 请求登录器功能配置
	 *
	 * 用途：
	 *   - 客户端启动时请求服务器端下发的配置项
	 *   - 包含：功能开关、灰度参数、皮肤显示、是否需要短信验证等
	 *
	 * 回调：
	 *   - 对应 SetGetClientConfigCallback()
	 *
	 * 返回：
	 *   0 = 已成功发起请求（结果在回调中返回）
	 */
	int GetClientConfig();

	///////////////////////////////////////////////////////////////////////////////
	// 下行短信登录（GoDown 系列业务）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 下行短信登录初始化
	 *
	 * 参数：
	 *   - account  用户账号（手机号/用户名）
	 *   - isVoice  是否语音验证码 ("0"/"1")
	 *   - m_flowid 当前业务流程 ID
	 *
	 * 回调：
	 *   - SetGoDownConfigCallback()
	 */
	int GoDownConfigInit(const char* account, const char* isVoice, const char* m_flowid);

	/**
	 * @brief 请求下行短信图形验证码（图验）
	 *
	 * 参数：
	 *   - captchaInfo  图形验证码相关信息
	 *
	 * 回调：
	 *   - SetGoDownSendSmsCheckCodeCallBack()
	 */
	int GoDownSendSmsCheckCode(const char* captchaInfo);

	/**
	 * @brief 请求验证码（短信/语音发送）
	 *
	 * 参数：
	 *   - isVoice 是否语音验证码
	 *
	 * 回调：
	 *   - SetGoDownconfirmSendSmsCallBack()
	 */
	int GoDownconfirmSendSms(int isVoice);

	/**
	 * @brief 下行短信最终登录
	 *
	 * 参数：
	 *   - account      用户账号
	 *   - verifyCode   短信验证码
	 *   - keepLoginFlag 是否自动登录
	 *
	 * 回调：
	 *   - SetGoDownconfirmLoginCallBack()
	 */
	int GoDownconfirmLogin(const char* account, const char* verifyCode, int keepLoginFlag);

	///////////////////////////////////////////////////////////////////////////////
	// 快速登录（FastLogin 系列）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 快速登录
	 *
	 * 参数：
	 *   - autoLoginKey 快速登录 Key
	 *
	 * 回调：
	 *   - SetFastLoginLoginCallBack()
	 */
	int FastLogin(const char* autoLoginKey);

	///////////////////////////////////////////////////////////////////////////////
	// 新版 GetTicket（GetTicketNew）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 获取新版登录票据（取代旧 GetTicket）
	 *
	 * 回调：
	 *   - SetGetTicketLoginCallBack()
	 */
	int GetTicketNew();

	///////////////////////////////////////////////////////////////////////////////
	// 快速登录主动删除账号
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 快速登录入口：主动删除账号
	 *
	 * 参数：
	 *   - keepLoginKey 自动登录 Key
	 *   - inputUserId  要删除的账号
	 *
	 * 回调：
	 *   - SetFastLoginActiveDeleteAccountCallBack()
	 */
	int FastLoginActiveDeleteAccount(const char* keepLoginKey, const char* inputUserId);

	///////////////////////////////////////////////////////////////////////////////
	// 检查账号登录类型（验证码/实名/人脸/短信）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 查询账号的登录类型
	 *
	 * 用途：
	 *   - 某些账号首次登录时需要判断是否触发实名
	 *   - 某些账号可能需要短信验证
	 *   - 有些账号属于人脸识别流程
	 *
	 * 参数：
	 *   - sndaId 盛趣账号 ID（业务字段）
	 *
	 * 回调：
	 *   - SetCheckAccountTypeLoginCallback()
	 */
	int CheckAccountLoginType(const char* sndaId);

	///////////////////////////////////////////////////////////////////////////////
	// 星座运势计算（CheckCalculate）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 星座运势计算（娱乐功能）
	 *
	 * 参数：
	 *   - star_sign  星座英文名（如 "aries"）
	 *   - date       日期（yyyy-mm-dd）
	 *
	 * 回调：
	 *   - SetCalculateCallback()
	 */
	int CheckCalculate(const char* star_sign, const char* date);

	///////////////////////////////////////////////////////////////////////////////
	// 广告系统初始化（InitAdv）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 广告系统初始化
	 *
	 * 用途：
	 *   - 登录页广告、公告、Banner 刷新
	 *
	 * 回调：
	 *   - SetInitAdvCallback()
	 */
	int CheckInitAdv();

	/**
	 * @brief 设置 TraceId（全链路跟踪 ID）
	 */
	int SetTraceId(const char* traceId);

	/**
	 * @brief 清理验证码 key（与图验系统绑定）
	 */
	int ClearCodeKey();

	///////////////////////////////////////////////////////////////////////////////
	// 三、监护人信息补填（未成年人保护业务）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 监护人短信发送（无图验）
	 *
	 * 参数：
	 *   - flowId   当前业务流程 ID
	 *   - phone    监护人手机号
	 *   - isVoice  是否语音验证码（0/1）
	 *
	 * 回调：
	 *   - SetGuardDianSendSmsCallBack()
	 */
	int GuardDianSendSms(const char* flowId, const char* phone, int isVoice);

	/**
	 * @brief 监护人短信发送（带图形验证码）
	 *
	 * 参数：
	 *   - flowId        流程 ID
	 *   - captchaInfo   图验信息
	 *   - isVoice       是否语音验证码
	 *
	 * 回调：
	 *   - SetGuardDianSendSmsCheckCodeCallback()
	 */
	int GuardDianSendSmsCheckCode(const char* flowId, const char* captchaInfo, int isVoice);

	/**
	 * @brief 监护人提交短信验证结果
	 *
	 * 参数：
	 *   - ageCheckFlag 未成年人标记
	 *   - flowId       流程 ID
	 *   - verifyCode   短信验证码
	 *   - tutorName    监护人姓名
	 *   - tutorIdCard  监护人身份证
	 *   - phone        监护人手机号
	 *
	 * 回调：
	 *   - SetGuardDianConfrimSendSmsResultCallBack()
	 */
	int GuardDianConfrimSendSmsResult(
		int ageCheckFlag,
		const char* flowId,
		const char* verifyCode,
		const char* tutorName,
		const char* tutorIdCard,
		const char* phone);

	///////////////////////////////////////////////////////////////////////////////
	// 其他功能
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 设置登录器 SDO 版本号（内部标识）
	 */
	int SetSdoVersion(const char* sdoVersion);

	/**
	 * @brief RLT 登录：保留会话并自动登录
	 *
	 * 参数：
	 *   - vkey          登录票据
	 *   - keepLoginFlag 自动登录标记
	 */
	int RltLoginKeepLogin(const char* vkey, int keepLoginFlag);

	/**
	 * @brief 设置运行计时器 ID（用于调试与追踪）
	 */
	int SetRunTimerId(const char* runTimeId);

	///////////////////////////////////////////////////////////////////////////////
	// 人脸打开/人身核验功能
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 人脸打开Url获取
	 * 参数：
	 *   - confType      配置类型（当前场景固定：移动端<FACIAL_RECOGNITION_URL>、PC<FACIAL_RECOGNITION_PC_URL>）
	 *   - confKey       配置键（当前场景固定：-1）
	 */
	int SetFaceRecognitionUrl(const char* confType, const char* confKey);

	/**
	 * @brief 查询人脸识别票据状态,验证成功或者失败
	 * 参数：
	 *   - appId         应用ID
	 *   - ticket        人脸核验的票据
	 */
	int SetQueryFaceVerifyTicketStatus(const char* appId, const char* ticket);

	/**
	 * @brief 获取人脸识别页面URL/账号持有人登记页面URL
	 * 参数：
	 *   - confType      配置类型（当前场景固定：移动端<FACIAL_RECOGNITION_URL>、PC<FACIAL_RECOGNITION_PC_URL>）
	 *   - confKey       配置键（当前场景固定：-1）
	 */
	int SetFaceHolderRegistrationUrl(const char* confType, const char* confKey);

	/**
	 * @brief 激活码登录
	 * 参数：
	 *   - activeCode    激活码
	 */
	int GetActiveCodeLogin(const char* activeCod);

	///////////////////////////////////////////////////////////////////////////////
	// GunionWegame 回调设置
	///////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////
	// GunionWegame 功能接口
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief Gunion-WeGame 初始化（会话初始化 / 业务环境绑定）
	 *
	 * 业务说明：
	 *   - 在 Gunion 握手完成、会话密钥建立之后调用
	 *   - 使用当前 Gunion 会话密钥对初始化参数进行 3DES 加密
	 *   - 同时计算初始化请求所需的 signature
	 *   - 向 /v1/account/initialize 接口发起请求
	 *
	 * 流程说明：
	 *   1. 基于当前会话上下文（MAC、deviceId 等）构造初始化参数
	 *   2. 使用 Gunion 会话密钥进行 3DES(Base64) 加密，生成 POST 数据
	 *   3. 按协议规则计算请求签名（signature）
	 *   4. 构建并投递初始化 HTTP 请求
	 *
	 * 调用前置条件：
	 *   - 已成功完成 GetPublicKey 流程
	 *   - 已成功完成 Gunion Handshake（会话密钥已建立）
	 *   - 当前 Gunion 会话处于有效状态
	 *
	 * @param appId
	 *   - 当前应用的 AppId，用于构建初始化请求的通用参数
	 *
	 * @return
	 *   - 0     初始化请求已成功构造并投递
	 *   - 非 0  初始化参数构造、加密、签名或请求投递失败
	 */
	int CheckGunionWegameInit(const char* appId);

	/**
	 * @brief GunionWegame 登录
	 *
	 * 参数：
	 *   - rail_id              WeGame 用户 ID
	 *   - rail_session_ticket  WeGame 会话票据
	 *   - is_limited           是否受限账号
	 *   - company_id           公司 ID
	 *
	 * @return 0 成功，非 0 失败
	 */
	int GetCheckGunionWegameLogin(
		const char* rail_id,
		const char* rail_session_ticket,
		bool is_limited,
		int company_id);

	/**
	 * @brief GunionWegame 短信发送
	 *
	 * @param phone                     手机号
	 * @param voice_msg                 是否是语言验证码
	 * @param str_new_deviceid_server   新设备标识
	 *
	 * @return 0 成功，非 0 失败
	 */
	int CheckGunionWegameSmsSend(
		const char* phone,
		const char* voice_msg,
		const char* str_new_deviceid_server);

	/**
	 * @brief GunionWegame 带图验短信发送
	 *
	 * @param imagecodeType             图验类型
	 * @param voice_msg                 语音提示文案
	 * @param captchaInfo               图形验证码信息
	 * @param str_new_deviceid_server   新设备标识
	 *
	 * @return 0 成功，非 0 失败
	 */
	int CheckGunionWegamePicSmsSend(
		const char* imagecodeType,
		const char* voice_msg,
		const char* captchaInfo,
		const char* str_new_deviceid_server);

	/**
	 * @brief GunionWegame 三方账号绑定手机号
	 *
	 * @param phone                     手机号
	 * @param sms_code                  短信验证码
	 * @param str1_new_deviceid_server  新设备标识
	 *
	 * @return 0 成功，非 0 失败
	 */
	int CheckGunionWegameThirdAccountBindPhone(
		const char* phone,
		const char* sms_code,
		const char* str1_new_deviceid_server);

	/**
	 * @brief GunionWegame 实名补填
	 *
	 * @param idcard                    身份证号
	 * @param name                      姓名
	 * @param realInfoFlowId            实名流程 ID
	 * @param str_new_deviceid_server   新设备标识
	 *
	 * @return 0 成功，非 0 失败
	 */
	int CheckGunionWegameFillRealinfo(
		const char* idcard,
		const char* name,
		const char* realInfoFlowId,
		const char* str_new_deviceid_server);

	/**
	 * @brief GunionWegame 获取登录票据
	 *
	 * @param time_s                    票据有效时间（秒）
	 * @param str_new_deviceid_server   新设备标识
	 *
	 * @return 0 成功，非 0 失败
	 */
	int CheckGunionWegameGetTicket(
		int time_s,
		const char* str_new_deviceid_server);

	/**
	 * @brief GunionWegame 获取支付票据
	 *
	 * @param time_s                    票据有效时间（秒）
	 * @param str_new_deviceid_server   新设备标识
	 *
	 * @return 0 成功，非 0 失败
	 */
	int CheckGunionWegameGetPayTicket(
		int time_s,
		const char* str_new_deviceid_server);

	/**
	 * @brief GunionWegame 设置 token
	 *
	 * @param token 登录 token
	 * @return 0 成功，非 0 失败
	 */
	int CheckGunionWegameSetToken(const char* token);

	/**
	 * @brief GunionWegame 设置 randKey
	 *
	 * @param randKey 随机密钥
	 * @return 0 成功，非 0 失败
	 */
	int CheckGunionWegameSetRandKey(const char* randKey);

public:
	/**
	*	GhomePC相关接口
	*/
	/**
	 * @brief 请求隐私协议跳转地址
	 *
	 * @param appId   应用ID
	 * @param scene   场景标识（登录/注册等）
	 *
	 * @return 0 成功发起请求，非0失败
	 */
	int CheckDoPrivateRequest(const char* appId, const char* scene);


	/**
	 * @brief 短信验证码发送
	 *
	 * @param phone                     手机号
	 * @param voice_msg                 是否是语言验证码
	 * @param smsType                   短信验证码类型，短信登录:4，未成年用户监护人信息收集:9
	 * @param str_new_deviceid_server   新设备标识
	 *
	 * @return 0 成功
	 */
	int CheckDoSmsSend(
		const char* phone,
		const char* voiceMsg,
		const char* smsType,
		const char* str_new_deviceid_server);

	/**
	 * @brief 短信验证码发生带图验发送
	 *
	 * @param imagecodeType             图验类型
	 * @param voice_msg                 语音提示文案
	 * @param captchaInfo               图形验证码信息
	 * @param smsType                   短信验证码类型，短信登录:4，未成年用户监护人信息收集:9
	 * @param str_new_deviceid_server   新设备标识
	 *
	 * @return 0 成功
	 */
	int CheckCheckPicSmsSend(
		const char* imagecodeType,
		const char* voice_msg,
		const char* captchaInfo,
		const char* smsType,
		const char* str_new_deviceid_server);

	/**
	 * @brief 短信验证码登录
	 *
	 * @param phone                     手机号
	 * @param sms_code                  验证码
	 * @param str_new_deviceid_server   新设备标识
	 *
	 * @return 0 成功
	 */
	int CheckCheckSmsLogin(const char* phone,
		const char* sms_code,
		const char* str_new_deviceid_server);

	/**
	 * @brief 实名认证校验
	 *
	 * @param idcard 身份证号
	 * @param name   姓名
	 * @param str_new_deviceid_server   新设备标识
	 *
	 * @return 0 成功
	 */
	int CheckCheckRealName(const char* idcard,
		const char* name,
		const char* str_new_deviceid_server);

	/**
	 * @brief 自动登录
	 *
	 * @param autokey 登录自动key
	 * @param str_new_deviceid_server   新设备标识
	 */
	int CheckCheckAutoLogin(const char* autokey,
		const char* str_new_deviceid_server);

	/**
	 * @brief 登出（标准流程）
	 *
	 * @param autokey 自动key
	 * @param str_new_deviceid_server   新设备标识
	 */
	int CheckCheckLogout(const char* autokey,
		const char* str_new_deviceid_server);

	/**
	 * @brief 密码登录
	 *
	 * @param phone    账号
	 * @param password 密码
	 * @param str_new_deviceid_server   新设备标识
	 */
	int CheckDoPwdLogin(const char* phone,
		const char* password,
		const char* str_new_deviceid_server);

	/**
	 * @brief 图验密码登录
	 *
	 * @param imageType    图验类型
	 * @param captchaInfo  图验数据
	 * @param str_new_deviceid_server   新设备标识
	 */
	int CheckDoPicPwdLogin(const char* imageType,
		const char* captchaInfo,
		const char* str_new_deviceid_server);

	/**
	 * @brief 第三方登录
	 *
	 * @param company_id   渠道ID
	 * @param third_ticket 三方票据
	 * @param code_verify  校验码
	 * @param openid       三方openid
	 * @param uid          三方uid
	 * @param str_new_deviceid_server   新设备标识
	 */
	int CheckDoThirdLogin(const char* company_id, const char* third_ticket,
		const char* code_verify, const char* openid, const char* uid,
		const char* str_new_deviceid_server);

	/**
	 * @brief 个性账号静密登录
	 * @param account                   账号
	 * @param password                  密码
	 * @param str_new_deviceid_server   新设备标识
	 */
	int CheckDoSpecialPwdLogin(const char* account,
		const char* password,
		const char* str_new_deviceid_server);

	/**
	 * @brief 个性账号图验静密登录
	 * @param imageType                 图验类型
	 * @param captchaInfo               图验数据
	 * @param str_new_deviceid_server   新设备标识
	 */
	int CheckDoSpecialPicPwdLogin(const char* imageType,
		const char* captchaInfo,
		const char* str_new_deviceid_server);

	/**
	 * @brief 个性账号短信发送
	 *
	 * 业务说明：
	 *   - 个性账号登录流程中发送短信验证码
	 *   - 可能返回图验信息（普通图验或极验）
	 *
	 * @param account                   个性账号（用户名/账号ID）
	 * @param voice_msg                 语音提示文案（用于语音验证码场景,0/1）
	 * @param str_new_deviceid_server   新设备标识
	 *
	 * @return 0 成功发起请求，非 0 失败
	 */
	int CheckDoSpecialSmsSend(const char* account,
		const char* voice_msg,
		const char* str_new_deviceid_server);

	/**
	 * @brief 个性账号图验短信发送
	 * @param check_code   图验类型
	 * @param captchaInfo  图验校验数据
	 * @param str_new_deviceid_server   新设备标识
	 */
	int CheckSpecialCheckPicSmsSend(const char* check_code,
		const char* captchaInfo,
		const char* str_new_deviceid_server);

	/**
	 * @brief 个性账号确认安全手机
	 *
	 * 业务说明：
	 *   - 个性账号触发安全验证流程
	 * @param str_new_deviceid_server   新设备标识
	 *
	 * @return 0 成功发起请求，非 0 失败
	 */
	int CheckDoSpecialConfirmSmsSend(const char* str_new_deviceid_server);

	/**
	 * @brief 个性账号短信登录
	 *
	 * @param account   个性账号
	 * @param sms_code  短信验证码
	 * @param str_new_deviceid_server   新设备标识
	 *
	 * @return 0 成功发起请求，非 0 失败
	 */
	int CheckDoSpecialCheckSmsLogin(const char* account,
		const char* sms_code,
		const char* str_new_deviceid_server);

	/**
	 * @brief 写入登录区服记录
	 *
	 * 业务说明：
	 *   - 用户登录成功后记录区服信息
	 *
	 * @param userid  用户ID
	 * @param area    区服大区
	 * @param group   区服分组
	 * @param str_new_deviceid_server   新设备标识
	 *
	 * @return 0 成功，非 0 失败
	 */
	int CheckDoLoginArea(const char* userid,
		const char* area,
		const char* group,
		const char* str_new_deviceid_server);

	/**
	 * @brief 获取区服记录
	 *
	 * 业务说明：
	 *   - 查询用户最近登录的区服信息
	 * @param str_new_deviceid_server   新设备标识
	 *
	 * @return 0 成功，非 0 失败
	 */
	int CheckDoGetLoginArea(const char* str_new_deviceid_server);

	/**
	 * @brief 创建统一收银台支付订单
	 *
	 * @param szOrderId         游戏侧订单号
	 * @param szProductId       商品ID
	 * @param szGroupId         区服分组ID
	 * @param szAreaId          区服ID
	 * @param szExtend          扩展字段（JSON或自定义参数）
	 * @param iSSandboxAccount  是否沙盒账号（1=沙盒，0=正式）
	 * @param str_new_deviceid_server   新设备标识
	 *
	 * @return 0 成功发起请求，非 0 失败
	 */
	int CheckDoCreatPayEntrance(const char* szOrderId,
		const char* szProductId,
		const char* szGroupId,
		const char* szAreaId,
		const char* szExtend,
		int iSSandboxAccount,
		const char* str_new_deviceid_server);

	/**
	 * @brief 查询支付订单状态
	 *
	 * @param szOrderId         游戏侧订单号
	 * @param iSSandboxAccount  是否沙盒账号
	 * @param str_new_deviceid_server   新设备标识
	 *
	 * @return 0 成功发起请求，非 0 失败
	 */
	int CheckDoCreateCheckPayOrderStatus(const char* szOrderId,
		int iSSandboxAccount, const char* str_new_deviceid_server);

	/**
	 * @brief 激活码校验
	 *
	 * @param activation  激活码字符串
	 * @param str_new_deviceid_server   新设备标识
	 *
	 * @return 0 成功发起请求，非 0 失败
	 */
	int CheckDoCreateCheckActivation(const char* activation,
		const char* str_new_deviceid_server);

	/**
	 * @brief 获取服务协议与隐私协议内容
	 *
	 * @param appId  应用ID
	 * @param scene  场景标识（登录/注册等）
	 *
	 * @return 0 成功发起请求，非 0 失败
	 */
	int CheckGameAgreementUrlContent(const char* appId, const char* scene);

	/**
	 * @brief 监护人信息收集
	 *
	 * 业务说明：
	 *   - 未成年人实名认证后
	 *   - 需提交监护人信息
	 *
	 * @param idcard      监护人身份证号
	 * @param name        监护人姓名
	 * @param phone       监护人手机号
	 * @param smscode     短信验证码
	 * @param confirmKey  二次确认Key
	 * @param str_new_deviceid_server   新设备标识
	 *
	 * @return 0 成功发起请求，非 0 失败
	 */
	int CheckSetTutorInfo(const char* idcard,
		const char* name,
		const char* phone,
		const char* smscode,
		const char* confirmKey,
		const char* str_new_deviceid_server);

	/**
	 * @brief GunionWegame 登录
	 *
	 * @param selectLoginContext  登录上下文（渠道返回信息）
	 * @param companyId           渠道公司ID
	 * @param str_new_deviceid_server   新设备标识
	 *
	 * @return 0 成功发起请求，非 0 失败
	 */
	int CheckDoGunionWegameLogin(const char* selectLoginContext,
		const char* companyId,
		const char* str_new_deviceid_server);

	/**
	 * @brief Gunion 获取公钥
	 *
	 *
	 * @return 0 成功发起请求，非 0 失败
	 */
	int CheckDoGunionGetPublicKey();

	/**
	 * @brief Gunion 握手
	* @param publicKey   握手得到的公钥
	 *
	 *
	 * @return 0 成功发起请求，非 0 失败
	 */
	int CheckDoGunionHandShake(const char* publicKey);
public:

	///////////////////////////////////////////////////////////////////////////////
	// 网络地址解析工具（静态方法）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 解析服务器地址字符串
	 *
	 * 功能：
	 *   输入形态可能为：
	 *     "login.sdo.com:443"
	 *     "login.sdo.com"
	 *     "10.2.3.4:80"
	 *
	 *   本函数会自动拆解成：
	 *     hostName = "login.sdo.com"
	 *     port     = 443（默认 80）
	 *
	 * 使用场景：
	 *   Initialize / Initialize2 中解析 serverAddr、backupServerAddr
	 */
	static void ParseHttpAddr(
		const string& serverAddr,
		string& hostName,
		int& port);

	///////////////////////////////////////////////////////////////////////////////
	// 多域名切换（Host3 / Host4）
	//-----------------------------------------------------------------------------
	// 业务说明：
	//   host1 / host2 为主域名/备用域名
	//   host3 / host4 为“备用备援域名”（短信校验、动密、风控使用）
	//
	//   HttpThread / CurlHttpThread 内部会自动切换：
	//      1) host1 失败 → host2
	//      2) host1+host2 都失败 → host3 / host4
	//
	//   这里提供接口供外部配置 host3/4。
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 配置备用备援域名（host3 / host4）
	 *
	 * @param hostname3   第三个域名（如：login3.sdo.com）
	 * @param hostport3   对应端口
	 * @param hostname4   第四个域名（如：login4.sdo.com）
	 * @param hostport4   对应端口
	 *
	 * 使用场景：
	 *   调岗、机房迁移时给客户端提供新的容灾域名。
	 */
	void SetHost3AndHost4(string hostname3, int hostport3,
		string hostname4, int hostport4);

	///////////////////////////////////////////////////////////////////////////////
	// 多域名切换（Host5 / Host6）
	//-----------------------------------------------------------------------------
	// 业务说明：
	//   新版“下行短信登录 / 安全服务”需要额外的 host5/host6。
	//
	//   HttpThread / CurlHttpThread 会根据 loginserver 返回的配置
	//   动态决定是否切换到 host5/6。
	//
	//   这里提供接口供外部配置 host5/6。
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 配置第五/第六域名（host5 / host6）
	 *
	 * @param hostname5   第五域名（如：sms5.sdo.com）
	 * @param hostport5   对应端口
	 * @param hostname6   第六域名（如：sms6.sdo.com）
	 * @param hostport6   对应端口
	 *
	 * 使用场景：
	 *   与新风控通道、短信下发服务对应。
	 */
	void SetHost5AndHost6(string hostname5, int hostport5,
		string hostname6, int hostport6);

	///////////////////////////////////////////////////////////////////////////////
	// 错误码字符集安全处理（UTF-8 / GBK）
	//-----------------------------------------------------------------------------
	// 业务背景：
	//   - 登录 / 短信 / 风控等接口返回的错误信息，
	//     大多数为 UTF-8 编码。
	//   - 但历史原因或特定服务返回的部分错误码，
	//     实际内容为 GBK 编码字符串。
	//
	//   - 若直接按 UTF-8 使用，会导致中文乱码、UI 显示异常。
	//   - 因此需要根据错误码，判断是否需要进行 UTF-8 → GBK 转换。
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 判断指定错误码是否需要按 GBK 编码处理
	 *
	 * 业务说明：
	 *   - 针对部分历史遗留或特殊服务返回的错误码
	 *   - 这些错误码对应的错误信息并非标准 UTF-8
	 *
	 * @param code 服务端返回的错误码
	 *
	 * @return
	 *   - true  ：该错误码对应的错误信息需要按 GBK 处理
	 *   - false ：错误信息可直接按 UTF-8 使用
	 */
	bool NeedGbkForCode(int code);

	/**
	 * @brief 获取安全可显示的错误信息字符串
	 *
	 * 业务说明：
	 *   - 根据错误码判断字符集
	 *   - 自动进行 UTF-8 → GBK 转换（如有需要）
	 *   - 避免调用层自行判断字符集，导致逻辑分散
	 *
	 * @param utf8Msg 服务端返回的错误信息（原始 UTF-8 指针）
	 * @param code    对应的错误码
	 *
	 * @return
	 *   - 返回可直接用于 UI 显示的安全字符串
	 */
	std::string GetSafeMsg(const char* utf8Msg, int code);

private:

	///////////////////////////////////////////////////////////////////////////////
	// 工具函数
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 生成 GUID（作为 uniqueId / traceId 使用）
	 */
	string CreateGUID();


	///////////////////////////////////////////////////////////////////////////////
	// 网络响应总入口
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 统一处理 HTTP 返回结果
	 *
	 * 负责：
	 *   1. 调用 MsgProcess 解析 JSON → keyValues
	 *   2. 根据 requestCode 调用对应的 ProcessXXXResponse()
	 *   3. 将结果派发给各业务回调
	 *
	 * @param result       HTTP 结果（0=成功）
	 * @param requestCode  请求类型枚举，如 REQ_StaticLogin / REQ_GetDynamicKey
	 * @param response     HTTP Body（JSON 字符串）
	 * @param vecCookies   HTTP Set-Cookie 列表
	 * @param isUrl2       是否来自备用域名 host2
	 */
	void ProcessResponse(
		int result,
		int requestCode,
		const string& response,
		const vector<string>& vecCookies,
		bool isUrl2);

	///////////////////////////////////////////////////////////////////////////////
	// 动态密钥（PublicKey → DynamicKey）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理获取 DynamicKey 的返回结果
	 *
	 * 解析字段：
	 *   - dynamicKey
	 *   - expires
	 *
	 * 回调：
	 *   m_getDynamicKeyCallback
	 */
	void ProcessGetDynamicKeyResponse(int reqeustCode,
		map<string, string>& keyValues);

	///////////////////////////////////////////////////////////////////////////////
	// 登录流程（静密/动密/实名登录/扫码）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理登录接口（StaticLogin / DynamicLogin / FcmLogin）
	 *
	 * 关键字段：
	 *   - ticket
	 *   - resultCode
	 *   - secureLevel
	 *   - needSms / needCaptcha
	 */
	void ProcessLoginResponse(int reqeustCode,
		map<string, string>& keyValues);

	///////////////////////////////////////////////////////////////////////////////
	// 账号类型检测
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理账号类型校验（邮箱/手机/ID/第三方）
	 *
	 * 关键字段：
	 *   - accountType
	 */
	void ProcessCheckAccoutTypeResponse(int reqeustCode,
		map<string, string>& keyValues);

	///////////////////////////////////////////////////////////////////////////////
	// 手机短信验证码（发送验证码接口）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理发送手机验证码的返回结果
	 *
	 * 关键字段：
	 *   - smsSessionKey
	 */
	void ProcessSendPhoneCheckCodeResponse(int reqeustCode,
		map<string, string>& keyValues);

	///////////////////////////////////////////////////////////////////////////////
	// 二维码登录（获取二维码）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理获取二维码（GetQrCode）
	 *
	 * 关键字段：
	 *   - scene
	 *   - qrcodeUrl
	 *   - sessionKey
	 */
	void ProcessGetQrCodeResponse(int reqeustCode,
		map<string, string>& keyValues);

	///////////////////////////////////////////////////////////////////////////////
	// 登录状态查询（扫码轮询）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理登录状态轮询（GetLoginState）
	 *
	 * 关键字段：
	 *   - loginStatus
	 *   - tgt / ticket
	 */
	void ProcessGetLoginStatusResponse(int reqeustCode,
		map<string, string>& keyValues);

	///////////////////////////////////////////////////////////////////////////////
	// 服务端延长登录态
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理 ExtendLoginState 返回结果
	 *
	 * 关键字段：
	 *   - extendResult
	 */
	void ProcessExtendLoginStateResponse(int reqeustCode,
		map<string, string>& keyValues);

	///////////////////////////////////////////////////////////////////////////////
	// 注销
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理注销（Logout）
	 */
	void ProcessLogoutResponse(int reqeustCode,
		map<string, string>& keyValues);


	///////////////////////////////////////////////////////////////////////////////
	// 一键登录 / 推送消息登录（手机号一键登录）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理发送一键登录推送消息
	 */
	void ProcessSendPushMessageResponse(int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理一键登录状态查询
	 */
	void ProcessGetPushMessageStatusResponse(int reqeustCode,
		map<string, string>& keyValues);

	///////////////////////////////////////////////////////////////////////////////
	// 获取账号信息 / 历史 / 取消推送
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理获取账号信息返回
	 */
	void ProcessGetAccountInfoResponse(int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理获取登录历史返回
	 */
	void ProcessGetLoginHistoryResponse(int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理取消推送登录返回
	 */
	void ProcessCancelPushMessageResponse(int reqeustCode,
		map<string, string>& keyValues);

	///////////////////////////////////////////////////////////////////////////////
	// PublicKey（用于生成 DynamicKey）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理 PublicKey 返回（生成 DynamicKey 的前置步骤）
	 *
	 * 关键字段：
	 *   - publicKey
	 */
	void ProcessGetPublicKeyResponse(int reqeustCode,
		map<string, string>& keyValues);

	///////////////////////////////////////////////////////////////////////////////
	// 营销活动信息（登录推广信息获取）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理营销活动信息（如礼包、推广任务）
	 */
	void ProcessGetPromotionInfoResponse(int reqeustCode,
		map<string, string>& keyValues);

	///////////////////////////////////////////////////////////////////////////////
	// 人身核验相关流程
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 人脸返回url
	 */
	void ProcessCreateGetFaceRecognitionUrlRequestResponse(int reqeustCode, map<string, string>& keyValues);

	/**
	 * @brief 查询人脸识别票据状态
	 */
	void ProcessCreateQueryFaceVerifyTicketStatusRequestResponse(int reqeustCode, map<string, string>& keyValues);

	/**
	 * @brief 信息收集返回url
	 */
	void ProcessCreateGetFaceHolderRegistrationUrlRequestResponse(int reqeustCode, map<string, string>& keyValues);
private:

	///////////////////////////////////////////////////////////////////////////////
	// 推广任务确认（PromotionInfoConfirm）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理推广信息确认（PromotionInfoConfirm）
	 *
	 * 用途：
	 *   - 某些游戏在登录后会有“推广任务 / 激励任务”需要确认用户已阅读
	 *
	 * 返回字段示例：
	 *   - confirmResult = 0/1
	 */
	void ProcessPromotionInfoConfirmResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	///////////////////////////////////////////////////////////////////////////////
	// 获取 ClientVKey（增强登录安全校验）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理客户端 VKey 返回
	 *
	 * 用途：
	 *   - 某些安全增强接口要求客户端生成 vkey，并与服务端校验
	 *
	 * 返回字段示例：
	 *   - vkey
	 *   - expire
	 */
	void ProcessGetClientVKeyResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	///////////////////////////////////////////////////////////////////////////////
	// 发送用户账号（行为统计 / 安全分析）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理发送用户账号（SendUserAccount）返回
	 *
	 * 用途：
	 *   - 某些服务需要上报用户账号以执行登录前风控分析
	 */
	void ProcessSendUserAccountResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	///////////////////////////////////////////////////////////////////////////////
	// 获取账号分组信息（如：多区服、角色列表）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理账号分组信息（AccountGroup）
	 *
	 * 关键字段：
	 *   - groupId
	 *   - groupName
	 *   - list (区服/角色)
	 */
	void ProcessGetAccountGroupResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	///////////////////////////////////////////////////////////////////////////////
	// 查询 SessionId 状态（会话检测）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理 SessionId 状态查询返回
	 *
	 * 用途：
	 *   - 某些登录方式中，SessionKey 需要查询状态（如是否可用、是否过期）
	 */
	void ProcessGetSessionIdStateResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	///////////////////////////////////////////////////////////////////////////////
	// 登录后用户信息（昵称、头像等）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理获取登录用户信息（头像昵称等）
	 */
	void ProcessGetLoginUserInfoResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理设置登录用户信息（修改昵称/头像）
	 */
	void ProcessSetLoginUserInfoResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	///////////////////////////////////////////////////////////////////////////////
	// 获取大区信息（Area 列表 / World 列表）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理获取大区列表（GetLoginAreaInfo）
	 *
	 * 用途：
	 *   - 某些游戏需要在登录器内展示区服列表
	 */
	void ProcessGetLoginAreaInfoResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	///////////////////////////////////////////////////////////////////////////////
	// 踢人机制（Kickoff）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理踢人验证（KickoffAccountVerify）
	 *
	 * 关键字段：
	 *   - verifyResult
	 *   - nextStep
	 */
	void ProcessKickoffVerifyResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理踢人结果（KickoffAccountResult）
	 */
	void ProcessKickoffResultResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	///////////////////////////////////////////////////////////////////////////////
	// 扩展短信服务（MiGu、普通短信）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理发送咪咕短信（SendMiGuSms）
	 */
	void ProcessSendMiGuSmsRequestResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理发送短信（SendSms）
	 */
	void ProcessSendSmsRequestResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理图验后发送短信（CheckCodeToSendSms）
	 *
	 * 关键字段：
	 *   - smsSessionKey
	 */
	void ProcessCheckCodeToSendSmsRequestResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	///////////////////////////////////////////////////////////////////////////////
	// 用户隐私协议（新版隐私协议）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理隐私协议提交（UserPrivacyConfig）
	 *
	 * 用途：
	 *   - 登录前强制用户阅读隐私协议
	 *   - 强制版本更新的隐私协议
	 */
	void ProcessUserPrivacyConfigRequestResponse(
		int reqeustCode,
		map<string, string>& keyValues);

private:

	///////////////////////////////////////////////////////////////////////////////
	// 人脸识别（三步流程）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理人脸识别初始化（FaceVerifyInit）
	 *
	 * 用途：
	 *   - 初始化人脸验证流程
	 *   - 服务端返回 contextId、策略、加密参数等
	 */
	void ProcessFaceVerifyInitRequestResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理获取人脸识别结果（GetFaceCodeResult）
	 *
	 * 用途：
	 *   - 拉取当前验证的进度、是否通过、失败原因等
	 */
	void ProcessGetFaceCodeResultRequestResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理人脸识别动作上报（SendAction）
	 *
	 * 用途：
	 *   - 上传动作（如点头、眨眼），用于 liveness 检测
	 */
	void ProcessSendActionRequestResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	///////////////////////////////////////////////////////////////////////////////
	// 获取 Ticket（特殊接口，既可用于安全重定向，也用于跨业务校验）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理 GetTicket 返回
	 *
	 * 常用于：
	 *   - WeGame 登录需要 ticket
	 *   - Steam / 手机渠道也可能需要 ticket
	 */
	void ProcessGetTicketRequestResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	///////////////////////////////////////////////////////////////////////////////
	// WeGame 订单创建流程
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理创建 WeGame 订单返回
	 */
	void ProcessCreateWeGameOrderRequestResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	///////////////////////////////////////////////////////////////////////////////
	// 传奇4（Mir4 / CQ4）订单创建 / 查询
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理传奇4订单创建返回
	 */
	void ProcessCreateCQ4OrderRequestResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理传奇4订单查询返回
	 */
	void ProcessCreateCQ4QueryRequestResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	///////////////////////////////////////////////////////////////////////////////
	// 授权码（AuthCode）→ 返回 ticket
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理授权码验证返回（用于换取 ticket）
	 */
	void ProcessCreateAuthCodeResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	///////////////////////////////////////////////////////////////////////////////
	// 三方登录皮肤灵活展示（UI 动态配置）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理三方登录皮肤配置返回
	 *
	 * 用途：
	 *   - 返回 UI 皮肤、按钮文案、动态展示内容
	 */
	void ProcessGetThirdLoginSkinRequestResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	///////////////////////////////////////////////////////////////////////////////
	// WeGame 状态查询
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理 WeGame 登录状态返回
	 */
	void ProcessWeGameStatusRequestResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	///////////////////////////////////////////////////////////////////////////////
	// 联想(LX) 订单创建
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理联想订单创建返回
	 */
	void ProcessCreateLxOrderRequestResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	///////////////////////////////////////////////////////////////////////////////
	// Steam 手游渠道订单（ChannelOrder）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理 Steam 渠道订单创建返回
	 */
	void ProcessCreateSteamChannelOrderRequestResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	///////////////////////////////////////////////////////////////////////////////
	// QQGame 订单 & 登录状态
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理 QQGame 订单创建返回
	 */
	void ProcessCreateQQGameOrderRequestResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理 QQGame 登录状态返回
	 */
	void ProcessQQGameIsLoginRequestResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	///////////////////////////////////////////////////////////////////////////////
	// 卫士通加解密接口（UeInitClient / UeReport）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理卫士通初始化返回（UeInitClient）
	 */
	void ProcessUeInitClientRequestResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	///////////////////////////////////////////////////////////////////////////////
	// 工具函数（内部使用）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief map<string,string> → map<string,string>
	 *
	 * 简单工具：将 src 拷贝到 dest
	 */
	void MapToMap(
		map<string, string>& dest,
		map<string, string> src);

	/**
	 * @brief 解析推广链接中的 promotionId
	 *
	 * 示例 URL：
	 *   https://xxx.xxx/promo?id=12345
	 *
	 * 提取：
	 *   promotionId = 12345
	 */
	bool ParsePromotionUrl(
		const string url,
		string& promotionId);

private:

	///////////////////////////////////////////////////////////////////////////////
	//下行短信相关（GoDown 系列）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理：获取登录器功能相关配置（GetClientConfig）
	 *
	 * 典型用途：
	 *   - 下行短信开关
	 *   - 快速登录能力是否可用
	 *   - 验证码策略、UI 显示策略
	 */
	void ProcessCreateGetClientConfigRequestResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：账号初始化配置（GoDownConfigInit）
	 *
	 * 返回内容一般包括：
	 *   - flowId（后续图验、短信发送必传）
	 *   - 手机号状态
	 *   - 是否需要语音/短信二选一
	 *   - 是否需要图验
	 */
	void ProcessCreateGetGetGoDownConfigInitRequestResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：获取下行短信图验信息（GoDownSendSmsCheckCode）
	 *
	 * 服务端会返回：
	 *   - 图验参数（challenge / validate / captchaInfo）
	 *   - 图验是否必须
	 */
	void ProcessCreateGetGoDownSendSmsCheckCodeRequestResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：确认发送下行短信（GoDownconfirmSendSms）
	 *
	 * 用途：
	 *   - 校验短信防刷策略
	 *   - 返回真实验证码发送状态
	 */
	void ProcessCreateGetGoDownconfirmSendSmsRequestResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：确认下行短信登录（GoDownconfirmLogin）
	 *
	 * 用途：
	 *   - 最终登录接口（验证码 + 图验 + 场景参数）
	 *   - 返回用户基础资料、票据、账户类型等
	 */
	void ProcessCreateGetGoDownconfirmLoginRequestResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	///////////////////////////////////////////////////////////////////////////////
	// 快速登录（FastLogin 系列）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理：快速登录（FastLogin）
	 *
	 * 典型用途：
	 *   - 使用 fastKey 一键登录
	 *   - 不需要输入账号与密码
	 */
	void ProcessCreateGetFastLoginLoginRequestResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：新版本 GetTicket（GetTicketNew）
	 *
	 * 一般返回：
	 *   - 新版本票据
	 *   - 用于后续第三方订单、会话验证
	 */
	void ProcessCreateGetTicketLoginRequestResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：快速登录主动删除账号
	 *
	 * 用途：
	 *   - 清理快速登录记录
	 *   - 用户可选择移除某个 fastKey
	 */
	void ProcessCreateFastLoginActiveDeleteAccountRequestResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	///////////////////////////////////////////////////////////////////////////////
	// 快速登录账号类型判定
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理：检测快速列表账号类型（CheckAccountLoginType）
	 *
	 * 典型用途：
	 *   - 判断账号是否为 "一键登录账号"
	 *   - 是否绑定手机、是否走下行短信流程等
	 */
	void ProcessCreateCheckAccountLoginTypeRequestResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	///////////////////////////////////////////////////////////////////////////////
	// 星座计算接口（CheckCalculate）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理：星座运算（CheckCalculate）
	 *
	 * 完全业务需求：
	 *   - 输入生日、星座，返回计算结果
	 */
	void ProcessCreateCheckCalculateRequestResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	///////////////////////////////////////////////////////////////////////////////
	// 广告系统初始化（InitAdv）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理：广告系统初始化返回
	 *
	 * 用途：
	 *   - 拉取启动广告、Banner、版本轮播广告配置
	 */
	void ProcessCreateCheckInitAdvRequestResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	///////////////////////////////////////////////////////////////////////////////
	// 监护人信息补填（GuardDian 系列）
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 处理：监护人信息补填短信发送（带/不带图验）
	 */
	void ProcessCreateGuardDianSendSmsRequestResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：监护人补填带图验短信发送
	 *
	 * 典型业务：
	 *   - 使用 captchaInfo 继续完成短信流程
	 */
	void ProcessCreateGuardDianSendSmsCheckCodeRequestResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：监护人补填提交结果
	 *
	 * 用途：
	 *   - 返回审核状态（成功 / 失败 / 补充材料）
	 */
	void ProcessCreateGuardDianConfrimSendSmsResultRequestResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：激活码登录
	 *
	 * 用途：
	 *   - 最终激活码登录接口
	 *   - 返回用户基础资料、票据、账户类型等
	 */
	void ProcessCreateActiveCodeLoginResultRequestResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	// ---------------------- Gunion登录相关响应处理 ----------------------

	/**
	 * @brief 处理：Gunion 获取 RSA 公钥响应（握手第一步）
	 *
	 * 典型业务：
	 *   - 解析服务端返回的 RSA 公钥
	 *   - 构造完整 PEM 格式的公钥字符串
	 *   - 为后续 Handshake（会话密钥协商）阶段做准备
	 *
	 * 注意：
	 *   - 本接口返回数据为明文
	 *   - 不涉及 randKey / token / 3DES 解密
	 */
	void ProcessCreateGunionGetPublicKeyResponse(
		int requestCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：Gunion 握手响应（会话密钥协商 / 建立会话）
	 *
	 * 典型业务：
	 *   - 使用本地生成的 randKey 对服务端返回数据进行 3DES 解密
	 *   - 解析解密后的 JSON 数据
	 *   - 获取并保存会话 token
	 *
	 * 协议说明：
	 *   - 对应接口：/v1/basic/handshake
	 *   - 返回的 data 字段为 3DES 加密的 Base64 字符串
	 *
	 * 注意：
	 *   - 只有在 GetPublicKey 成功后才能调用
	 *   - Handshake 成功后，Gunion 接口进入加密通信阶段
	 */
	void ProcessCreateGunionHandshakeResponse(
		int requestCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：Gunion-WeGame 初始化响应
	 *
	 * 典型业务：
	 *   - 解析初始化返回的配置数据
	 *   - 包含三方登录参数、文案、token、randKey 等信息
	 *   - 为后续登录流程做准备
	 */
	void ProcessCreateGunionWegameInitResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：Gunion-WeGame 登录响应
	 *
	 * 典型业务：
	 *   - 解析登录结果（成功 / 失败）
	 *   - 获取用户标识、登录票据、绑定状态等信息
	 */
	void ProcessGunionWegameLoginResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：Gunion-WeGame 短信发送响应
	 *
	 * 典型业务：
	 *   - 判断短信是否发送成功
	 *   - 可能返回是否需要图形验证码等信息
	 */
	void ProcessCreateGunionWegameSmsSendResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：Gunion-WeGame 图形验证码短信发送响应
	 *
	 * 典型业务：
	 *   - 解析图形验证码 URL / 参数
	 *   - 引导用户完成图验后继续短信流程
	 */
	void ProcessCreateGunionWegamePicSmsSendResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：Gunion-WeGame 三方账号绑定手机响应
	 *
	 * 典型业务：
	 *   - 返回绑定结果
	 *   - 获取绑定后的手机号、账号信息等
	 */
	void ProcessCreateGunionWegameThirdAccountBindPhoneResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：Gunion-WeGame 实名信息提交响应
	 *
	 * 典型业务：
	 *   - 提交实名信息后的结果处理
	 *   - 判断是否通过实名校验或需要补充信息
	 */
	void ProcessCreateGunionWegameFillRealinfoResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：Gunion-WeGame 获取登录票据响应
	 *
	 * 典型业务：
	 *   - 获取并保存登录票据（Login Ticket）
	 *   - 用于后续登录态校验或游戏启动
	 */
	void ProcessCreateGunionWegameGetTicketResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：Gunion-WeGame 获取支付票据响应
	 *
	 * 典型业务：
	 *   - 获取支付流程使用的票据
	 *   - 用于后续下单或支付接口调用
	 */
	void ProcessCreateGunionWegameGetPayTicketResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：Gunion-Private 提交响应
	 *
	 * 典型业务：
	 *   - 隐私协议url返回
	 */
	void ProcessCreateGunionDoPrivateResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：Gunion-GameAgreement 获取游戏协议内容响应
	 */
	void ProcessCreateGunionGetGameAgreementUrlContentResponse(
		int reqeustCode,
		map<string, string>& keyValues);


	/**
	 * @brief 处理：Gunion 短信发送响应
	 *
	 * 典型业务：
	 *   - 判断短信是否发送成功
	 *   - 可能返回是否需要图形验证码等信息
	 */
	void ProcessCreateGunionSmsSendResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：Gunion图形验证码短信发送响应
	 *
	 * 典型业务：
	 *   - 解析图形验证码 URL / 参数
	 *   - 引导用户完成图验后继续短信流程
	 */
	void ProcessCreateGunionPicSmsSendResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：Gunion-SmsLogin offical账号校验短信登录响应
	 */
	void ProcessCreateGunionCheckSmsLoginResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：Gunion-SpecialSmsSend officalpt账号短信发送响应
	 */
	void ProcessCreateGunionDoSpecialSmsSendResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：Gunion-SpecialPicSmsSend officalpt账号图形验证码短信响应
	 */
	void ProcessCreateGunionCheckSpeicalPicSmsSendResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：Gunion-SpecialConfirmSmsSend officalpt账号确认短信发送响应
	 */
	void ProcessCreateGunionCheckSpecialConfirmSmsSendLoginResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：Gunion-SpecialSmsLogin officalpt账号短信登录响应
	 */
	void ProcessCreateGunionCheckSpecialSmsLoginResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：Gunion-PwdLogin 密码登录响应
	 */
	void ProcessCreateGunionCheckPwdLoginResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：Gunion-PicPwdLogin 图形验证码密码登录响应
	 */
	void ProcessCreateGunionCheckPicPwdLoginResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：Gunion-ThirdLogin 第三方登录响应
	 */
	void ProcessCreateGunionCheckThirdLoginResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：Gunion-SpecialPwdLogin officalpt账号密码登录响应
	 */
	void ProcessCreateGunionCheckSpecialPwdLoginResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：Gunion-SpecialPicPwdLogin officalpt账号图形密码登录响应
	 */
	void ProcessCreateGunionCheckSpecialPicPwdLoginResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：Gunion-AutoLogin 自动登录响应
	 */
	void ProcessCreateGunionCheckAutoLoginResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：Gunion-Logout 登出响应
	 */
	void ProcessCreateGunionCheckLogoutResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：Gunion-RealName 实名认证响应
	 */
	void ProcessCreateGunionCheckRealNameResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：Gunion-SetLoginArea 设置登录区服响应
	 */
	void ProcessCreateGunionCheckSetLoginAreaLoginResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：Gunion-GetLoginArea 获取区服列表响应
	 */
	void ProcessCreateGunionCheckGetLoginAreaLoginResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：Gunion-PayEntrance 创建统一收银台支付订单
	 */
	void ProcessCreateGunionPayEntranceResponse(int reqeustCode, map<string, string>& keyValues);

	/**
	 * @brief 处理：Gunion-PayEntrance 查询统一收银台支付订单支付结果
	 */
	void ProcessCreateGunionCheckPayOrderStatusResponse(int reqeustCode, map<string, string>& keyValues);

	/**
	 * @brief 处理：Gunion-Activation 激活码校验响应
	 */
	void ProcessCreateGunionCheckActivationResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：Gunion-SetTutorInfo 监护人信息设置响应
	 */
	void ProcessCreateGunionCheckSetTutorInfoResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：Gunion-WegameLogin Wegame账号绑定登录响应
	 */
	void ProcessCreateGunionCheckGunionWegameLoginResponse(
		int reqeustCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：GunionLogin 获取 RSA 公钥响应（握手第一步）
	 *
	 * 典型业务：
	 *   - 解析服务端返回的 RSA 公钥
	 *   - 构造完整 PEM 格式的公钥字符串
	 *   - 为后续 Handshake（会话密钥协商）阶段做准备
	 *
	 * 注意：
	 *   - 本接口返回数据为明文
	 *   - 不涉及 randKey / token / 3DES 解密
	 */
	void ProcessCreateGunionLoginGetPublicKeyResponse(
		int requestCode,
		map<string, string>& keyValues);

	/**
	 * @brief 处理：GunionLogin 握手响应（会话密钥协商 / 建立会话）
	 *
	 * 典型业务：
	 *   - 使用本地生成的 randKey 对服务端返回数据进行 3DES 解密
	 *   - 解析解密后的 JSON 数据
	 *   - 获取并保存会话 token
	 *
	 * 协议说明：
	 *   - 对应接口：/v1/basic/handshake
	 *   - 返回的 data 字段为 3DES 加密的 Base64 字符串
	 *
	 * 注意：
	 *   - 只有在 GetPublicKey 成功后才能调用
	 *   - Handshake 成功后，Gunion 接口进入加密通信阶段
	 */
	void ProcessCreateGunionLoginHandshakeResponse(
		int requestCode,
		map<string, string>& keyValues);
private:
	/**
	 * @brief HTTP 请求线程对象（旧版依赖 WinInet）
	 *
	 * 说明：
	 *   - LoginClient 的所有 HTTP 请求都不是主线程执行，而是通过 HttpThread
	 *   - HttpThread 负责：排队 → 发送 HTTP → 回调 LoginClient
	 *   - 旧逻辑使用 WinInet，Curl 替换后将被 CurlHttpThread 覆盖使用
	 *
	 * 注意：
	 *   - 这里保留 HttpThread* 是为了兼容历史流程（某些业务流程仍会走旧逻辑）
	 *   - 新增 CurlHttpThread 后，两者共存，只使用其中一个
	 */
	 //HttpThread* m_httpThread;

	 /**
	  * @brief HTTP 请求线程对象（新版基于 libcurl + OpenSSL）
	  *
	  * 功能说明：
	  *   - 完全替代旧版 WinInet 的 HttpThread
	  *   - 使用 libcurl（支持 HTTPS / TLS1.0~1.3，不受 Windows TLS 设置影响）
	  *   - 支持超时、中断（Cancel）、多域名备援（host1/host2/host3/host4/host5/host6）
	  *   - send → recv 整套回调与 HttpThread 完全对齐，因此可无缝切换
	  *
	  * 设计目的：
	  *   - 逐步弃用 WinInet，统一迁移到 curl 体系
	  *   - 在不改动 LoginClient 其他业务逻辑的前提下，可直接替换 HTTP 底层
	  *
	  * 使用方式：
	  *   - 与旧 m_httpThread 结构一致（ProcessRequest / GetState / Cancel）
	  *   - LoginClient.cpp 中仅需替换 new HttpThread(this)
	  *     → new CurlHttpThread(this)
	  *
	  * 兼容性：
	  *   - 两者可以共存（灰度发布 / 分业务迁移）
	  *   - 若 m_curlHttpThread 不为空 → 优先使用 curl 线程
	  *   - 若为空 → 回退到旧 HttpThread（WinInet）
	  */
	CurlHttpThread* m_httpThread;

	/**
	 * @brief 请求处理器（负责生成 HttpRequest + 解析参数）
	 *
	 * 功能：
	 *   - 构建 HTTP 请求：URL、POST 表单、Header、Cookies
	 *   - 将业务参数（如 userId / password / appId 等）组装到请求中
	 *   - 标记请求的 requestCode，用来区分回调处理函数
	 *
	 * 工作流程：
	 *   LoginClient::XXXLogin()
	 *        ↓
	 *   m_requestProcess.BuildXXXRequest()
	 *        ↓
	 *   生成 HttpRequest（包含 URL、参数、业务 requestCode）
	 *        ↓
	 *   m_httpThread->ProcessRequest(request)
	 *
	 * 与 m_mapResponseFunc 配合，通过 requestCode 分发 Response 处理。
	 */
	RequestProcess m_requestProcess;

	/**
	 * @brief 获取动态 Key 的回调
	 *
	 * 业务说明：
	 *   - 登录流程中必须先通过 publicKey + 私钥计算动态 key
	 *   - 动态 key 用于后续密码加密、短信验证、扫码登录等
	 *   - 生成动态 key 的过程由服务端控制，客户端只负责请求
	 */
	SdoBase_GetDynamicKeyCallback m_getDynamicKeyCallback;

	/**
	 * @brief 图形验证码 / 一键登录验证码登录回调
	 *
	 * 业务说明：
	 *   - 用户输入验证码（文字验证/图形验证/极验）后触发
	 *   - 登录器必须在 UI 上展示结果（成功 → 登录；失败 → 提示重试）
	 */
	SdoBase_CheckCodeLoginCallback m_checkCodeLoginCallback;

	/**
	 * @brief 动态密码（如安全卡、令牌、短信动态码）登录回调
	 *
	 * 业务说明：
	 *   - 联动安全中心的动密验证流程
	 *   - 验证成功后返回登录票据、用户 ID 等
	 */
	SdoBase_DynamicLoginCallback m_dynamicLoginCallback;

	/**
	 * @brief 实名认证登录回调
	 *
	 * 业务说明：
	 *   - 输入真实姓名 + 身份证号的实名登录流程
	 *   - 用于新用户实名验证、未成年限制逻辑
	 */
	SdoBase_FcmLoginCallback m_fcmLoginCallback;

	/**
	 * @brief 静密登录 / 一键登录 / 扫码登录的最终回调（最常用）
	 *
	 * 业务说明：
	 *   - 所有登录流程的最终结果都通过该回调统一输出
	 *   - 返回 ticket、userId、绑定信息、登录态等
	 */
	SdoBase_LoginResultCallback m_loginResultCallback;

	/**
	 * @brief 检查账号类型回调（手机号？邮箱？老账号？）
	 *
	 * 业务说明：
	 *   - 用于判断输入的 ID 是手机号 / 邮箱 / SDO账号
	 *   - 登录器 UI 根据账号类型动态展示不同登录方式
	 */
	SdoBase_CheckAccounTypeCallback m_checkAccountCallback;

	/**
	 * @brief 发送短信验证码回调
	 *
	 * 业务说明：
	 *   - 用于一键登录 / 下行短信登录 / 找回密码等场景
	 *   - 返回短信发送结果、冷却时间、图验等必要参数
	 */
	SdoBase_SendPhoneCheckCodeCallback m_phoneCheckCodeCallback;

	/**
	 * @brief 获取二维码 Key 回调（扫码登录）
	 *
	 * 业务说明：
	 *   - PC 端获取二维码 key 后，将 URL + 参数组合为二维码
	 *   - 手机扫码后触发调用 → 再轮询状态
	 */
	SdoBase_GetQrCodeCallback m_getCodeKeyCallback;

	/**
	 * @brief 获取登录状态回调
	 *
	 * 业务说明：
	 *   - 用于扫码登录轮询
	 *   - 用于一键登录轮询
	 *   - 返回是否已确认、是否成功、是否过期
	 */
	SdoBase_GetLoginStateCallback m_loginStatusCallback;

	/**
	 * @brief 延伸登录状态回调（ExtendLogin）
	 *
	 * 业务说明：
	 *   - 某些产品需要第二次验证，如“是否绑定手机”、“是否需要补充实名”
	 *   - 服务端返回需要补充的额外步骤
	 */
	SdoBase_ExtendLoginStateCallback m_extendLoginStateCallback;

	/**
	 * @brief 注销（Logout）回调
	 *
	 * 业务说明：
	 *   - 用户主动登出时触发
	 *   - 服务端同步清除 ticket / session
	 */
	SdoBase_LogoutCallback m_logoutCallback;

	/**
	 * @brief 发送 PushMessage（推送一键登录）回调
	 *
	 * 业务说明：
	 *   - 输入手机号 → 服务端 push 登录确认弹框到手机 App
	 *   - PC 端显示“等待手机确认”界面
	 */
	SdoBase_SendPushMessageCallback m_sendPushMessageCallback;

	/**
	 * @brief 获取 PushMessage 登录状态的回调
	 *
	 * 业务说明：
	 *   - 用于 push 登录的轮询
	 *   - 验证用户是否点击了“确认/取消/拒绝”
	 */
	SdoBase_GetPushMessageStatusCallback m_getPushMessageStatusCallback;

	/**
	 * @brief 获取账号信息（昵称、手机号、绑定状态）回调
	 *
	 * 业务说明：
	 *   - 登录后获取用户资料（如用户中心展示）
	 */
	SdoBase_GetAccountInfoCallback m_getAccountInfoCallback;

	/**
	 * @brief 获取用户登录历史回调
	 *
	 * 业务说明：
	 *   - 查看用户历史登录记录
	 *   - 用于部分产品的安全提示模块
	 */
	SdoBase_GetLoginHistoryCallback m_getLoginHistoryCallback;

	/**
	 * @brief 取消 PushMessage 登录回调（用于中断等待）
	 */
	SdoBase_CancelPushMessageCallback m_cancelPushMessageCallback;

	/**
	 * @brief 获取 Session 状态回调
	 *
	 * 业务说明：
	 *   - 判断 session 是否有效、是否过期
	 */
	SdoBase_GetSessionIdStatesCallBack m_getSessionIdStatesCallBack;

	/**
	 * @brief 异地登录踢人前的验证回调
	 *
	 * 业务说明：
	 *   - 判断账号是否需要短信验证
	 *   - 给 UI 反馈需要输入验证码还是直接踢下线
	 */
	SdoBase_KickoffAccountVerifyCallback m_callbackKickoffAccountVerify;

	/**
	 * @brief 异地登录踢人结果回调
	 *
	 * 业务说明：
	 *   - 用户确认踢人后，返回踢人结果
	 */
	SdoBase_KickoffAccountResultCallback m_callbackKickoffAccountResult;

	/**
	 * @brief 获取用户扩展信息回调
	 *
	 * 业务说明：
	 *   - 包含头像、性别、地区等扩展字段，供 UI 展示
	 */
	SdoBase_GetLoginUserInfoCallback m_getLoginUserInfoCallback;

	/**
	 * @brief 设置用户扩展信息回调（设置昵称、备注等）
	 */
	SdoBase_SetLoginUserInfoCallback m_setLoginUserInfoCallback;

	/**
	 * @brief 获取登录大区信息（跨区登录、游戏分服）
	 */
	SdoBase_GetLoginAreaInfoCallback m_getLoginAreaInfoCallback;

	/**
	 * @brief 用户隐私协议回调（旧版）
	 *
	 * 业务说明：
	 *   - 同意隐私政策的确认结果
	 */
	SdoBase_UserPrivacyConfigCallback m_UserPrivacyConfigCallback;

	/**
	 * @brief 人脸识别初始化回调
	 */
	SdoBase_FaceVerifyInitCallback m_FaceVerifyInitCallback;

	/**
	 * @brief 获取人脸识别结果回调
	 */
	SdoBase_GetFaceCodeResultCallback m_GetFaceCodeResultCallback;

	/**
	 * @brief 人脸识别动作指令回调（眨眼/摇头等）
	 */
	SdoBase_SendActionCallback m_SendActionCallback;

	/**
	 * @brief 获取 Ticket 回调（不同于登录返回的 Ticket）
	 *
	 * 业务说明：
	 *   - 某些游戏如《庆余年》需要独立获取二次票据
	 */
	SdoBase_GetTicketCallback m_GetTicketCallBack;

	/**
	 * @brief 创建 WeGame 订单的回调（用于充值/购买服务）
	 */
	SdoBase_CreateWeGameOrderCallback m_CreateWeGameOrderCallback;

	/**
	 * @brief 新版隐私协议回调
	 *
	 * 业务说明：
	 *   - 对应 NewVersionUserPrivacyConfig()
	 *   - 新协议版本（与旧版 UserPrivacyConfig 区分）
	 *   - 用户是否同意新版隐私协议
	 */
	SdoBase_NewVersionUserPrivacyConfigCallback m_NewVersionUserPrivacyConfigCallback;

	/**
	 * @brief 授权码回调（返回 authCode 对应的 ticket）
	 *
	 * 业务说明：
	 *   - 新版授权码登录流程
	 *   - 用户通过手机/网页输入授权码 → PC 调用检查接口
	 *   - 回调返回 ticket，用于后续登录流程
	 */
	SdoBase_CreateAuthCodeTicketCallback m_CreateAuchCodeCallback;

	/**
	 * @brief 三方登录灵活展示界面配置回调
	 *
	 * 业务说明：
	 *   - 某些游戏需要根据渠道（如微信/QQ/微博）动态展示不同登录皮肤
	 *   - 服务端返回应该显示哪些登录按钮、图标、顺序
	 */
	SdoBase_CreateThirdLoginSkinCallback m_CreateThirdLoginSkinCallback;

	///////////////////////////////////////////////////////////////////////////////
	// 传奇4（Mir4）相关回调
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 传奇4：创建订单回调
	 *
	 * 业务说明：
	 *   - 用于充值/购买商品
	 *   - 回调返回创建订单的结果（订单ID、状态等）
	 */
	SdoBase_CreateCQ4OrderCallback m_CreateCQ4OrderCallback;

	/**
	 * @brief 传奇4：查询订单信息回调
	 *
	 * 业务说明：
	 *   - 查询订单状态（支付成功/失败/超时）
	 */
	SdoBase_CreateCQ4QueryCallback m_CreateCQ4QueryCallback;

	///////////////////////////////////////////////////////////////////////////////
	// Steam / QQGame / LX 渠道相关回调
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief Steam 手游模式支付回调（SteamChannel）
	 *
	 * 业务说明：
	 *   - Steam手游充值流程
	 *   - PC 客户端需要监听订单结果
	 */
	SdoBase_CreateSteamChannelOrderCallback m_CreateSteamChannelOrderCallback;

	/**
	 * @brief QQGame 订单回调
	 *
	 * 业务说明：
	 *   - QQGame渠道充值订单创建回调
	 *   - 返回订单状态、签名验证结果等
	 */
	SdoBase_CreateQQGameOrderCallback m_CreateQQGameOrderCallback;

	/**
	 * @brief WeGame Status 回调（IsLogin、状态检测）
	 *
	 * 业务说明：
	 *   - 用于判定 WeGame 客户端是否登录 / 是否授权
	 */
	SdoBase_WeGameStatusCallback m_WeGameStatusCallback;

	/**
	 * @brief QQGame 是否已登录回调
	 *
	 * 业务说明：
	 *   - PC Launcher 接入 QQGame Sdk，需要确认登录态
	 */
	SdoBase_QQGameIsLoginCallback m_QQGameIsLoginCallback;

	/**
	 * @brief 乐享（LX）渠道订单创建回调
	 *
	 * 业务说明：
	 *   - LX 渠道充值使用
	 */
	SdoBase_CreateLxOrderCallback m_CreateLxOrderCallback;

	///////////////////////////////////////////////////////////////////////////////
	// 卫士通 Ue 安全加解密
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 卫士通 SDK 初始化回调
	 *
	 * 业务说明：
	 *   - 初始化后才可以进行加密签名等操作
	 */
	SdoBase_UeInitClientCallback m_UeInitClientCallback;

	///////////////////////////////////////////////////////////////////////////////
	// 请求分发与通用映射
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief response → 对应处理函数映射表
	 *
	 * 业务说明：
	 *   - key = requestCode
	 *   - value = 对应的处理成员函数指针（ProcessXXXResponse）
	 *   - 用于统一分发服务端返回值
	 */
	typedef void (LoginClient::* ResponseProcessFunc)(int reqeustCode, map<string, string>& keyValues);
	map<int, ResponseProcessFunc> m_mapResponseFunc;

	/**
	 * @brief 动态注册的回调函数映射（字符串→函数指针）
	 *
	 * 业务说明：
	 *   - SetCallBack() 动态设置的回调都会放这里
	 *   - 比如 JS / Lua 层动态注册
	 */
	map<string, void*> m_mapCallback;

	/**
	 * @brief 业务层临时保存的 KV 字段（字符串 → 字符串）
	 *
	 * 业务说明：
	 *   - 保存一些参数，用于后续接口使用
	 *   - 如 SetValue/GetValue
	 */
	map<string, string> m_mapValList;

	/**
	 * @brief 请求参数存储（Key→Value）
	 *
	 * 业务说明：
	 *   - 存储当前请求依赖的参数
	 *   - 例如：登录需要的某些字段
	 */
	map<string, string> m_mapReqParams;

	/**
	 * @brief 响应参数存储（Key→Value）
	 *
	 * 业务说明：
	 *   - 服务端返回的部分字段需要保存（如 loginType）
	 *   - 便于后续业务查询
	 */
	map<string, string> m_mapRespParams;

	/**
	 * @brief 当前保存的 publicKey（动态 Key 使用）
	 *
	 * 业务说明：
	 *   - GetPublicKey 请求成功后自动存储
	 *   - 后续 GetDynamicKey 会立即使用
	 */
	string m_publicKey;

	/**
	 * @brief 当前用户 GUID（唯一会话标识）
	 *
	 * 业务说明：
	 *   - 初始化调用 CreateGUID 生成
	 *   - 用于服务端追踪请求链路
	 */
	string m_guid;

	/**
	 * @brief 同步等待事件（WaitResponse 用）
	 *
	 * 业务说明：
	 *   - 接口需要阻塞等待服务器响应
	 *   - m_waitEvent 用于 LoginClient::WaitResponse(timeout) 中的阻塞触发
	 *   - 当 HTTP 返回时，在 ProcessResponse 内部会 SetEvent 唤醒等待线程
	 */
	HANDLE m_waitEvent;

	/**
	 * @brief 是否处于“下行短信验证码登录”流程中
	 *
	 * 业务说明：
	 *   - PushMessage 登录（手机号免密登录）时会有两种方式：
	 *       * 直接推送免密登录
	 *       * 推送后要求输入短信验证码
	 *   - 用于业务流控制（需要输入验证码的场景）
	 */
	bool m_isPushMessageCheckCode;

	/**
	 * @brief 下行短信登录的 sessionKey（服务端下发）
	 *
	 * 业务说明：
	 *   - 推送登录过程中服务端可能返回一个 sessionKey
	 *   - 该 key 用于后续验证码校验，如 SendPushMessage、PushMessageLogin 等接口使用
	 */
	string m_pushMsgSessionKey;

	///////////////////////////////////////////////////////////////////////////////
	// 下行短信 / 登录器配置 相关业务
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 获取登录器配置回调（功能开关 / 广告开关 / UI 配置等）
	 */
	SdoBase_GetClientConfigCallback m_GetClientConfigCallback;

	/**
	 * @brief 下行短信：初始化流程回调（如手机号、是否语音、flowId）
	 */
	SdoBase_GoDownConfigInitCallback m_GoDownConfigInitCallback;

	/**
	 * @brief 下行短信：图形验证码（Geetest）相关回调
	 */
	SdoBase_GoDownConfigSendSmsCheckCodeCallback m_GoDownSendSmsCheckCodeCallback;

	/**
	 * @brief 下行短信：确认发送验证码回调
	 */
	SdoBase_GoDownconfirmSendSmsCallback m_GoDownconfirmSendSmsCallback;

	/**
	 * @brief 下行短信：确认登录（短信验证码验证）回调
	 */
	SdoBase_GoDownconfirmLoginCallback m_GoDownconfirmLoginCallback;

	/**
	 * @brief 快速登录（FastLogin）回调
	 *
	 * 业务说明：
	 *   - 用于免密自动登录
	 *   - 端上会记录 KEY，用于下一次快速登录
	 */
	SdoBase_FastLoginLoginCallback m_FastLoginLoginCallback;

	/**
	 * @brief 下行短信初始化返回的 flowId
	 *
	 * 业务说明：
	 *   - 服务端返回的状态 ID（flowId）
	 *   - 后续所有短信登录步骤都必须依赖此 ID
	 */
	std::string sms_flowId;

	/**
	 * @brief 监护人信息补填流程的 flowId
	 *
	 * 业务说明：
	 *   - 用户需要补填监护人姓名/身份证/号码时使用
	 */
	std::string guard_dian_flowId;

	/**
	 * @brief 新版 GetTicket（取票据）流程的回调
	 */
	SdoBase_NewVersionGetTicketCallback m_GetTicketLoginCallback;

	/**
	 * @brief 其他登录场景：用户未绑定安全手机时触发的回调
	 *
	 * 业务说明：
	 *   - 例如扫描登录 / 第三方登录后验证发现未绑定安全手机
	 */
	SdoBase_OtherLoginSecurityPhoneCallback m_BindSecurityPhoneLoginCallback;

	/**
	 * @brief 快速登录主动删除账号（解绑、注销账号列表）回调
	 */
	SdoBase_FastLoginActiveDeleteAccountLoginCallback m_FastLoginActiveDeleteAccountLoginCallback;

	/**
	 * @brief 获取账号登录类型（如普通账号、游客账号、快速登录账号）回调
	 */
	SdoBase_CheckAccountTypeLoginCallback m_CheckAccountTypeLoginCallback;

	///////////////////////////////////////////////////////////////////////////////
	// 星座运势 / 广告系统 / 监护人系统
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 星座运势回调（CheckCalculate 用）
	 *
	 * 业务说明：
	 *   - 某些活动页面会显示星座占卜
	 *   - 这是用于获取服务端占卜结果
	 */
	SdoBase_CalculateCallback m_CalculateCallback;

	/**
	 * @brief 广告系统初始化回调
	 *
	 * 业务说明：
	 *   - 负责登录器启动页、背景图、弹窗广告的配置
	 */
	SdoBase_InitAdvCallback m_InitAdvCallback;

	/**
	 * @brief 监护人短信发送回调
	 *
	 * 业务说明：
	 *   - 未成年监护系统需要监护人短信校验
	 */
	SdoBase_GuardDianSendSmsCallback m_GuardDianSendSmsCallback;

	/**
	 * @brief 监护人图形验证码+短信验证码发送回调
	 */
	SdoBase_GuardDianSendSmsCheckCodeCallback m_GuardDianSendSmsCheckCodeCallback;

	/**
	 * @brief 监护人确认短信（最终验证）回调
	 */
	SdoBase_GuardDianConfrimSendSmsResultCallback m_GuardDianConfrimSendSmsResultCallback;

	/**
	 * @brief 监护人信息最终回调（审核是否通过）
	 */
	SdoBaseGuardDianCallback m_GuardDianCallback;

	///////////////////////////////////////////////////////////////////////////////
	// 人脸失败/信息收集
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief 人脸打开的Url
	 */
	SdoBaseSetFaceRecognitionUrlCallback m_FaceRecognitionUrlCallback;

	/**
	 * @brief 查询人脸识别票据状态
	 */
	SdoBaseSetQueryFaceVerifyTicketStatusCallback m_QueryFaceVerifyTicketStatusCallback;

	/**
	 * @brief 收集人身核验信息url
	 */
	SdoBaseSetFaceHolderRegistrationUrlCallback m_HolderRegistrationUrlCallback;

	/**
	 * @brief 激活码登录回调
	 */
	SdoBaseActiveCodeLoginCallback m_ActiveLoginCallback;

	/**
	 * @brief 展示激活码界面回调
	 */
	SdoBaseShowActiveCodeLoginDlgCallback m_ShowActiveLoginDlgCallback;

	///////////////////////////////////////////////////////////////////////////////
	// GunionWegame 回调成员
	///////////////////////////////////////////////////////////////////////////////

	/**
	 * @brief GunionWegame 初始化回调
	 *
	 * 初始化完成后触发，返回：
	 *   - 各三方平台 AppId / Key（微信 / QQ / 微博）
	 *   - 品牌信息（游戏名、Logo、品牌名）
	 *   - 登录文案、语音提示
	 *   - token / randKey 等初始化参数
	 */
	SdoBase_CreateGunionWegameInitCallback
		m_CreateGunionWegameInitCallback;

	/**
	 * @brief GunionWegame 登录结果回调
	 *
	 * 登录完成后触发，返回：
	 *   - 登录结果码
	 *   - 用户 ID
	 *   - 登录票据
	 *   - 是否新用户
	 *   - 是否需要绑定手机及相关策略
	 */
	SdoBase_CreateGunionWegameLoginCallback
		m_CreateGunionWegameLoginCallback;

	/**
	 * @brief GunionWegame 短信发送回调
	 *
	 * 用于普通短信 / 语音短信发送结果通知
	 */
	SdoBase_CreateGunionWegameSmsSendCallback
		m_CreateGunionWegameSmsSendCallback;

	/**
	 * @brief GunionWegame 带图形验证码短信发送回调
	 *
	 * 回调中包含：
	 *   - 图形验证码 URL
	 *   - 是否为极验
	 *   - 验证码尺寸信息
	 */
	SdoBase_CreateGunionWegamePicSmsSendCallback
		m_CreateGunionWegamePicSmsSendCallback;

	/**
	 * @brief GunionWegame 三方账号绑定手机号回调
	 *
	 * 回调返回：
	 *   - 绑定手机号
	 *   - 盛趣账号 ID
	 *   - 三方昵称
	 *   - 实名流程 ID
	 */
	SdoBase_CreateGunionWegameThirdAccountBindPhoneCallback
		m_CreateGunionWegameThirdAccountBindPhoneCallback;

	/**
	 * @brief GunionWegame 实名补填回调
	 *
	 * 用于三方登录后补填实名信息的结果通知
	 */
	SdoBase_CreateGunionWegameFillRealinfoCallback
		m_CreateGunionWegameFillRealinfoCallback;

	/**
	 * @brief GunionWegame 获取登录票据回调
	 *
	 * 返回最新登录票据，用于后续业务调用
	 */
	SdoBase_CreateGunionWegameGetTicketCallback
		m_CreateGunionWegameGetTicketCallback;

	/**
	 * @brief GunionWegame 获取支付票据回调
	 *
	 * 返回支付专用票据，用于支付流程
	 */
	SdoBase_CreateGunionWegameGetPayTicketCallback
		m_CreateGunionWegameGetPayTicketCallback;
	/**
	==========================================================
		GHOME_PC 回调函数指针成员变量
	==========================================================
	 */

	 ///////////////////////////////////////////////////////////////////////////////
	 // 隐私协议相关回调
	 ///////////////////////////////////////////////////////////////////////////////

	SdoBase_CreateDoPrivateCallback m_CreateDoPirvateCallback;
	///< 隐私协议跳转回调（返回隐私URL）

	///////////////////////////////////////////////////////////////////////////////
	// 短信验证码体系
	///////////////////////////////////////////////////////////////////////////////

	SdoBase_CreateDoSmsSendCallback m_CreateDoSmsSendCallback;
	///< 短信发送回调

	SdoBase_CreateCheckPicSmsSendCallback m_CreateCheckPicSmsSendCallback;
	///< 带图形验证码短信发送回调

	SdoBase_CreateCheckSmsLoginCallback m_CreateCheckSmsLoginCallback;
	///< 短信验证码登录回调

	///////////////////////////////////////////////////////////////////////////////
	// 实名认证 / 自动登录 / 登出 / 票据
	///////////////////////////////////////////////////////////////////////////////

	SdoBase_CreateCheckRealNameCallback m_CreateCheckRealNameCallback;
	///< 实名认证回调（返回年龄、是否成年、是否需要监护人）

	SdoBase_CreateCheckAutoLoginCallback m_CreateCheckAutoLoginCallback;
	///< 自动登录回调

	SdoBase_CreateCheckLogoutCallback m_CreateCheckLogoutCallback;
	///< 登出回调

	///////////////////////////////////////////////////////////////////////////////
	// 普通账号密码登录体系
	///////////////////////////////////////////////////////////////////////////////

	SdoBase_CreateDoPwdLoginCallback m_CreateCheckPwdLoginCallback;
	///< 密码登录回调

	SdoBase_CreateCheckPicPwdLoginCallback m_CreateCheckPicPwdLoginCallback;
	///< 图验密码登录回调

	///////////////////////////////////////////////////////////////////////////////
	// 第三方登录体系
	///////////////////////////////////////////////////////////////////////////////

	SdoBase_CreateCheckThirdLoginCallback m_CreateCheckThirdLoginCallback;
	///< 第三方登录回调（QQ / 微信 / 微博 / Wegame）

	///////////////////////////////////////////////////////////////////////////////
	// 个性账号登录体系
	///////////////////////////////////////////////////////////////////////////////

	SdoBase_CreateDoSpecialPwdLoginCallback m_CreateCheckSpecialPwdLoginCallback;
	///< 个性账号静密登录回调

	SdoBase_CreateCheckSpecialPicPwdLoginCallback m_CreateCheckSpecialPicPwdLoginCallback;
	///< 个性账号带图验静密登录回调

	SdoBase_CreateDoSpecialSmsSendCallback m_CreateDoSpecialSmsSendCallback;
	///< 个性账号短信发送回调

	SdoBase_CreateCheckSpecialPicSmsSendCallback m_CreateCheckSpecialPicSmsSendCallback;
	///< 个性账号带图验短信发送回调

	SdoBase_CreateDoSpecialConfirmSmsSendCallback m_CreateDoSpecialConfirmSmsSendCallback;
	///< 个性账号安全手机确认短信发送回调

	SdoBase_CreateDoSpecialCheckSmsLoginCallback m_CreateDoSpecialCheckSmsLoginCallback;
	///< 个性账号短信登录回调

	///////////////////////////////////////////////////////////////////////////////
	// 区服记录体系
	///////////////////////////////////////////////////////////////////////////////

	SdoBase_CreateCheckLoginAreaCallback m_CreateCheckSetLoginAreaLoginCallback;
	///< 登录区服记录回调

	SdoBase_CreateCheckGetLoginAreaCallback m_CreateCheckGetLoginAreaLoginCallback;
	///< 获取区服配置回调

	///////////////////////////////////////////////////////////////////////////////
	// 支付体系
	///////////////////////////////////////////////////////////////////////////////

	SdoBase_CreatePayEntranceCallback m_CreateDoPayEntranceCallback;
	///< 统一收银台支付回调

	SdoBase_CreateCheckPayOrderStatus m_CreateDoCheckPayOrderStatuseCallback;
	///< 支付订单状态查询回调

	SdoBase_CreateCheckActivation m_CreateDoCheckActivationCallback;
	///< 激活码校验回调

	///////////////////////////////////////////////////////////////////////////////
	// 协议与合规体系
	///////////////////////////////////////////////////////////////////////////////

	SdoBase_CreateCheckGameAgreementUrlContentCallback m_CreateCheckGameAgreementUrlContentCallback;
	///< 获取服务协议 / 隐私协议 HTML 内容回调

	SdoBase_CreateCheckSetTutorInfoCallback m_CreateCheckSetTutorInfoCallback;
	///< 监护人信息收集回调

	///////////////////////////////////////////////////////////////////////////////
	// Wegame 登录体系
	///////////////////////////////////////////////////////////////////////////////

	SdoBase_CreateGhomeGunionWegameLoginCallback m_CreateCheckGunionWegameLoginCallback;
	///< Wegame 登录结果回调

	///////////////////////////////////////////////////////////////////////////////
	// Gunion 获取公钥握手体系
	///////////////////////////////////////////////////////////////////////////////

	SdoBase_CreateGunionGetPublicKeyCallback m_CreateCheckGunionGetPublicKeyCallback;
	///< Gunion 获取公钥回调

	SdoBase_CreateGunionHandShakeCallback m_CreateCheckGunionHandShakeCallback;
	///< Gunion 握手回调

	/**
	==========================================================
		GHOME_PC 回调函数指针成员变量
	==========================================================
	 */

	 /**
	  * @brief 自动登录 Key（FastLogin 返回）
	  *
	  * 业务说明：
	  *   - 服务端若支持自动延续登录态，则不会每次返回 ticket
	  *   - 此 Key 允许端上保持登录状态（类似“refreshToken”）
	  */
	std::string auto_login_fast_key;

	/**
	 * @brief 控制当前 LoginClient 是否启用 HTTPS 证书校验
	 *
	 * 设计目的：
	 *   - SdoBase_DownloadImageA / CurlHttpClient 都依赖 verifyCert 来决定是否校验证书
	 *   - 默认值通常为 true（开启严格校验）
	 *   - 在 Debug / 抓包（如 Fiddler）环境需要关闭校验，否则接口会失败
	 *
	 * 使用说明：
	 *   - verifyCert = true  → 严格校验证书链（生产环境）
	 *   - verifyCert = false → 不校验证书，用于开发、测试、调试环境
	 *
	 * 生命周期：
	 *   - 与 LoginClient 实例绑定
	 *   - 可随时调用 setVerifyCert() 动态修改
	 */
	bool m_verifyLoginClientCert = true;

public:
	/**
	 * @brief Gunion 加密 / 签名上下文句柄
	 *
	 * 设计目的：
	 *   - 用于 Gunion-WeGame 相关接口的加密签名、token / randKey 维护
	 *   - 作为 Gunion 体系下所有 HTTP 请求的安全上下文对象
	 *   - 支撑 WeGame 初始化、登录、短信、实名、票据等完整流程
	 *
	 * 使用范围：
	 *   - Gunion-WeGame Init / Login / Sms / Ticket / PayTicket 等接口
	 *   - GuardDian（监护人信息补填）相关接口
	 *
	 * 生命周期：
	 *   - 随 LoginClient 实例创建并初始化
	 *   - 在 LoginClient 析构或 Release 时统一释放
	 *
	 * 注意事项：
	 *   - 不允许跨 LoginClient 实例复用
	 *   - 不建议外部直接操作其内部状态
	 */
	 //GunionBaseHandle* gunion_handle;

	 /**
	  * @brief Gunion-WeGame 初始化阶段下发的 new_device_id_server
	  *
	  * 业务说明：
	  *   - 该值由 Gunion-WeGame Init 接口返回
	  *   - 用于标识服务端感知的“设备唯一标识”
	  *   - 后续短信发送、实名、票据等接口需携带该参数
	  *
	  * 使用范围：
	  *   - Gunion-WeGame SmsSend / PicSmsSend
	  *   - Gunion-WeGame ThirdAccountBindPhone
	  *   - Gunion-WeGame FillRealinfo / GetTicket / GetPayTicket
	  *
	  * 生命周期：
	  *   - 在 Gunion-WeGame Init 成功后写入
	  *   - 在 LoginClient 生命周期内保持有效
	  *
	  * 注意事项：
	  *   - 未完成 Init 前，该值可能为空
	  *   - 不应由外部随意修改
	  */
	std::string new_device_id_server;

	/**
	 * @brief Gunion-WeGame 图形验证码短信发送的上下文标识（CodeGuid）
	 *
	 * 业务说明：
	 *   - 在 Gunion-WeGame 短信发送过程中，如果需要图形验证码，
	 *     服务端会返回一个 CodeGuid 用于标识当前验证码上下文
	 *   - 后续 PicSmsSend / 验证流程需携带该值
	 *
	 * 使用范围：
	 *   - Gunion-WeGame PicSmsSend 接口
	 *   - 图形验证码校验后的短信发送流程
	 *
	 * 生命周期：
	 *   - 在需要图形验证码的短信流程中生成
	 *   - 完成对应短信流程后可被覆盖或清空
	 *
	 * 注意事项：
	 *   - 该值仅在“图验短信流程”中有效
	 *   - 不应跨不同短信流程复用
	 */
	std::string gunion_wegame_check_sms_send_CodeGuid;

	/**
	 * @brief Gunion-WeGame 图形验证码短信发送上下文标识（手机号体系）
	 *
	 * 业务说明：
	 *   - 在手机号短信发送流程中，如果触发图形验证码，
	 *     服务端会返回一个 CodeGuid 用于标识当前图验上下文
	 *   - 后续 PicSmsSend 接口需携带该值
	 *
	 * 使用范围：
	 *   - Gunion-WeGame PicSmsSend（手机号图验短信发送）
	 *
	 * 生命周期：
	 *   - 在触发图形验证码时生成
	 *   - 完成当前短信流程后可被覆盖或清空
	 *
	 * 注意事项：
	 *   - 不可跨不同短信流程复用
	 *   - 不应与个性账号图验 Guid 混用
	 */
	std::string ghome_check_sms_send_CodeGuid;

	/**
	 * @brief Gunion-WeGame 静密登录图形验证码上下文标识
	 *
	 * 业务说明：
	 *   - 在密码登录流程中，如触发图形验证码，
	 *     服务端返回静密登录专属 CodeGuid
	 *   - 后续 PicPwdLogin 接口需携带该值
	 *
	 * 使用范围：
	 *   - Gunion-WeGame PicPwdLogin（手机号图验密码登录）
	 *
	 * 生命周期：
	 *   - 在触发静密登录图验时生成
	 *   - 当前登录流程结束后可被覆盖
	 *
	 * 注意事项：
	 *   - 仅用于“手机号密码登录”图验流程
	 *   - 不应与短信图验 Guid 混用
	 */
	std::string ghome_check_static_login_CodeGuid;

	/**
	 * @brief Gunion 个性账号图形验证码上下文标识
	 *
	 * 业务说明：
	 *   - 在个性账号登录流程中，如触发图形验证码，
	 *     服务端返回专属 CodeGuid
	 *   - 后续 SpecialPicPwdLogin 接口需携带该值
	 *
	 * 使用范围：
	 *   - Gunion SpecialPicPwdLogin
	 *
	 * 生命周期：
	 *   - 在触发个性账号图验时生成
	 *   - 当前登录流程结束后可被覆盖
	 *
	 * 注意事项：
	 *   - 不可与手机号图验 Guid 混用
	 *   - 与 outInfo 强绑定
	 */
	std::string ghome_check_special_check_code_guid;

	/**
	 * @brief Gunion 个性账号短信发送流程上下文标识
	 *
	 * 业务说明：
	 *   - 在个性账号短信发送流程中生成
	 *   - 用于标识当前短信发送事务
	 *   - 后续 ConfirmSmsSend / CheckSmsLogin 需携带该值
	 *
	 * 使用范围：
	 *   - SpecialSmsSend
	 *   - SpecialPicSmsSend
	 *   - SpecialConfirmSmsSend
	 *   - SpecialCheckSmsLogin
	 *
	 * 生命周期：
	 *   - 在短信发送阶段生成
	 *   - 当前短信登录流程结束后可清空
	 *
	 * 注意事项：
	 *   - 不可跨不同短信流程复用
	 *   - 与当前 account 强绑定
	 */
	std::string ghome_check_special_sms_send_guid;
public:
	/**
	 * @brief 设置 HTTPS 证书校验开关
	 *
	 * @param enable:
	 *      true  - 启用证书校验（推荐：正式环境）
	 *      false - 禁用证书校验（用于调试/抓包）
	 *
	 * 场景示例：
	 *   - 调试验证码接口、需要通过 Fiddler 抓包
	 *   - 内网测试环境使用 HTTPS 但证书不完整
	 */
	void SetVerifyCert(bool enable)
	{
		m_verifyLoginClientCert = enable;
	}

	/**
	 * @brief 获取当前证书校验开关状态
	 *
	 * 返回：
	 *     true  - 当前启用了证书校验
	 *     false - 当前关闭了证书校验（Debug/抓包模式）
	 */
	bool GetVerifyCert() const
	{
		return m_verifyLoginClientCert;
	}

	/**
	 * @brief 三方登录功能的“外部扩展标识”（静态变量）
	 *
	 * 业务说明：
	 *   - 某些第三方登录流程（例如：QQ、微信、微博、Lenovo、顺网登录等）
	 *     会根据登录入口来源或业务场景附加一个扩展标识。
	 *
	 *   - 该扩展值由业务层设置，LoginClient 仅做透传，不参与处理逻辑。
	 *
	 *   - static 的原因是：第三方登录扩展标识通常与整体会话/进程相关，
	 *     而不是与某个 LoginClient 实例绑定。
	 *
	 * 示例：
	 *   登录入口 A → thirdLoginExtern = 1
	 *   登录入口 B → thirdLoginExtern = 2
	 *   登录入口 C → thirdLoginExtern = 10086
	 *
	 *   服务端根据此 ID 决定展示不同 UI、风控级别、登录策略等。
	 */
	static int thirdLoginExtern;

	/**
	 * @brief 获取 thirdLoginExtern 的字符串形式（用于 URL 参数拼接）
	 *
	 * 业务说明：
	 *   - 网络请求通常要求传字符串，而 thirdLoginExtern 是 int
	 *   - 因此提供统一转换接口，避免业务层重复写 int → string 的逻辑
	 *   - 内部使用 ostringstream 保证兼容性（VS2008/VS2019 均适用）
	 *
	 * 返回：
	 *   - 返回 thirdLoginExtern 转为 string
	 *
	 * 用途：
	 *   - 如：ThirdLogin、ThirdForLogin、GetThirdLoginSkin 需要添加扩展参数
	 *   - 会作为 &extern=xxx 传给服务器
	 */
	static string getThirdLoginExtern()
	{
		ostringstream os;     // 构造字符串流
		os << thirdLoginExtern; // 将 int 写入流
		return os.str();        // 返回 string
	}

	/**
	 * @brief Gunion-WeGame 专用的三方登录扩展标识（字符串形式）
	 *
	 * 业务说明：
	 *   - Gunion-WeGame 登录流程中，部分接口需要额外携带第三方登录扩展参数
	 *   - 与通用的 thirdLoginExtern 不同，该值通常由 Gunion / WeGame 体系单独维护
	 *   - 该扩展标识直接以字符串形式参与 URL 或 POST 参数拼接
	 *
	 * 设计说明：
	 *   - 与 getThirdLoginExtern() 区分开，避免不同登录体系之间参数混用
	 *   - LoginClient 仅负责透传该参数，不参与具体业务判断
	 *
	 * 用途示例：
	 *   - GunionWeGameInit
	 *   - GunionWeGameLogin
	 *   - GunionWeGameGetTicket
	 *
	 * 服务器用途：
	 *   - 服务端可根据该扩展标识区分不同 WeGame 登录入口
	 *   - 决定对应的登录策略、风控规则或业务配置
	 */
	static string gunionWegameThirdLoginExtern;

	/**
	 * @brief 获取 Gunion-WeGame 三方登录扩展标识
	 *
	 * 业务说明：
	 *   - 返回 Gunion-WeGame 登录流程使用的扩展标识字符串
	 *   - 用于统一拼接到网络请求参数中
	 *
	 * 返回：
	 *   - Gunion-WeGame 三方登录扩展标识（string）
	 *
	 * 使用场景：
	 *   - 构造 Gunion-WeGame 相关 HTTP 请求时
	 *   - 作为 &extern=xxx 或等效参数传递给服务器
	 */
	static string getGunionWegameThirdLoginExtern()
	{
		return gunionWegameThirdLoginExtern;
	}

	/**
	 * @brief 三方登录的“买量 / 渠道归因标识”（静态变量）
	 *
	 * 业务说明：
	 *   - 用于标识当前登录会话对应的买量渠道或推广来源
	 *     （如：广告平台、渠道包、投放来源等）
	 *
	 *   - 该字段主要用于：
	 *     - 用户来源归因
	 *     - 买量效果统计
	 *     - 渠道投放分析
	 *
	 *   - 由业务层在进入登录流程前设置，
	 *     LoginClient 仅负责透传，不解析、不校验。
	 *
	 *   - 使用 std::string 的原因：
	 *     - 渠道 ID 通常为字符串（如："tencent_ads"、"douyin_001"、"oppo_store"）
	 *     - 便于后续扩展（避免 int 编码不够用）
	 *
	 *   - static 的原因：
	 *     - 买量渠道通常与整个启动 / 登录会话绑定
	 *     - 而非某个具体 LoginClient 实例
	 *
	 * 示例：
	 *   channelIdExtern = "tencent_ads"
	 *   channelIdExtern = "douyin_feed_01"
	 *   channelIdExtern = "huawei_store"
	 */
	static std::string channelIdExtern;

	/**
	 * @brief 获取买量 / 渠道归因标识（字符串形式）
	 *
	 * 业务说明：
	 *   - channelIdExtern 用于标识当前登录会话的买量渠道或推广来源
	 *   - 该值通常由业务层在进入登录流程前设置
	 *   - LoginClient 仅负责透传，不参与解析或校验
	 *
	 * 设计说明：
	 *   - 使用 std::string 存储，适配多种渠道标识格式
	 *     （如广告平台、渠道包、投放来源编码等）
	 *   - 提供统一 getter 接口，便于网络层或参数拼装层调用
	 *
	 * 返回值：
	 *   - 当前设置的买量渠道标识字符串
	 *   - 若未设置，可能为空字符串
	 *
	 * 使用场景：
	 *   - 登录 / 初始化接口中作为可选扩展参数透传
	 *   - 用于服务端进行用户来源归因、投放效果统计
	 *
	 * 示例：
	 *   channelIdExtern = "tencent_ads"      → 返回 "tencent_ads"
	 *   channelIdExtern = "douyin_feed_01"   → 返回 "douyin_feed_01"
	 */
	static std::string getChannelIdExtern()
	{
		return channelIdExtern;
	}
public:

	//【谁决定流程】         GunionHandshake
	//【谁发请求】           LoginClient
	//【谁构造 HttpRequest】 RequestProcess
	//【谁真正发网】         CurlHttpThread

	/**
	 * @brief 确保 Gunion-WeGame 握手流程已完成
	 *
	 * 行为说明：
	 *   - 若尚未开始握手：启动握手流程
	 *   - 若握手进行中：不重复启动
	 *   - 若握手已完成：返回最终结果
	 *
	 * @return
	 *   - true  ：握手已完成且成功
	 *   - false ：握手失败或尚未完成
	 */
	bool EnsureGunionReady();

	/**
	 * @brief 启动 Gunion 公钥获取流程
	 *
	 * @return true 请求已成功投递
	 */
	bool StartGunionGetPublicKey();

	/**
	 * @brief 启动 Gunion 握手流程（会话建立 / 会话密钥协商）
	 *
	 * 业务说明：
	 *   - 使用 GetPublicKey 阶段获取的 RSA 公钥
	 *   - 客户端生成 randKey，并使用 RSA 公钥加密
	 *   - 向 /v1/basic/handshake 接口发起请求
	 *
	 * @param publicKey
	 *   - 服务端下发的 RSA 公钥（PEM 格式）
	 *
	 * @return
	 *   - true  请求已成功投递
	 *   - false 请求构造或投递失败
	 */
	bool StartGunionHandshake(const std::string& publicKey);

public:
	/**
	 * @brief 使用当前 Gunion 会话密钥解密 3DES(Base64) 数据
	 *
	 * 业务背景：
	 *   - Gunion 服务端在多个接口中，会将 data 字段
	 *     使用 3DES(EDE3/ECB) 加密后再进行 Base64 编码返回
	 *   - 客户端需要使用“当前会话 randKey”进行解密
	 *
	 * 加密/解密约定：
	 *   - 算法：3DES / EDE3 / ECB
	 *   - IV   ：全 0（历史协议约定）
	 *   - Key  ：当前 Gunion 会话 randKey
	 *   - 输入：Base64 编码字符串
	 *   - 输出：JSON 明文字符串
	 *
	 * 架构说明：
	 *   - 本接口只做“解密动作”
	 *   - 不解析 JSON
	 *   - 不处理业务语义
	 *   - 由 ResponseProcess / LoginClient 上层决定如何使用解密结果
	 *
	 * 使用场景：
	 *   - Gunion Handshake 返回 data
	 *   - Wegame 登录 / 短信 / 绑手机 / 实名 等接口
	 *
	 * @param base64Cipher 服务端返回的 Base64 编码 3DES 密文
	 * @param outPlain    输出解密后的明文（通常为 JSON 字符串）
	 *
	 * @return true  解密成功
	 * @return false 解密失败（密文非法 / randKey 不存在 / OpenSSL 错误）
	 */
	bool DecryptGunion3Des(
		const std::string& base64Cipher,
		std::string& outPlain) const;

	// ============================================================
	// Gunion-WeGame 加密 / 签名能力（迁移自 CGAuthClient）
	// ============================================================

	//EnsureGunionReady
	//	└─ GunionHandshake::Start
	//	└─ StartGunionGetPublicKey
	//	└─ BuildGunionGetPublicKeyRequest
	//	└─ OnPublicKeyResponse
	//	└─ GenerateGunionRandKey
	//	└─ EncryptGunionRsaPub
	//	└─ StartGunionNegotiateKey
	/**
	 * @brief 生成 Gunion 会话使用的随机对称密钥
	 *
	 * 说明：
	 *   - 用于 NegotiateKey 阶段
	 *   - 后续所有 Gunion 接口使用该 randKey 进行 3DES 加密
	 */
	void GenerateGunionRandKey();

	/**
	 * @brief 使用 RSA 公钥加密数据
	 *
	 * @param publicKey    服务端返回的 RSA 公钥
	 * @param plainText   明文（通常为 randKey）
	 * @param encryptText 输出 Base64 编码后的密文
	 */
	bool EncryptGunionRsaPub(
		const std::string& publicKey,
		const std::string& plainText,
		std::string& encryptText);

	/**
	 * @brief 计算 Gunion 接口签名（MD5）
	 */
	std::string CalcGunionSignature(
		const std::map<std::string, std::string>& params);

	/**
	 * @brief UCS2 → ANSI（GBK）转换
	 */
	std::string ConvertUcs2ToAnsi(
		const std::string& ucs2);

	/**
	 * @brief 构建 Gunion-WeGame 初始化接口的 3DES 加密参数
	 *
	 * 说明：
	 *   - 迁移自 CGAuthClient::EncrytParams3DesCStyleDoInit
	 *   - 为 Gunion-WeGame 初始化接口（/v1/account/initialize）专属实现
	 *   - 按初始化接口协议规则拼接参数并进行 3DES(EDE3/ECB) 加密
	 *
	 * @param deviceIdServer     服务端设备标识（初始化接口使用，当前可为空）
	 * @param mac                客户端 MAC 地址
	 * @param windowsDeviceId    Windows 设备唯一标识
	 * @param randKey            当前 Gunion 会话使用的对称密钥
	 * @param outEncryptedParams 输出 Base64 编码后的加密参数
	 *
	 * @return
	 *   - true  加密成功
	 *   - false 参数构造或加密失败
	 */
	bool BuildGunionWegameInitEncryptedParams(
		const std::string& deviceIdServer,
		const std::string& mac,
		const std::string& windowsDeviceId,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief 计算 Gunion-WeGame 初始化接口请求签名（MD5）
	 *
	 * 说明：
	 *   - 迁移自 CGAuthClient::CalcSignatureCStypeDoInit
	 *   - 为 Gunion-WeGame 初始化接口（/v1/account/initialize）专属实现
	 *   - 按初始化接口协议规则对参数排序、拼接并计算 MD5 签名
	 *
	 * @param deviceIdServer  服务端设备标识（初始化接口使用，当前可为空）
	 * @param mac             客户端 MAC 地址
	 * @param windowsDeviceId Windows 设备唯一标识
	 * @param randKey         当前 Gunion 会话使用的对称密钥
	 * @param outSignature    输出 32 位大写 HEX 格式签名
	 *
	 * @return
	 *   - true  签名计算成功
	 *   - false 参数异常或计算失败
	 */
	bool CalcGunionWegameInitSignature(
		const std::string& deviceIdServer,
		const std::string& mac,
		const std::string& windowsDeviceId,
		const std::string& randKey,
		std::string& outSignature);

	/**
	 * @brief 构建 Gunion-WeGame 登录接口的 3DES 加密参数
	 *
	 * 说明：
	 *   - 迁移自 CGAuthClient::EncrytParams3DesCStyleWegameLogin
	 *   - 为 Gunion-WeGame 登录接口（/v1/cooperation/wegameLogin）专属实现
	 *   - 按登录接口协议规则拼接参数并进行 3DES(EDE3/ECB) 加密
	 *
	 * 协议参数说明：
	 *   - deviceid           客户端设备标识（本地生成）  /与deviceIdServer一个值
	 *   - device_id_server   服务端下发的新设备标识
	 *   - thirdToken         WeGame 会话票据（rail_session_ticket）
	 *   - thirdUserId        WeGame 用户 ID（rail_id）
	 *   - islimited          是否受限账号（"0" / "1"）
	 *   - companyid          公司 ID
	 *
	 * @param deviceId        客户端设备标识  /与deviceIdServer一个值
	 * @param deviceIdServer  服务端设备标识
	 * @param thirdToken      WeGame 会话票据
	 * @param thirdUserId     WeGame 用户 ID
	 * @param isLimited       是否受限账号（字符串形式）
	 * @param companyId       公司 ID（字符串形式）
	 * @param randKey         当前 Gunion 会话使用的对称密钥
	 * @param outEncryptedParams 输出 Base64 编码后的加密参数
	 *
	 * @return
	 *   - true  加密成功
	 *   - false 参数构造或加密失败
	 */
	bool BuildGunionWegameLoginEncryptedParams(
		const std::string& deviceId,
		const std::string& deviceIdServer,
		const std::string& thirdToken,
		const std::string& thirdUserId,
		const std::string& isLimited,
		const std::string& companyId,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief 计算 Gunion-WeGame 登录接口请求签名（MD5）
	 *
	 * 说明：
	 *   - 迁移自 CGAuthClient::CalcSignatureCStypeWegameLogin
	 *   - 为 Gunion-WeGame 登录接口（/v1/cooperation/wegameLogin）专属实现
	 *   - 按登录接口协议规则对参数排序、拼接并计算 MD5 签名
	 *
	 * 签名规则：
	 *   1. 将所有参与签名的参数按 key 排序
	 *   2. 使用 key=value 形式拼接，中间以 '&' 分隔
	 *   3. 拼接当前 Gunion 会话 randKey
	 *   4. 转为小写
	 *   5. 计算 MD5，输出 32 位大写 HEX 字符串
	 *
	 * @param deviceId        客户端设备标识  /与deviceIdServer一个值
	 * @param deviceIdServer  服务端设备标识
	 * @param thirdToken      WeGame 会话票据
	 * @param thirdUserId     WeGame 用户 ID
	 * @param isLimited       是否受限账号（字符串形式）
	 * @param companyId       公司 ID（字符串形式）
	 * @param randKey         当前 Gunion 会话使用的对称密钥
	 * @param outSignature    输出 32 位大写 HEX 格式签名
	 *
	 * @return
	 *   - true  签名计算成功
	 *   - false 参数异常或计算失败
	 */
	bool CalcGunionWegameLoginSignature(
		const std::string& deviceId,
		const std::string& deviceIdServer,
		const std::string& thirdToken,
		const std::string& thirdUserId,
		const std::string& isLimited,
		const std::string& companyId,
		const std::string& randKey,
		std::string& outSignature);

	/**
	 * @brief 构建 Gunion-WeGame 短信发送接口的 3DES 加密参数
	 *
	 * 说明：
	 *   - 迁移自 CGAuthClient::EncrytParams3DesCStyleWegameSmsSend
	 *   - 为 Gunion-WeGame 短信发送接口（/v1/basic/smssend）专属实现
	 *   - 按短信发送接口协议规则拼接参数并进行 3DES(EDE3/ECB) 加密
	 *
	 * 协议参数说明：
	 *   - phone               手机号码
	 *   - type                短信类型（如："5"，由旧协议约定）
	 *   - supportPic          是否支持图片验证码（"0" / "1"）
	 *   - voiceMsg            语音提示文案（允许为空）
	 *   - sms_new             是否使用新短信流程（"0" / "1"）
	 *   - deviceid            客户端设备标识（本地生成）
	 *   - device_id_server    服务端下发的新设备标识
	 *
	 * @param phone             手机号码
	 * @param type              短信类型（字符串形式，按协议固定值）
	 * @param supportPic        是否支持图片验证码（字符串形式）
	 * @param voiceMsg          语音提示文案
	 * @param smsNew            是否使用新短信流程（字符串形式）
	 * @param deviceId          客户端设备标识 /与deviceIdServer一个值
	 * @param deviceIdServer    服务端设备标识
	 * @param randKey           当前 Gunion 会话使用的对称密钥
	 * @param outEncryptedParams 输出 Base64 编码后的加密参数
	 *
	 * @return
	 *   - true  加密成功
	 *   - false 参数构造或加密失败
	 */
	bool BuildGunionWegameSmsSendEncryptedParams(
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
	 * @brief 计算 Gunion-WeGame 短信发送接口请求签名（MD5）
	 *
	 * 说明：
	 *   - 迁移自 CGAuthClient::CalcSignatureCStypeWegameSmsSend
	 *   - 为 Gunion-WeGame 短信发送接口（/v1/basic/smssend）专属实现
	 *   - 按短信发送接口协议规则对参数排序、拼接并计算 MD5 签名
	 *
	 * 签名规则：
	 *   1. 将所有参与签名的参数按 key 字典序排序
	 *   2. 使用 key=value 形式拼接，中间以 '&' 分隔
	 *   3. 拼接当前 Gunion 会话 randKey
	 *   4. 转换为小写
	 *   5. 计算 MD5，输出 32 位大写 HEX 字符串
	 *
	 * 参与签名参数：
	 *   - phone
	 *   - type
	 *   - supportPic
	 *   - voiceMsg
	 *   - sms_new
	 *   - deviceid
	 *   - device_id_server
	 *
	 * @param phone          手机号码
	 * @param type           短信类型（字符串形式）
	 * @param supportPic     是否支持图片验证码（字符串形式）
	 * @param voiceMsg       语音提示文案
	 * @param smsNew         是否使用新短信流程（字符串形式）
	 * @param deviceId       客户端设备标识 /与deviceIdServer一个值
	 * @param deviceIdServer 服务端设备标识
	 * @param randKey        当前 Gunion 会话使用的对称密钥
	 * @param outSignature   输出 32 位大写 HEX 格式签名
	 *
	 * @return
	 *   - true  签名计算成功
	 *   - false 参数异常或计算失败
	 */
	bool CalcGunionWegameSmsSendSignature(
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
	// Gunion-WeGame PicSmsSend（带图形验证码短信发送）
	// ============================================================

	/**
	 * @brief 构建 Gunion-WeGame 带图形验证码短信发送接口的 3DES 加密参数
	 *
	 * 说明：
	 *   - 为 Gunion-WeGame 带图验短信发送接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 实际加密逻辑由 GunionCrypto 层完成
	 *
	 * 协议参数说明：
	 *   - phone               手机号码
	 *   - type                短信类型（如："5"，由协议固定）
	 *   - checkCodeGuid       图形验证码 GUID
	 *   - checkCode           普通图片验证码
	 *   - outInfo             验证码信息包括极验验证码交互后获得
		 *   - captchaInfo1        普通图验信息（imagecodeType = "1"）
		 *   - captchaInfo4        极验信息（imagecodeType = "4"）
	 *   - voiceMsg            语音提示文案
	 *   - sms_new             是否使用新短信流程（"0" / "1"）
	 *   - deviceid            客户端设备标识
	 *   - device_id_server    服务端下发的新设备标识
	 *
	 * @param phone             手机号码
	 * @param type              短信类型
	 * @param checkCodeGuid     图形验证码 GUID
	 * @param captchaInfo1      普通图验信息
	 * @param captchaInfo4      极验信息
	 * @param voiceMsg          语音提示文案
	 * @param smsNew            是否使用新短信流程
	 * @param deviceIdServer    服务端设备标识
	 * @param randKey           当前 Gunion 会话使用的对称密钥
	 * @param outEncryptedParams 输出 Base64 编码后的加密参数
	 *
	 * @return
	 *   - true  加密参数构建成功
	 *   - false 失败
	 */
	bool BuildGunionWegamePicSmsSendEncryptedParams(
		const std::string& phone,
		const std::string& type,
		const std::string& checkCodeGuid,
		const std::string& checkCode,
		const std::string& outInfo,
		const std::string& voiceMsg,
		const std::string& smsNew,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief 计算 Gunion-WeGame 带图形验证码短信发送接口请求签名（MD5）
	 *
	 * 说明：
	 *   - 为 Gunion-WeGame 带图验短信发送接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 签名算法由 GunionCrypto 层完成
	 *
	 * 签名规则：
	 *   - 参与签名参数排序后拼接
	 *   - 拼接 randKey
	 *   - 转小写后计算 MD5
	 *
	 * @param phone          手机号码
	 * @param type           短信类型
	 * @param checkCodeGuid  图形验证码 GUID
	 * @param checkCode      普通图片验证码
	 * @param outInfo        验证码信息包括极验验证码交互后获得
	 * @param voiceMsg       语音提示文案
	 * @param smsNew         是否使用新短信流程
	 * @param deviceIdServer 服务端设备标识
	 * @param randKey        当前 Gunion 会话使用的对称密钥
	 * @param outSignature   输出 32 位大写 HEX 格式签名
	 *
	 * @return
	 *   - true  签名计算成功
	 *   - false 失败
	 */
	bool CalcGunionWegamePicSmsSendSignature(
		const std::string& phone,
		const std::string& type,
		const std::string& checkCodeGuid,
		const std::string& checkCode,
		const std::string& outInfo,
		const std::string& voiceMsg,
		const std::string& smsNew,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion-WeGame 三方账号绑定手机号（ThirdAccountBindPhone）
	// ============================================================

	/**
	 * @brief 构建 Gunion-WeGame 三方账号绑定手机号接口的 3DES 加密参数
	 *
	 * 说明：
	 *   - LoginClient 仅负责流程与参数校验
	 *   - 实际加密算法由 GunionCrypto 层实现
	 *
	 * 协议参数说明：
	 *   - deviceid            客户端设备标识
	 *   - phone               手机号码
	 *   - smscode             短信验证码
	 *   - type                短信类型（如："5"，协议固定）
	 *   - scene               业务场景（如："gunionWegameLogin"）
	 *   - device_id_server    服务端下发的新设备标识
	 *
	 * @param deviceId          客户端设备标识
	 * @param phone             手机号码
	 * @param smscode           短信验证码
	 * @param type              短信类型
	 * @param scene             业务场景
	 * @param deviceIdServer    服务端设备标识
	 * @param randKey           当前 Gunion 会话使用的对称密钥
	 * @param outEncryptedParams 输出 Base64 编码后的加密参数
	 *
	 * @return
	 *   - true  成功
	 *   - false 失败
	 */
	bool BuildGunionWegameThirdAccountBindPhoneEncryptedParams(
		const std::string& deviceId,
		const std::string& phone,
		const std::string& smscode,
		const std::string& type,
		const std::string& scene,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief 计算 Gunion-WeGame 三方账号绑定手机号接口请求签名（MD5）
	 *
	 * 签名规则：
	 *   - 参数按 key 字典序排序
	 *   - key=value 拼接
	 *   - 拼接 randKey
	 *   - 转小写
	 *   - MD5 → 32 位大写 HEX
	 *
	 * @param deviceId        客户端设备标识
	 * @param phone           手机号码
	 * @param smscode         短信验证码
	 * @param type            短信类型
	 * @param scene           业务场景
	 * @param deviceIdServer  服务端设备标识
	 * @param randKey         当前 Gunion 会话使用的对称密钥
	 * @param outSignature    输出 32 位大写 HEX 签名
	 *
	 * @return
	 *   - true  成功
	 *   - false 失败
	 */
	bool CalcGunionWegameThirdAccountBindPhoneSignature(
		const std::string& deviceId,
		const std::string& phone,
		const std::string& smscode,
		const std::string& type,
		const std::string& scene,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion-WeGame 实名信息补填（FillRealinfo）
	// ============================================================

	/**
	 * @brief 构建 Gunion-WeGame 实名信息补填接口的 3DES 加密参数
	 *
	 * 说明：
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 实际加密算法由 GunionCrypto 层实现
	 *
	 * 协议参数说明：
	 *   - idcard              身份证号
	 *   - name                姓名（UTF-8）
	 *   - realInfoFlowId      实名流程 ID
	 *   - device_id_server    服务端下发的新设备标识
	 *
	 * @param idcard             身份证号
	 * @param nameUtf8           姓名（UTF-8 编码）
	 * @param realInfoFlowId     实名流程 ID
	 * @param deviceIdServer     服务端设备标识
	 * @param randKey            当前 Gunion 会话使用的对称密钥
	 * @param outEncryptedParams 输出 Base64 编码后的加密参数
	 *
	 * @return
	 *   - true  成功
	 *   - false 失败
	 */
	bool BuildGunionWegameFillRealinfoEncryptedParams(
		const std::string& idcard,
		const std::string& nameUtf8,
		const std::string& realInfoFlowId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief 计算 Gunion-WeGame 实名信息补填接口请求签名（MD5）
	 *
	 * 签名规则：
	 *   - 参数按 key 字典序排序
	 *   - key=value 拼接
	 *   - 拼接 randKey
	 *   - 转小写
	 *   - MD5 → 32 位大写 HEX
	 *
	 * @param idcard          身份证号
	 * @param nameUtf8        姓名（UTF-8 编码）
	 * @param realInfoFlowId  实名流程 ID
	 * @param deviceIdServer  服务端设备标识
	 * @param randKey         当前 Gunion 会话使用的对称密钥
	 * @param outSignature    输出 32 位大写 HEX 签名
	 *
	 * @return
	 *   - true  成功
	 *   - false 失败
	 */
	bool CalcGunionWegameFillRealinfoSignature(
		const std::string& idcard,
		const std::string& nameUtf8,
		const std::string& realInfoFlowId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);


	// ============================================================
	// Gunion-WeGame GetTicket / GetPayTicket（票据接口）
	// ============================================================

	/**
	 * @brief 构建 Gunion-WeGame 获取票据接口的 3DES 加密参数
	 *
	 * 协议参数：
	 *   - time                 票据有效时间（秒）
	 *   - device_id_server     服务端下发的新设备标识
	 */
	bool BuildGunionWegameGetTicketEncryptedParams(
		int time_s,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief 计算 Gunion-WeGame 获取票据接口请求签名（MD5）
	 *
	 * 签名规则：
	 *   - 参数 key=value 排序
	 *   - '&' 拼接
	 *   - 追加 randKey
	 *   - tolower
	 *   - MD5 → 32 位大写 HEX
	 */
	bool CalcGunionWegameGetTicketSignature(
		int time_s,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion SmsLogin（短信登录接口）
	// ============================================================

	/**
	 * @brief 构建 Gunion-WeGame 短信发送接口的 3DES 加密参数
	 *
	 * 说明：
	 *   - 迁移自 CGAuthClient::EncrytParams3DesCStyleWegameSmsSend
	 *   - 为 Gunion-WeGame 短信发送接口（/v1/basic/smssend）专属实现
	 *   - 按短信发送接口协议规则拼接参数并进行 3DES(EDE3/ECB) 加密
	 *
	 * 协议参数说明：
	 *   - phone               手机号码
	 *   - type                短信类型（如："5"，由旧协议约定）
	 *   - supportPic          是否支持图片验证码（"0" / "1"）
	 *   - voiceMsg            语音提示文案（允许为空）
	 *   - sms_new             是否使用新短信流程（"0" / "1"）
	 *   - deviceid            客户端设备标识（本地生成） /与deviceIdServer一个值
	 *   - device_id_server    服务端下发的新设备标识
	 *
	 * @param phone             手机号码
	 * @param type              短信类型（字符串形式，按协议固定值）
	 * @param supportPic        是否支持图片验证码（字符串形式）
	 * @param voiceMsg          语音提示文案
	 * @param smsNew            是否使用新短信流程（字符串形式）
	 * @param deviceId          客户端设备标识 /与deviceIdServer一个值
	 * @param deviceIdServer    服务端设备标识
	 * @param randKey           当前 Gunion 会话使用的对称密钥
	 * @param outEncryptedParams 输出 Base64 编码后的加密参数
	 *
	 * @return
	 *   - true  加密成功
	 *   - false 参数构造或加密失败
	 */
	bool BuildGunionSmsSendEncryptedParams(
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
	 * @brief 计算 Gunion 短信发送接口请求签名（MD5）
	 *
	 * 说明：
	 *   - 迁移自 CGAuthClient::CalcSignatureCStypeWegameSmsSend
	 *   - 为 Gunion 短信发送接口（/v1/basic/smssend）专属实现
	 *   - 按短信发送接口协议规则对参数排序、拼接并计算 MD5 签名
	 *
	 * 签名规则：
	 *   1. 将所有参与签名的参数按 key 字典序排序
	 *   2. 使用 key=value 形式拼接，中间以 '&' 分隔
	 *   3. 拼接当前 Gunion 会话 randKey
	 *   4. 转换为小写
	 *   5. 计算 MD5，输出 32 位大写 HEX 字符串
	 *
	 * 参与签名参数：
	 *   - phone
	 *   - type
	 *   - supportPic
	 *   - voiceMsg
	 *   - sms_new
	 *   - deviceid
	 *   - device_id_server
	 *
	 * @param phone          手机号码
	 * @param type           短信类型（字符串形式）
	 * @param supportPic     是否支持图片验证码（字符串形式）
	 * @param voiceMsg       语音提示文案
	 * @param smsNew         是否使用新短信流程（字符串形式）
	 * @param deviceId       客户端设备标识 /与deviceIdServer一个值
	 * @param deviceIdServer 服务端设备标识
	 * @param randKey        当前 Gunion 会话使用的对称密钥
	 * @param outSignature   输出 32 位大写 HEX 格式签名
	 *
	 * @return
	 *   - true  签名计算成功
	 *   - false 参数异常或计算失败
	 */
	bool CalcGunionSmsSendSignature(
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
	// Gunion PicSmsSend（带图形验证码短信发送）
	// ============================================================

	/**
	 * @brief 构建 Gunion 带图形验证码短信发送接口的 3DES 加密参数
	 *
	 * 说明：
	 *   - 为 Gunion-WeGame 带图验短信发送接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 实际加密逻辑由 GunionCrypto 层完成
	 *
	 * 协议参数说明：
	 *   - phone               手机号码
	 *   - type                短信类型（如："5"，由协议固定）
	 *   - checkCodeGuid       图形验证码 GUID
	 *   - checkCode           普通图片验证码
	 *   - outInfo             验证码信息包括极验验证码交互后获得
		 *   - captchaInfo1        普通图验信息（imagecodeType = "1"）
		 *   - captchaInfo4        极验信息（imagecodeType = "4"）
	 *   - voiceMsg            语音提示文案
	 *   - sms_new             是否使用新短信流程（"0" / "1"）
	 *   - deviceid            客户端设备标识
	 *   - device_id_server    服务端下发的新设备标识
	 *
	 * @param phone             手机号码
	 * @param type              短信类型
	 * @param checkCodeGuid     图形验证码 GUID
	 * @param captchaInfo1      普通图验信息
	 * @param captchaInfo4      极验信息
	 * @param voiceMsg          语音提示文案
	 * @param smsNew            是否使用新短信流程
	 * @param deviceIdServer    服务端设备标识
	 * @param randKey           当前 Gunion 会话使用的对称密钥
	 * @param outEncryptedParams 输出 Base64 编码后的加密参数
	 *
	 * @return
	 *   - true  加密参数构建成功
	 *   - false 失败
	 */
	bool BuildGunionPicSmsSendEncryptedParams(
		const std::string& phone,
		const std::string& type,
		const std::string& checkCodeGuid,
		const std::string& checkCode,
		const std::string& outInfo,
		const std::string& voiceMsg,
		const std::string& smsNew,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief 计算 Gunion-WeGame 带图形验证码短信发送接口请求签名（MD5）
	 *
	 * 说明：
	 *   - 为 Gunion-WeGame 带图验短信发送接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 签名算法由 GunionCrypto 层完成
	 *
	 * 签名规则：
	 *   - 参与签名参数排序后拼接
	 *   - 拼接 randKey
	 *   - 转小写后计算 MD5
	 *
	 * @param phone          手机号码
	 * @param type           短信类型
	 * @param checkCodeGuid  图形验证码 GUID
	 * @param checkCode      普通图片验证码
	 * @param outInfo        验证码信息包括极验验证码交互后获得
	 * @param voiceMsg       语音提示文案
	 * @param smsNew         是否使用新短信流程
	 * @param deviceIdServer 服务端设备标识
	 * @param randKey        当前 Gunion 会话使用的对称密钥
	 * @param outSignature   输出 32 位大写 HEX 格式签名
	 *
	 * @return
	 *   - true  签名计算成功
	 *   - false 失败
	 */
	bool CalcGunionPicSmsSendSignature(
		const std::string& phone,
		const std::string& type,
		const std::string& checkCodeGuid,
		const std::string& checkCode,
		const std::string& outInfo,
		const std::string& voiceMsg,
		const std::string& smsNew,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	/**
	 * @brief 构建 Gunion 短信登录接口的 3DES 加密参数
	 *
	 * 说明：
	 *   - 为 Gunion 短信登录接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 实际加密逻辑由 GunionCrypto 层完成
	 *
	 * 协议参数说明：
	 *   - sms_new           新版短信模版，兼容性字段，固定传1
	 *   - sms_type          短信验证码类型，短信登录固定传:4
	 *   - phone             手机号码
	 *   - smsCode           短信验证码
	 *   - windowsDeviceId   客户端设备标识（Windows 环境拼装/采集得到）
	 *   - deviceIdServer    服务端下发的新设备标识
	 *   - randKey           当前 Gunion 会话使用的对称密钥
	 *
	 * @param sms_new            新版短信模版，兼容性字段，固定传1
	 * @param sms_type           短信验证码类型，短信登录固定传:4
	 * @param phone              手机号码
	 * @param smsCode            短信验证码
	 * @param windowsDeviceId    客户端设备标识（BuildClientEnvironmentDeviceId()） /与deviceIdServer一个值
	 * @param deviceIdServer     服务端设备标识（服务端下发）
	 * @param randKey            当前 Gunion 会话使用的对称密钥
	 * @param outEncryptedParams 输出 Base64 编码后的加密参数
	 *
	 * @return
	 *   - true  加密参数构建成功
	 *   - false 失败
	 */
	bool BuildGunionSmsLoginEncryptedParams(
		const std::string& sms_new,
		const std::string& sms_type,
		const std::string& phone,
		const std::string& smsCode,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief 计算 Gunion 短信登录接口请求签名（MD5）
	 *
	 * 说明：
	 *   - 为 Gunion 短信登录接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 签名算法由 GunionCrypto 层完成
	 *
	 * 签名规则：
	 *   - 参与签名参数排序后拼接
	 *   - 拼接 randKey
	 *   - 转小写后计算 MD5
	 *
	 * @param sms_new        新版短信模版，兼容性字段，固定传1
	 * @param sms_type       短信验证码类型，短信登录固定传:4
	 * @param phone          手机号码
	 * @param smsCode        短信验证码
	 * @param windowsDeviceId客户端设备标识（BuildClientEnvironmentDeviceId()） /与deviceIdServer一个值
	 * @param deviceIdServer 服务端设备标识（服务端下发）
	 * @param randKey        当前 Gunion 会话使用的对称密钥
	 * @param outSignature   输出 32 位大写 HEX 格式签名
	 *
	 * @return
	 *   - true  签名计算成功
	 *   - false 失败
	 */
	bool CalcGunionSmsLoginSignature(
		const std::string& sms_new,
		const std::string& sms_type,
		const std::string& phone,
		const std::string& smsCode,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion RealName（实名认证接口）
	// ============================================================
	/**
	 * @brief 构建 Gunion 实名认证接口的 3DES 加密参数
	 *
	 * 说明：
	 *   - 为 Gunion 实名认证接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 实际加密逻辑由 GunionCrypto 层完成
	 *
	 * 协议参数说明：
	 *   - idcard           身份证号码
	 *   - name             真实姓名
	 *   - windowsDeviceId  客户端设备标识（Windows 环境拼装/采集得到）
	 *   - deviceIdServer   服务端下发的新设备标识
	 *   - randKey          当前 Gunion 会话使用的对称密钥
	 *
	 * 加密规则：
	 *   - 原始参数按协议顺序拼装
	 *   - 使用 randKey 进行 3DES 加密
	 *   - 对加密结果进行 Base64 编码
	 *
	 * @param idcard            身份证号码
	 * @param name              真实姓名
	 * @param windowsDeviceId   客户端设备标识（BuildClientEnvironmentDeviceId()）
	 * @param deviceIdServer    服务端设备标识（服务端下发）
	 * @param randKey           当前 Gunion 会话使用的对称密钥
	 * @param outEncryptedParams 输出 Base64 编码后的加密参数
	 *
	 * @return
	 *   - true  加密参数构建成功
	 *   - false 失败
	 */
	bool BuildGunionRealNameEncryptedParams(
		const std::string& idcard,
		const std::string& name,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief 计算 Gunion 实名认证接口请求签名（MD5）
	 *
	 * 说明：
	 *   - 为 Gunion 实名认证接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 签名算法由 GunionCrypto 层完成
	 *
	 * 签名规则：
	 *   - 参与签名参数排序后拼接
	 *   - 拼接 randKey
	 *   - 转小写后计算 MD5
	 *   - 输出 32 位大写 HEX 字符串
	 *
	 * @param idcard          身份证号码
	 * @param name            真实姓名
	 * @param windowsDeviceId 客户端设备标识（BuildClientEnvironmentDeviceId()）
	 * @param deviceIdServer  服务端设备标识（服务端下发）
	 * @param randKey         当前 Gunion 会话使用的对称密钥
	 * @param outSignature    输出 32 位大写 HEX 格式签名
	 *
	 * @return
	 *   - true  签名计算成功
	 *   - false 失败
	 */
	bool CalcGunionRealNameSignature(
		const std::string& idcard,
		const std::string& name,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion AutoLogin（自动登录接口）
	// ============================================================

	/**
	 * @brief 构建 Gunion 自动登录接口的 3DES 加密参数
	 *
	 * 说明：
	 *   - 为 Gunion 自动登录接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 实际加密逻辑由 GunionCrypto 层完成
	 *
	 * 协议参数说明：
	 *   - autokey          自动登录凭证（服务端下发）
	 *   - windowsDeviceId  客户端设备标识（Windows 环境拼装/采集得到）  /与deviceIdServer一个值
	 *   - deviceIdServer   服务端下发的新设备标识
	 *   - randKey          当前 Gunion 会话使用的对称密钥
	 *
	 * 加密规则：
	 *   - 原始参数按协议顺序拼装
	 *   - 使用 randKey 进行 3DES 加密
	 *   - 对加密结果进行 Base64 编码
	 *
	 * @param autokey           自动登录凭证
	 * @param windowsDeviceId   客户端设备标识（BuildClientEnvironmentDeviceId()）  /与deviceIdServer一个值
	 * @param deviceIdServer    服务端设备标识（服务端下发）
	 * @param randKey           当前 Gunion 会话使用的对称密钥
	 * @param outEncryptedParams 输出 Base64 编码后的加密参数
	 *
	 * @return
	 *   - true  加密参数构建成功
	 *   - false 失败
	 */
	bool BuildGunionAutoLoginEncryptedParams(
		const std::string& autokey,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief 计算 Gunion 自动登录接口请求签名（MD5）
	 *
	 * 说明：
	 *   - 为 Gunion 自动登录接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 签名算法由 GunionCrypto 层完成
	 *
	 * 签名规则：
	 *   - 参与签名参数排序后拼接
	 *   - 拼接 randKey
	 *   - 转小写后计算 MD5
	 *   - 输出 32 位大写 HEX 字符串
	 *
	 * @param autokey          自动登录凭证
	 * @param windowsDeviceId  客户端设备标识（BuildClientEnvironmentDeviceId()） /与deviceIdServer一个值
	 * @param deviceIdServer   服务端设备标识（服务端下发）
	 * @param randKey          当前 Gunion 会话使用的对称密钥
	 * @param outSignature     输出 32 位大写 HEX 格式签名
	 *
	 * @return
	 *   - true  签名计算成功
	 *   - false 失败
	 */
	bool CalcGunionAutoLoginSignature(
		const std::string& autokey,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion Logout（登出接口）
	// ============================================================

	/**
	 * @brief 构建 Gunion 登出接口的 3DES 加密参数
	 *
	 * 说明：
	 *   - 为 Gunion 登出接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 实际加密逻辑由 GunionCrypto 层完成
	 *
	 * 协议参数说明：
	 *   - autokey          当前用户自动登录凭证（服务端下发）
	 *   - deviceIdServer   服务端下发的新设备标识
	 *   - randKey          当前 Gunion 会话使用的对称密钥
	 *
	 * 加密规则：
	 *   - 原始参数按协议顺序拼装
	 *   - 使用 randKey 进行 3DES 加密
	 *   - 对加密结果进行 Base64 编码
	 *
	 * @param autokey            当前用户自动登录凭证）
	 * @param deviceIdServer     服务端设备标识（服务端下发）
	 * @param randKey            当前 Gunion 会话使用的对称密钥
	 * @param outEncryptedParams 输出 Base64 编码后的加密参数
	 *
	 * @return
	 *   - true  加密参数构建成功
	 *   - false 失败
	 */
	bool BuildGunionLogoutEncryptedParams(
		const std::string& autokey,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);


	/**
	 * @brief 计算 Gunion 登出接口请求签名（MD5）
	 *
	 * 说明：
	 *   - 为 Gunion 登出接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 签名算法由 GunionCrypto 层完成
	 *
	 * 签名规则：
	 *   - 参与签名参数排序后拼接
	 *   - 拼接 randKey
	 *   - 转小写后计算 MD5
	 *   - 输出 32 位大写 HEX 字符串
	 *
	 * @param autokey          当前用户自动登录凭证
	 * @param deviceIdServer   服务端设备标识（服务端下发）
	 * @param randKey          当前 Gunion 会话使用的对称密钥
	 * @param outSignature     输出 32 位大写 HEX 格式签名
	 *
	 * @return
	 *   - true  签名计算成功
	 *   - false 失败
	 */
	bool CalcGunionLogoutSignature(
		const std::string& autokey,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion PwdLogin（密码登录接口）
	// ============================================================

	/**
	 * @brief 构建 Gunion 密码登录接口的 3DES 加密参数
	 *
	 * 说明：
	 *   - 为 Gunion 密码登录接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 实际加密逻辑由 GunionCrypto 层完成
	 *
	 * 协议参数说明：
	 *   - phone             手机号码
	 *   - password          密码（明文输入，由协议侧加密传输）
	 *   - windowsDeviceId   客户端设备标识（Windows 环境拼装/采集得到）/与deviceIdServer一个值
	 *   - deviceIdServer    服务端下发的新设备标识
	 *   - randKey           当前 Gunion 会话使用的对称密钥
	 *
	 * 加密规则：
	 *   - 原始参数按协议顺序拼装
	 *   - 使用 randKey 进行 3DES 加密
	 *   - 对加密结果进行 Base64 编码
	 *
	 * @param phone             手机号码
	 * @param password          密码
	 * @param supportPic        是否支持图验，老逻辑兼容，新接入必传1
	 * @param windowsDeviceId   客户端设备标识（BuildClientEnvironmentDeviceId()）/与deviceIdServer一个值
	 * @param deviceIdServer    服务端设备标识（服务端下发）
	 * @param randKey           当前 Gunion 会话使用的对称密钥
	 * @param outEncryptedParams 输出 Base64 编码后的加密参数
	 *
	 * @return
	 *   - true  加密参数构建成功
	 *   - false 失败
	 */
	bool BuildGunionPwdLoginEncryptedParams(
		const std::string& phone,
		const std::string& password,
		const std::string& supportPic,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief 计算 Gunion 密码登录接口请求签名（MD5）
	 *
	 * 说明：
	 *   - 为 Gunion 密码登录接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 签名算法由 GunionCrypto 层完成
	 *
	 * 签名规则：
	 *   - 参与签名参数排序后拼接
	 *   - 拼接 randKey
	 *   - 转小写后计算 MD5
	 *   - 输出 32 位大写 HEX 字符串
	 *
	 * @param phone          手机号码
	 * @param password       密码
	*  @param supportPic        是否支持图验，老逻辑兼容，新接入必传1
	 * @param windowsDeviceId客户端设备标识（BuildClientEnvironmentDeviceId()）/与deviceIdServer一个值
	 * @param deviceIdServer 服务端设备标识（服务端下发）
	 * @param randKey        当前 Gunion 会话使用的对称密钥
	 * @param outSignature   输出 32 位大写 HEX 格式签名
	 *
	 * @return
	 *   - true  签名计算成功
	 *   - false 失败
	 */
	bool CalcGunionPwdLoginSignature(
		const std::string& phone,
		const std::string& password,
		const std::string& supportPic,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion PicPwdLogin（图验密码登录接口）
	// ============================================================

	/**
	 * @brief 构建 Gunion 图验密码登录接口的 3DES 加密参数
	 *
	 * 说明：
	 *   - 为 Gunion 图验密码登录接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 实际加密逻辑由 GunionCrypto 层完成
	 *
	 * 协议参数说明：
	 *   - checkCodeGuid    图形验证码 GUID（服务端下发）
	 *   - password          普通图片验证码，当上述smssend接口返回普通图验(imagecodeType=1)后时必传，
								 必须将用户输入验证码使用该字段传入，
								 历史遗留问题，需考虑兼容。字段名存在混淆，但此处 指代 checkCod
	 *   - outInfo           验证码信息包括极验验证码交互后获得，极验(imagecodeType=4)后时必传
	 *   - windowsDeviceId  客户端设备标识（Windows 环境拼装/采集得到）/与deviceIdServer一个值
	 *   - deviceIdServer   服务端下发的新设备标识
	 *   - randKey          当前 Gunion 会话使用的对称密钥
	 *
	 * 加密规则：
	 *   - 原始参数按协议顺序拼装
	 *   - 使用 randKey 进行 3DES 加密
	 *   - 对加密结果进行 Base64 编码
	 *
	 * @param checkCodeGuid     图形验证码 GUID
	 * @param password          普通图片验证码，当上述smssend接口返回普通图验(imagecodeType=1)后时必传，
								 必须将用户输入验证码使用该字段传入，
								 历史遗留问题，需考虑兼容。字段名存在混淆，但此处 指代 checkCod
	 * @param outInfo           验证码信息包括极验验证码交互后获得，极验(imagecodeType=4)后时必传
	 * @param windowsDeviceId   客户端设备标识（BuildClientEnvironmentDeviceId()）/与deviceIdServer一个值
	 * @param deviceIdServer    服务端设备标识（服务端下发）
	 * @param randKey           当前 Gunion 会话使用的对称密钥
	 * @param outEncryptedParams 输出 Base64 编码后的加密参数
	 *
	 * @return
	 *   - true  加密参数构建成功
	 *   - false 失败
	 */
	bool BuildGunionPicPwdLoginEncryptedParams(
		const std::string& checkCodeGuid,
		const std::string& password,
		const std::string& outInfo,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief 计算 Gunion 图验密码登录接口请求签名（MD5）
	 *
	 * 说明：
	 *   - 为 Gunion 图验密码登录接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 签名算法由 GunionCrypto 层完成
	 *
	 * 签名规则：
	 *   - 参与签名参数排序后拼接
	 *   - 拼接 randKey
	 *   - 转小写后计算 MD5
	 *   - 输出 32 位大写 HEX 字符串
	 *
	 * @param outInfo           验证码信息包括极验验证码交互后获得，极验(imagecodeType=4)后时必传
	 * @param checkCodeGuid     图形验证码 GUID
	 * @param password          普通图片验证码，当上述smssend接口返回普通图验(imagecodeType=1)后时必传，
								 必须将用户输入验证码使用该字段传入，
								 历史遗留问题，需考虑兼容。字段名存在混淆，但此处 指代 checkCode
	 * @param windowsDeviceId   客户端设备标识（BuildClientEnvironmentDeviceId()）/与deviceIdServer一个值
	 * @param deviceIdServer    服务端设备标识（服务端下发）
	 * @param randKey           当前 Gunion 会话使用的对称密钥
	 * @param outSignature      输出 32 位大写 HEX 格式签名
	 *
	 * @return
	 *   - true  签名计算成功
	 *   - false 失败
	 */
	bool CalcGunionPicPwdLoginSignature(
		const std::string& checkCodeGuid,
		const std::string& password,
		const std::string& outInfo,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion ThirdLogin（第三方登录接口）
	// ============================================================

	/**
	 * @brief 构建 Gunion 第三方登录接口的 3DES 加密参数
	 *
	 * 说明：
	 *   - 为 Gunion 第三方登录接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 实际加密逻辑由 GunionCrypto 层完成
	 *
	 * 协议参数说明：
	 *   - companyId        第三方平台标识（如 QQ / WeChat / WeGame 等）
	 *   - thirdTicket      第三方平台下发的登录凭证
	 *   - codeVerify       校验码（部分平台需要）
	 *   - openid           第三方平台用户唯一标识
	 *   - uid              平台内部用户 ID（如存在）
	 *   - windowsDeviceId  客户端设备标识（Windows 环境拼装/采集得到） /与deviceIdServer一个值
	 *   - deviceIdServer   服务端下发的新设备标识
	 *   - randKey          当前 Gunion 会话使用的对称密钥
	 *
	 * 加密规则：
	 *   - 原始参数按协议顺序拼装
	 *   - 使用 randKey 进行 3DES 加密
	 *   - 对加密结果进行 Base64 编码
	 *
	 * @param companyId         第三方平台标识
	 * @param thirdTicket       第三方登录凭证
	 * @param windowsDeviceId   客户端设备标识（BuildClientEnvironmentDeviceId()） /与deviceIdServer一个值
	 * @param deviceIdServer    服务端设备标识（服务端下发）
	 * @param randKey           当前 Gunion 会话使用的对称密钥
	 * @param outEncryptedParams 输出 Base64 编码后的加密参数
	 *
	 * @return
	 *   - true  加密参数构建成功
	 *   - false 失败
	 */
	bool BuildGunionThirdLoginEncryptedParams(
		const std::string& companyId,
		const std::string& thirdTicket,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief 计算 Gunion 第三方登录接口请求签名（MD5）
	 *
	 * 说明：
	 *   - 为 Gunion 第三方登录接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 签名算法由 GunionCrypto 层完成
	 *
	 * 签名规则：
	 *   - 参与签名参数排序后拼接
	 *   - 拼接 randKey
	 *   - 转小写后计算 MD5
	 *   - 输出 32 位大写 HEX 字符串
	 *
	 * @param companyId        第三方平台标识
	 * @param thirdTicket      第三方登录凭证
	 * @param windowsDeviceId  客户端设备标识（BuildClientEnvironmentDeviceId()）/与deviceIdServer一个值
	 * @param deviceIdServer   服务端设备标识（服务端下发）
	 * @param randKey          当前 Gunion 会话使用的对称密钥
	 * @param outSignature     输出 32 位大写 HEX 格式签名
	 *
	 * @return
	 *   - true  签名计算成功
	 *   - false 失败
	 */
	bool CalcGunionThirdLoginSignature(
		const std::string& companyId,
		const std::string& thirdTicket,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion LoginArea（区服记录接口）
	// ============================================================
	/**
	 * @brief 构建 Gunion 区服记录接口的 3DES 加密参数
	 *
	 * 说明：
	 *   - 为 Gunion 区服记录（登录区服上报/记录）接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 实际加密逻辑由 GunionCrypto 层完成
	 *
	 * 协议参数说明：
	 *   - userid           用户 ID（平台侧用户标识）
	 *   - area             大区/区服 ID（业务定义）
	 *   - group            分组/渠道/服务器组（业务定义）
	 *   - windowsDeviceId  客户端设备标识（Windows 环境拼装/采集得到）
	 *   - deviceIdServer   服务端下发的新设备标识
	 *   - randKey          当前 Gunion 会话使用的对称密钥
	 *
	 * 加密规则：
	 *   - 原始参数按协议顺序拼装
	 *   - 使用 randKey 进行 3DES 加密
	 *   - 对加密结果进行 Base64 编码
	 *
	 * @param userid            用户 ID
	 * @param area              大区/区服
	 * @param group             分组/服务器组
	 * @param windowsDeviceId   客户端设备标识（BuildClientEnvironmentDeviceId()） /与deviceIdServer一个值
	 * @param deviceIdServer    服务端设备标识（服务端下发）
	 * @param randKey           当前 Gunion 会话使用的对称密钥
	 * @param outEncryptedParams 输出 Base64 编码后的加密参数
	 *
	 * @return
	 *   - true  加密参数构建成功
	 *   - false 失败
	 */
	bool BuildGunionLoginAreaEncryptedParams(
		const std::string& userid,
		const std::string& area,
		const std::string& group,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief 计算 Gunion 区服记录接口请求签名（MD5）
	 *
	 * 说明：
	 *   - 为 Gunion 区服记录（登录区服上报/记录）接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 签名算法由 GunionCrypto 层完成
	 *
	 * 签名规则：
	 *   - 参与签名参数排序后拼接
	 *   - 拼接 randKey
	 *   - 转小写后计算 MD5
	 *   - 输出 32 位大写 HEX 字符串
	 *
	 * @param userid         用户 ID
	 * @param area           大区/区服
	 * @param group          分组/服务器组
	 * @param windowsDeviceId客户端设备标识（BuildClientEnvironmentDeviceId()） /与deviceIdServer一个值
	 * @param deviceIdServer 服务端设备标识（服务端下发）
	 * @param randKey        当前 Gunion 会话使用的对称密钥
	 * @param outSignature   输出 32 位大写 HEX 格式签名
	 *
	 * @return
	 *   - true  签名计算成功
	 *   - false 失败
	 */
	bool CalcGunionLoginAreaSignature(
		const std::string& userid,
		const std::string& area,
		const std::string& group,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion GetLoginArea（获取区服记录接口）
	// ============================================================
	/**
	 * @brief 构建 Gunion 获取区服记录接口的 3DES 加密参数
	 *
	 * 说明：
	 *   - 为 Gunion 获取区服记录接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 实际加密逻辑由 GunionCrypto 层完成
	 *
	 * 协议参数说明：
	 *   - deviceIdServer   服务端下发的新设备标识
	 *   - randKey          当前 Gunion 会话使用的对称密钥
	 *
	 * 加密规则：
	 *   - 原始参数按协议顺序拼装
	 *   - 使用 randKey 进行 3DES 加密
	 *   - 对加密结果进行 Base64 编码
	 *
	 * @param deviceIdServer    服务端设备标识（服务端下发）
	 * @param randKey           当前 Gunion 会话使用的对称密钥
	 * @param outEncryptedParams 输出 Base64 编码后的加密参数
	 *
	 * @return
	 *   - true  加密参数构建成功
	 *   - false 失败
	 */
	bool BuildGunionGetLoginAreaEncryptedParams(
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief 计算 Gunion 获取区服记录接口请求签名（MD5）
	 *
	 * 说明：
	 *   - 为 Gunion 获取区服记录接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 签名算法由 GunionCrypto 层完成
	 *
	 * 签名规则：
	 *   - 参与签名参数排序后拼接
	 *   - 拼接 randKey
	 *   - 转小写后计算 MD5
	 *   - 输出 32 位大写 HEX 字符串
	 *
	 * @param deviceIdServer   服务端设备标识（服务端下发）
	 * @param randKey          当前 Gunion 会话使用的对称密钥
	 * @param outSignature     输出 32 位大写 HEX 格式签名
	 *
	 * @return
	 *   - true  签名计算成功
	 *   - false 失败
	 */
	bool CalcGunionGetLoginAreaSignature(
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion SpecialPwdLogin（个性账号密码登录接口）
	// ============================================================

	/**
	 * @brief 构建 Gunion 个性账号密码登录接口的 3DES 加密参数
	 *
	 * 说明：
	 *   - 为 Gunion 个性账号密码登录接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 实际加密逻辑由 GunionCrypto 层完成
	 *
	 * 协议参数说明：
	 *   - account          个性账号（自定义账号/用户名）
	 *   - password         密码（明文输入，由协议侧加密传输）
	 *   - windowsDeviceId  客户端设备标识（Windows 环境拼装/采集得到）/与deviceIdServer一个值
	 *   - deviceIdServer   服务端下发的新设备标识
	 *   - randKey          当前 Gunion 会话使用的对称密钥
	 *
	 * 加密规则：
	 *   - 原始参数按协议顺序拼装
	 *   - 使用 randKey 进行 3DES 加密
	 *   - 对加密结果进行 Base64 编码
	 *
	 * @param account           个性账号
	 * @param password          密码
	 * @param windowsDeviceId   客户端设备标识（BuildClientEnvironmentDeviceId()）/与deviceIdServer一个值
	 * @param deviceIdServer    服务端设备标识（服务端下发）
	 * @param randKey           当前 Gunion 会话使用的对称密钥
	 * @param outEncryptedParams 输出 Base64 编码后的加密参数
	 *
	 * @return
	 *   - true  加密参数构建成功
	 *   - false 失败
	 */
	bool BuildGunionSpecialPwdLoginEncryptedParams(
		const std::string& account,
		const std::string& password,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief 计算 Gunion 个性账号密码登录接口请求签名（MD5）
	 *
	 * 说明：
	 *   - 为 Gunion 个性账号密码登录接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 签名算法由 GunionCrypto 层完成
	 *
	 * 签名规则：
	 *   - 参与签名参数排序后拼接
	 *   - 拼接 randKey
	 *   - 转小写后计算 MD5
	 *   - 输出 32 位大写 HEX 字符串
	 *
	 * @param account         个性账号
	 * @param password        密码
	 * @param windowsDeviceId 客户端设备标识（BuildClientEnvironmentDeviceId()）/与deviceIdServer一个值
	 * @param deviceIdServer  服务端设备标识（服务端下发）
	 * @param randKey         当前 Gunion 会话使用的对称密钥
	 * @param outSignature    输出 32 位大写 HEX 格式签名
	 *
	 * @return
	 *   - true  签名计算成功
	 *   - false 失败
	 */
	bool CalcGunionSpecialPwdLoginSignature(
		const std::string& account,
		const std::string& password,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion SpecialPicPwdLogin（个性账号图验密码登录接口）
	// ============================================================

	/**
	 * @brief 构建 Gunion 个性账号图验密码登录接口的 3DES 加密参数
	 *
	 * 说明：
	 *   - 为 Gunion 个性账号图验密码登录接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 实际加密逻辑由 GunionCrypto 层完成
	 *
	 * 协议参数说明：
	 *   - imageType        图验类型
	 *                        "1"：普通图形验证码
	 *                        "4"：极验验证码
	 *   - checkCodeGuid    图形验证码 GUID（服务端下发）
	 *   - captchaInfo      验证码交互信息
	 *                        普通图验：图片验证码字符串
	 *                        极验：极验交互返回 JSON 数据
	 *   - windowsDeviceId  客户端设备标识（Windows 环境拼装/采集得到）/与deviceIdServer一个值
	 *   - deviceIdServer   服务端下发的新设备标识
	 *   - randKey          当前 Gunion 会话使用的对称密钥
	 *
	 * 加密规则：
	 *   - 原始参数按协议顺序拼装
	 *   - 使用 randKey 进行 3DES 加密
	 *   - 对加密结果进行 Base64 编码
	 *
	 * @param checkCodeGuid     图形验证码 GUID
	 * @param outInfo           验证码交互信息
	 * @param windowsDeviceId   客户端设备标识（BuildClientEnvironmentDeviceId()）/与deviceIdServer一个值
	 * @param deviceIdServer    服务端设备标识（服务端下发）
	 * @param randKey           当前 Gunion 会话使用的对称密钥
	 * @param outEncryptedParams 输出 Base64 编码后的加密参数
	 *
	 * @return
	 *   - true  加密参数构建成功
	 *   - false 失败
	 */
	bool BuildGunionSpecialPicPwdLoginEncryptedParams(
		const std::string& checkCodeGuid,
		const std::string& outInfo,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief 计算 Gunion 个性账号图验密码登录接口请求签名（MD5）
	 *
	 * 说明：
	 *   - 为 Gunion 个性账号图验密码登录接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 签名算法由 GunionCrypto 层完成
	 *
	 * 签名规则：
	 *   - 参与签名参数排序后拼接
	 *   - 拼接 randKey
	 *   - 转小写后计算 MD5
	 *   - 输出 32 位大写 HEX 字符串
	 *
	 * @param checkCodeGuid     图形验证码 GUID
	 * @param outInfo           验证码交互信息
	 * @param windowsDeviceId   客户端设备标识（BuildClientEnvironmentDeviceId()）/与deviceIdServer一个值
	 * @param deviceIdServer    服务端设备标识（服务端下发）
	 * @param randKey           当前 Gunion 会话使用的对称密钥
	 * @param outSignature      输出 32 位大写 HEX 格式签名
	 *
	 * @return
	 *   - true  签名计算成功
	 *   - false 失败
	 */
	bool CalcGunionSpecialPicPwdLoginSignature(
		const std::string& checkCodeGuid,
		const std::string& outInfo,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion SpecialSmsSend（个性账号短信发送接口）
	// ============================================================

	/**
	 * @brief 构建 Gunion 个性账号短信发送接口的 3DES 加密参数
	 *
	 * 说明：
	 *   - 为 Gunion 个性账号短信发送接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 实际加密逻辑由 GunionCrypto 层完成
	 *
	 * 协议参数说明：
	 *   - account          个性账号（自定义账号/用户名）
	 *   - voiceMsg         语音提示文案（可为空，按业务透传）
	 *   - sms_new          是否使用新短信流程（"0" / "1"）
	 *   - windowsDeviceId  客户端设备标识（Windows 环境拼装/采集得到）/与deviceIdServer一个值
	 *   - deviceIdServer   服务端下发的新设备标识
	 *   - randKey          当前 Gunion 会话使用的对称密钥
	 *
	 * 加密规则：
	 *   - 原始参数按协议顺序拼装
	 *   - 使用 randKey 进行 3DES 加密
	 *   - 对加密结果进行 Base64 编码
	 *
	 * @param account           个性账号
	 * @param voiceMsg          语音提示文案
	 * @param smsNew            是否使用新短信流程（"0" / "1"）
	 * @param windowsDeviceId   客户端设备标识（BuildClientEnvironmentDeviceId()） /与deviceIdServer一个值
	 * @param deviceIdServer    服务端设备标识（服务端下发）
	 * @param randKey           当前 Gunion 会话使用的对称密钥
	 * @param outEncryptedParams 输出 Base64 编码后的加密参数
	 *
	 * @return
	 *   - true  加密参数构建成功
	 *   - false 失败
	 */
	bool BuildGunionSpecialSmsSendEncryptedParams(
		const std::string& account,
		const std::string& voiceMsg,
		const std::string& smsNew,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief 计算 Gunion 个性账号短信发送接口请求签名（MD5）
	 *
	 * 说明：
	 *   - 为 Gunion 个性账号短信发送接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 签名算法由 GunionCrypto 层完成
	 *
	 * 签名规则（个性账号 SmsSend 专属规则）：
	 *   - 参与签名参数排序后拼接（字段集合以本接口为准）
	 *   - 拼接 randKey
	 *   - 转小写后计算 MD5
	 *   - 输出 32 位大写 HEX 字符串
	 *
	 * @param account         个性账号
	 * @param voiceMsg        语音提示文案
	 * @param smsNew          是否使用新短信流程（"0" / "1"）
	 * @param windowsDeviceId 客户端设备标识（BuildClientEnvironmentDeviceId()） /与deviceIdServer一个值
	 * @param deviceIdServer  服务端设备标识（服务端下发）
	 * @param randKey         当前 Gunion 会话使用的对称密钥
	 * @param outSignature    输出 32 位大写 HEX 格式签名
	 *
	 * @return
	 *   - true  签名计算成功
	 *   - false 失败
	 */
	bool CalcGunionSpecialSmsSendSignature(
		const std::string& account,
		const std::string& voiceMsg,
		const std::string& smsNew,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion SpecialPicSmsSend（个性账号图验短信发送接口）
	// ============================================================

	/**
	 * @brief 构建 Gunion 个性账号图验短信发送接口的 3DES 加密参数
	 *
	 * 说明：
	 *   - 为 Gunion 个性账号图验短信发送接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 实际加密逻辑由 GunionCrypto 层完成
	 *
	 * 协议参数说明：
	 *   - imagecodeType    图验类型
	 *                        "1"：普通图形验证码
	 *                        "4"：极验验证码
	 *   - captchaInfo      验证码交互信息
	 *                        普通图验：图片验证码字符串
	 *                        极验：极验交互返回 JSON 数据
	 *   - smsSendGuid      短信发送图验 GUID（服务端下发）
	 *   - windowsDeviceId  客户端设备标识（Windows 环境拼装/采集得到）
	 *   - deviceIdServer   服务端下发的新设备标识
	 *   - randKey          当前 Gunion 会话使用的对称密钥
	 *
	 * 加密规则：
	 *   - 原始参数按协议顺序拼装
	 *   - 使用 randKey 进行 3DES 加密
	 *   - 对加密结果进行 Base64 编码
	 *
	 * @param captchaInfo       验证码交互信息
	 * @param smsSendGuid       短信发送图验 GUID
	 * @param deviceIdServer    服务端设备标识（服务端下发）
	 * @param randKey           当前 Gunion 会话使用的对称密钥
	 * @param outEncryptedParams 输出 Base64 编码后的加密参数
	 *
	 * @return
	 *   - true  加密参数构建成功
	 *   - false 失败
	 */
	bool BuildGunionSpecialPicSmsSendEncryptedParams(
		const std::string& captchaInfo,
		const std::string& smsSendGuid,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief 计算 Gunion 个性账号图验短信发送接口请求签名（MD5）
	 *
	 * 说明：
	 *   - 为 Gunion 个性账号图验短信发送接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 签名算法由 GunionCrypto 层完成
	 *
	 * 签名规则（个性账号 PicSmsSend 专属规则）：
	 *   - 参与签名参数排序后拼接（字段集合以本接口为准）
	 *   - 拼接 randKey
	 *   - 转小写后计算 MD5
	 *   - 输出 32 位大写 HEX 字符串
	 *
	 * @param captchaInfo       验证码交互信息
	 * @param smsSendGuid       短信发送图验 GUID
	 * @param deviceIdServer    服务端设备标识（服务端下发）
	 * @param randKey           当前 Gunion 会话使用的对称密钥
	 * @param outSignature      输出 32 位大写 HEX 格式签名
	 *
	 * @return
	 *   - true  签名计算成功
	 *   - false 失败
	 */
	bool CalcGunionSpecialPicSmsSendSignature(
		const std::string& captchaInfo,
		const std::string& smsSendGuid,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion SpecialConfirmSmsSend（短信发送确认接口）
	// ============================================================

	/**
	 * @brief 构建 Gunion 短信发送确认接口的 3DES 加密参数
	 *
	 * 说明：
	 *   - 为 Gunion ConfirmSmsSend（短信发送确认）接口专属实现
	 *   - 当前场景用于个性账号短信发送流程的确认/校验步骤
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 实际加密逻辑由 GunionCrypto 层完成
	 *
	 * 协议参数说明：
	 *   - smsSendGuid      短信发送 GUID（服务端下发/流程中产生）
	 *   - deviceIdServer   服务端下发的新设备标识
	 *   - randKey          当前 Gunion 会话使用的对称密钥
	 *
	 * 加密规则：
	 *   - 原始参数按协议顺序拼装
	 *   - 使用 randKey 进行 3DES 加密
	 *   - 对加密结果进行 Base64 编码
	 *
	 * @param smsSendGuid       短信发送 GUID
	 * @param deviceIdServer    服务端设备标识（服务端下发）
	 * @param randKey           当前 Gunion 会话使用的对称密钥
	 * @param outEncryptedParams 输出 Base64 编码后的加密参数
	 *
	 * @return
	 *   - true  加密参数构建成功
	 *   - false 失败
	 */
	bool BuildGunionSpecialConfirmSmsSendEncryptedParams(
		const std::string& smsSendGuid,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief 计算 Gunion 短信发送确认接口请求签名（MD5）
	 *
	 * 说明：
	 *   - 为 Gunion ConfirmSmsSend（短信发送确认）接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 签名算法由 GunionCrypto 层完成
	 *
	 * 签名规则（ConfirmSmsSend 专属规则）：
	 *   - 参与签名参数排序后拼接（字段集合以本接口为准）
	 *   - 拼接 randKey
	 *   - 转小写后计算 MD5
	 *   - 输出 32 位大写 HEX 字符串
	 *
	 * @param smsSendGuid      短信发送 GUID
	 * @param deviceIdServer   服务端设备标识（服务端下发）
	 * @param randKey          当前 Gunion 会话使用的对称密钥
	 * @param outSignature     输出 32 位大写 HEX 格式签名
	 *
	 * @return
	 *   - true  签名计算成功
	 *   - false 失败
	 */
	bool CalcGunionSpecialConfirmSmsSendSignature(
		const std::string& smsSendGuid,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion SpecialCheckSmsLogin（个性账号短信登录校验接口）
	// ============================================================

	/**
	 * @brief 构建 Gunion 个性账号短信登录校验接口的 3DES 加密参数
	 *
	 * 说明：
	 *   - 为 Gunion 个性账号短信登录校验接口专属实现
	 *   - 当前场景用于个性账号短信验证码登录流程
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 实际加密逻辑由 GunionCrypto 层完成
	 *
	 * 协议参数说明：
	 *   - smsSendGuid      短信发送 GUID（短信发送阶段产生）
	 *   - smsCode          短信验证码
	 *   - account          个性账号（自定义账号/用户名）
	 *   - windowsDeviceId  客户端设备标识（Windows 环境拼装/采集得到） /与deviceIdServer一个值
	 *   - deviceIdServer   服务端下发的新设备标识
	 *   - randKey          当前 Gunion 会话使用的对称密钥
	 *
	 * 加密规则：
	 *   - 原始参数按协议顺序拼装
	 *   - 使用 randKey 进行 3DES 加密
	 *   - 对加密结果进行 Base64 编码
	 *
	 * @param smsSendGuid       短信发送 GUID
	 * @param smsCode           短信验证码
	 * @param account           个性账号
	 * @param windowsDeviceId   客户端设备标识（BuildClientEnvironmentDeviceId()) /与deviceIdServer一个值
	 * @param deviceIdServer    服务端设备标识（服务端下发）
	 * @param randKey           当前 Gunion 会话使用的对称密钥
	 * @param outEncryptedParams 输出 Base64 编码后的加密参数
	 *
	 * @return
	 *   - true  加密参数构建成功
	 *   - false 失败
	 */
	bool BuildGunionSpecialCheckSmsLoginEncryptedParams(
		const std::string& smsSendGuid,
		const std::string& smsCode,
		const std::string& account,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief 计算 Gunion 个性账号短信登录校验接口请求签名（MD5）
	 *
	 * 说明：
	 *   - 为 Gunion 个性账号短信登录校验接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 签名算法由 GunionCrypto 层完成
	 *
	 * 签名规则（SpecialCheckSmsLogin 专属规则）：
	 *   - 参与签名参数排序后拼接（字段集合以本接口为准）
	 *   - 拼接 randKey
	 *   - 转小写后计算 MD5
	 *   - 输出 32 位大写 HEX 字符串
	 *
	 * @param smsSendGuid      短信发送 GUID
	 * @param smsCode          短信验证码
	 * @param account          个性账号
	 * @param windowsDeviceId  客户端设备标识（BuildClientEnvironmentDeviceId()） /与deviceIdServer一个值
	 * @param deviceIdServer   服务端设备标识（服务端下发）
	 * @param randKey          当前 Gunion 会话使用的对称密钥
	 * @param outSignature     输出 32 位大写 HEX 格式签名
	 *
	 * @return
	 *   - true  签名计算成功
	 *   - false 失败
	 */
	bool CalcGunionSpecialCheckSmsLoginSignature(
		const std::string& smsSendGuid,
		const std::string& smsCode,
		const std::string& account,
		const std::string& windowsDeviceId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion PayEntrance（支付入口接口）
	// ============================================================

	/**
	 * @brief 构建 Gunion 支付入口接口的 3DES 加密参数
	 *
	 * 说明：
	 *   - 为 Gunion PayEntrance（支付入口）接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 实际加密逻辑由 GunionCrypto 层完成
	 *
	 * 协议参数说明：
	 *   - orderId          订单号（业务生成的唯一订单标识）
	 *   - productId        商品 ID
	 *   - groupId          分组 ID（服务器组/渠道组）
	 *   - areaId           大区 ID
	 *   - extend           扩展字段（业务透传信息，如角色信息/附加参数）
	 *   - deviceIdServer   服务端下发的新设备标识
	 *   - randKey          当前 Gunion 会话使用的对称密钥
	 *
	 * 加密规则：
	 *   - 原始参数按协议顺序拼装
	 *   - 使用 randKey 进行 3DES 加密
	 *   - 对加密结果进行 Base64 编码
	 *
	 * @param orderId           订单号
	 * @param productId         商品 ID
	 * @param groupId           分组 ID
	 * @param areaId            大区 ID
	 * @param extend            扩展字段
	 * @param deviceIdServer    服务端设备标识（服务端下发）
	 * @param randKey           当前 Gunion 会话使用的对称密钥
	 * @param outEncryptedParams 输出 Base64 编码后的加密参数
	 *
	 * @return
	 *   - true  加密参数构建成功
	 *   - false 失败
	 */
	bool BuildGunionPayEntranceEncryptedParams(
		const std::string& orderId,
		const std::string& productId,
		const std::string& groupId,
		const std::string& areaId,
		const std::string& extend,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief 计算 Gunion 支付入口接口请求签名（MD5）
	 *
	 * 说明：
	 *   - 为 Gunion PayEntrance（支付入口）接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 签名算法由 GunionCrypto 层完成
	 *
	 * 签名规则（PayEntrance 专属规则）：
	 *   - 参与签名参数排序后拼接（字段集合以本接口为准）
	 *   - 拼接 randKey
	 *   - 转小写后计算 MD5
	 *   - 输出 32 位大写 HEX 字符串
	 *
	 * @param orderId          订单号
	 * @param productId        商品 ID
	 * @param groupId          分组 ID
	 * @param areaId           大区 ID
	 * @param extend           扩展字段
	 * @param deviceIdServer   服务端设备标识（服务端下发）
	 * @param randKey          当前 Gunion 会话使用的对称密钥
	 * @param outSignature     输出 32 位大写 HEX 格式签名
	 *
	 * @return
	 *   - true  签名计算成功
	 *   - false 失败
	 */
	bool CalcGunionPayEntranceSignature(
		const std::string& orderId,
		const std::string& productId,
		const std::string& groupId,
		const std::string& areaId,
		const std::string& extend,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion CheckPayOrderStatus（查询支付订单状态接口）
	// ============================================================

	/**
	 * @brief 构建 Gunion 查询支付订单状态接口的 3DES 加密参数
	 *
	 * 说明：
	 *   - 为 Gunion CheckPayOrderStatus（查询支付订单状态）接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 实际加密逻辑由 GunionCrypto 层完成
	 *
	 * 协议参数说明：
	 *   - orderId          订单号（业务生成的唯一订单标识）
	 *   - randKey          当前 Gunion 会话使用的对称密钥
	 *
	 * 加密规则：
	 *   - 原始参数按协议顺序拼装
	 *   - 使用 randKey 进行 3DES 加密
	 *   - 对加密结果进行 Base64 编码
	 *
	 * @param orderId           订单号
	 * @param randKey           当前 Gunion 会话使用的对称密钥
	 * @param outEncryptedParams 输出 Base64 编码后的加密参数
	 *
	 * @return
	 *   - true  加密参数构建成功
	 *   - false 失败
	 */
	bool BuildGunionCheckPayOrderStatusEncryptedParams(
		const std::string& orderId,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief 计算 Gunion 查询支付订单状态接口请求签名（MD5）
	 *
	 * 说明：
	 *   - 为 Gunion CheckPayOrderStatus（查询支付订单状态）接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 签名算法由 GunionCrypto 层完成
	 *
	 * 签名规则（PayOrderStatus 专属规则）：
	 *   - 参与签名参数排序后拼接（字段集合以本接口为准）
	 *   - 拼接 randKey
	 *   - 转小写后计算 MD5
	 *   - 输出 32 位大写 HEX 字符串
	 *
	 * @param orderId          订单号
	 * @param randKey          当前 Gunion 会话使用的对称密钥
	 * @param outSignature     输出 32 位大写 HEX 格式签名
	 *
	 * @return
	 *   - true  签名计算成功
	 *   - false 失败
	 */
	bool CalcGunionCheckPayOrderStatusSignature(
		const std::string& orderId,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion CheckActivation（激活校验接口）
	// ============================================================

	/**
	 * @brief 构建 Gunion 激活校验接口的 3DES 加密参数
	 *
	 * 说明：
	 *   - 为 Gunion CheckActivation（激活校验）接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 实际加密逻辑由 GunionCrypto 层完成
	 *
	 * 协议参数说明：
	 *   - activation       激活码/激活标识（业务字段）
	 *   - windowsDeviceId  客户端设备标识（Windows 环境拼装/采集得到）
	 *   - deviceIdServer   服务端下发的新设备标识
	 *   - randKey          当前 Gunion 会话使用的对称密钥
	 *
	 * 加密规则：
	 *   - 原始参数按协议顺序拼装
	 *   - 使用 randKey 进行 3DES 加密
	 *   - 对加密结果进行 Base64 编码
	 *
	 * @param activation        激活码/激活标识
	 * @param deviceIdServer    服务端设备标识（服务端下发）
	 * @param randKey           当前 Gunion 会话使用的对称密钥
	 * @param outEncryptedParams 输出 Base64 编码后的加密参数
	 *
	 * @return
	 *   - true  加密参数构建成功
	 *   - false 失败
	 */
	bool BuildGunionCheckActivationEncryptedParams(
		const std::string& activation,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);


	/**
	 * @brief 计算 Gunion 激活校验接口请求签名（MD5）
	 *
	 * 说明：
	 *   - 为 Gunion CheckActivation（激活校验）接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 签名算法由 GunionCrypto 层完成
	 *
	 * 签名规则（Activation 专属规则）：
	 *   - 参与签名参数排序后拼接（字段集合以本接口为准）
	 *   - 拼接 randKey
	 *   - 转小写后计算 MD5
	 *   - 输出 32 位大写 HEX 字符串
	 *
	 * @param activation       激活码/激活标识
	 * @param deviceIdServer   服务端设备标识（服务端下发）
	 * @param randKey          当前 Gunion 会话使用的对称密钥
	 * @param outSignature     输出 32 位大写 HEX 格式签名
	 *
	 * @return
	 *   - true  签名计算成功
	 *   - false 失败
	 */
	bool CalcGunionCheckActivationSignature(
		const std::string& activation,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion SetTutorInfo（监护人信息收集接口）
	// ============================================================

	/**
	 * @brief 构建 Gunion 监护人信息收集接口的 3DES 加密参数
	 *
	 * 说明：
	 *   - 用于未成年人实名认证后的监护人信息提交
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 实际加密逻辑由 GunionCrypto 层完成
	 *
	 * 协议参数说明：
	 *   - idcard            监护人身份证号
	 *   - name              监护人姓名
	 *   - phone             监护人手机号
	 *   - smscode           短信验证码
	 *   - confirmKey        二次确认 Key
	 *   - deviceIdServer    服务端下发的新设备标识
	 *   - randKey           当前 Gunion 会话使用的对称密钥
	 *
	 * 加密规则：
	 *   - 按协议字段顺序拼装原始字符串
	 *   - 使用 randKey 进行 3DES 加密
	 *   - 对加密结果进行 Base64 编码
	 *
	 * @param idcard              监护人身份证号
	 * @param name                监护人姓名
	 * @param phone               监护人手机号
	 * @param smscode             短信验证码
	 * @param confirmKey          二次确认 Key
	 * @param deviceIdServer      服务端设备标识
	 * @param randKey             当前会话对称密钥
	 * @param outEncryptedParams  输出 Base64 编码后的加密参数
	 *
	 * @return
	 *   - true  构建成功
	 *   - false 构建失败
	 */
	bool BuildGunionSetTutorInfoEncryptedParams(
		const std::string& idcard,
		const std::string& name,
		const std::string& phone,
		const std::string& smscode,
		const std::string& confirmKey,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief 计算 Gunion 监护人信息收集接口请求签名（MD5）
	 *
	 * 说明：
	 *   - 为 Gunion SetTutorInfo 接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 签名算法由 GunionCrypto 层完成
	 *
	 * 签名规则：
	 *   - 参与签名字段排序后拼接
	 *   - 拼接 randKey
	 *   - 转小写后计算 MD5
	 *   - 输出 32 位大写 HEX 字符串
	 *
	 * @param idcard         监护人身份证号
	 * @param name           监护人姓名
	 * @param phone          监护人手机号
	 * @param smscode        短信验证码
	 * @param confirmKey     二次确认 Key
	 * @param deviceIdServer 服务端设备标识
	 * @param randKey        当前会话对称密钥
	 * @param outSignature   输出签名结果（32 位大写 HEX）
	 *
	 * @return
	 *   - true  计算成功
	 *   - false 计算失败
	 */
	bool CalcGunionSetTutorInfoSignature(
		const std::string& idcard,
		const std::string& name,
		const std::string& phone,
		const std::string& smscode,
		const std::string& confirmKey,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);

	// ============================================================
	// Gunion ChannelLogin（Wegame账号绑定登录接口）
	// ============================================================

	/**
	 * @brief 构建 Gunion Wegame账号绑定登录接口的 3DES 加密参数
	 *
	 * 说明：
	 *   - 用于 Gunion Wegame账号绑定登录
	 *   - selectLoginContext 为渠道返回的登录上下文信息
	 *   - companyId 为渠道公司标识
	 *   - LoginClient 仅负责流程控制
	 *   - 实际加密逻辑由 GunionCrypto 层完成
	 *
	 * 协议参数说明：
	 *   - selectLoginContext 渠道登录上下文（JSON 或字符串）
	 *   - companyId          渠道公司ID
	 *   - windowsDeviceId    客户端设备标识
	 *   - deviceIdServer     服务端下发的新设备标识
	 *   - randKey            当前 Gunion 会话使用的对称密钥
	 *
	 * 加密规则：
	 *   - 按协议字段顺序拼装原始字符串
	 *   - 使用 randKey 进行 3DES 加密
	 *   - 对加密结果进行 Base64 编码
	 *
	 * @param selectLoginContext 渠道登录上下文
	 * @param companyId          渠道公司ID
	 * @param deviceIdServer     服务端设备标识
	 * @param randKey            当前会话对称密钥
	 * @param outEncryptedParams 输出 Base64 编码后的加密参数
	 *
	 * @return
	 *   - true  构建成功
	 *   - false 构建失败
	 */
	bool BuildGunionWegameChannelLoginEncryptedParams(
		const std::string& selectLoginContext,
		const std::string& companyId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outEncryptedParams);

	/**
	 * @brief 计算 Gunion-WeGame 渠道登录接口请求签名（MD5）
	 *
	 * 说明：
	 *   - 为 Gunion-WeGame ChannelLogin 接口专属实现
	 *   - LoginClient 仅负责参数校验与流程控制
	 *   - 签名算法由 GunionCrypto 层完成
	 *
	 * 签名规则：
	 *   - 参与签名字段排序后拼接
	 *   - 拼接 randKey
	 *   - 转小写后计算 MD5
	 *   - 输出 32 位大写 HEX 字符串
	 *
	 * @param selectLoginContext 渠道登录上下文
	 * @param companyId          渠道公司ID
	 * @param deviceIdServer     服务端设备标识
	 * @param randKey            当前会话对称密钥
	 * @param outSignature       输出签名结果（32 位大写 HEX）
	 *
	 * @return
	 *   - true  计算成功
	 *   - false 计算失败
	 */
	bool CalcGunionWegameChannelLoginSignature(
		const std::string& selectLoginContext,
		const std::string& companyId,
		const std::string& deviceIdServer,
		const std::string& randKey,
		std::string& outSignature);
public:
	/**
	 * @brief 生成客户端环境签名deviceId（Client Signature）
	 *
	 * 说明：
	 *   - 迁移自历史 GetClientSign2 实现
	 *   - 使用 MAC / CPU 信息 / 磁盘序列号 生成客户端指纹
	 *   - 各字段分别做 MD5，再用 ':' 拼接
	 *   - 用于客户端唯一性识别（非 Gunion Handshake 签名）
	 *
	 * @return
	 *   - 非空字符串：生成成功
	 *   - 空字符串：获取设备信息失败
	 */
	std::string BuildClientEnvironmentDeviceId();


	/**
	 * @brief 获取客户端 MAC 地址（历史实现迁移）
	 *
	 * 说明：
	 *   - 迁移自旧版 static GetMacAddress
	 *   - 使用 GetAdaptersInfo 枚举本机网卡
	 *   - 返回第一个具有 IP 的网卡 MAC
	 *   - MAC 格式：XX-XX-XX-XX-XX-XX（大写）
	 *
	 * @return
	 *   - 非空字符串：获取成功
	 *   - 空字符串：失败
	 */
	std::string GetMacAddress();
private:
	// ============================================================
	// Gunion 会话状态（Handshake 生命周期内有效）
	// ============================================================

	/**
	 * @brief Gunion 服务端下发的 RSA 公钥（PEM 格式）
	 *
	 * 来源：
	 *   - /v1/basic/publickey 接口返回
	 *
	 * 用途：
	 *   - Handshake 阶段用于加密客户端生成的 randKey
	 *
	 * 说明：
	 *   - 包含 "BEGIN PUBLIC KEY" / "END PUBLIC KEY" 头尾
	 *   - 仅在会话建立前使用
	 */
	std::string m_gunionPublicKey;

	/**
	 * @brief Gunion 会话使用的随机对称密钥（randKey）
	 *
	 * 来源：
	 *   - 客户端在 Handshake 前生成（长度通常为 RAND_KEY_LEN）
	 *
	 * 用途：
	 *   - Handshake 请求中被 RSA 公钥加密发送给服务端
	 *   - Handshake 成功后，用于 3DES 解密服务端返回数据
	 *   - 后续所有 Gunion 接口的 3DES 加解密
	 *
	 * 生命周期：
	 *   - 从 Handshake 开始生成
	 *   - 会话结束 / 登录结束后失效
	 */
	std::string m_gunionRandKey;

	/**
	 * @brief Gunion 会话令牌（token）
	 *
	 * 来源：
	 *   - /v1/basic/handshake 接口返回（3DES 解密后获取）
	 *
	 * 用途：
	 *   - 作为 X-TOKEN 头部参数
	 *   - 标识当前 Gunion 会话的合法性
	 *
	 * 生命周期：
	 *   - Handshake 成功后生效
	 *   - 登录流程结束或会话失效后清空
	 */
	std::string m_gunionToken;

	// gunion phone（用于 SmsSend → PicSmsSend 链路）
	std::string gunion_phone_str;

	// m_enableGunionPreInit（用于判断是否走gunion逻辑）
	bool m_enableGunionPreInit;
private:
	/**
	 * @brief Gunion-WeGame 业务安全握手管理对象
	 *
	 * 设计目的：
	 *   - 承载 Gunion-WeGame 登录体系中的“业务层安全握手”流程
	 *   - 负责 GetPublicKey → NegotiateKey 的完整状态管理
	 *   - 替代历史上的 Wegame3dsCode.dll 实现
	 *
	 * 职责边界：
	 *   - 只负责握手流程控制与中间态维护
	 *   - 不直接进行网络通信
	 *   - 不依赖 libcurl / HttpThread / TLS 细节
	 *
	 * 使用方式：
	 *   - 由 LoginClient 在需要 Gunion-WeGame 登录时按需创建
	 *   - 通过 StartGunionHandshake() 或类似入口触发
	 *
	 * 生命周期：
	 *   - 与 LoginClient 实例生命周期绑定
	 *   - LoginClient 构造时可为空
	 *   - 在首次 Gunion-WeGame 登录流程中创建
	 *   - LoginClient 析构时自动释放
	 *
	 * 设计说明：
	 *   - 使用 std::unique_ptr 表明其“唯一拥有者”为 LoginClient
	 *   - 禁止在多个 LoginClient 实例之间共享
	 */
	std::unique_ptr<GunionHandshake> m_gunionHandshake;
};

/*
	┌──────────────┐
	│   Idle       │  （未开始）
	└──────┬───────┘
	│ Start()
	▼
	┌──────────────┐
	│ GetPublicKey │  （请求公钥）
	└──────┬───────┘
	│ 成功
	▼
	┌──────────────┐
	│ Handshake │  （协商 randKey / token）
	└──────┬───────┘
	│ 成功
	▼
	┌──────────────┐
	│  Success     │  （握手完成）
	└──────────────┘

	任一阶段失败 → Failed
*/

//┌───────────────────────────────────────────────┐
//│                  LoginClient                 │
//│---------------------------------------------- - │
//│  1. 参数校验                                  │
//│  2. 流程控制                                  │
//│  3. 构造 HttpRequest                          │
//│  4. 错误处理 / 重试 / Reset                   │
//└───────────────────────────────────────────────┘
//│
//▼
//┌───────────────────────────────────────────────┐
//│           Gunion 协议封装层（接口层）         │
//│---------------------------------------------- - │
//│  BuildXXXEncryptedParams()                    │
//│  CalcXXXSignature()                           │
//│                                               │
//│  每个业务接口一对函数                         │
//└───────────────────────────────────────────────┘
//│
//▼
//┌───────────────────────────────────────────────┐
//│              GunionCrypto 层                  │
//│---------------------------------------------- - │
//│  参数排序                                     │
//│  拼接规则                                     │
//│  3DES 加密（randKey）                         │
//│  Base64 编码                                  │
//│  MD5 签名（lower → md5 → upper）              │
//└───────────────────────────────────────────────┘
//│
//▼
//┌───────────────────────────────────────────────┐
//│            GunionHandshake 会话层             │
//│---------------------------------------------- - │
//│  publicKey 获取                               │
//│  randKey 生成                                 │
//│  token 获取                                   │
//│  会话生命周期管理                             │
//└───────────────────────────────────────────────┘
//│
//▼
//┌───────────────────────────────────────────────┐
//│                  HttpRequest                 │
//│---------------------------------------------- - │
//│  token + encryptedParams + signature          │
//│  发往 Gunion 服务端                           │
//└───────────────────────────────────────────────┘

//启动
//│
//▼
//Handshake
//├── 获取 publicKey
//├── 生成 randKey
//├── 换取 token
//▼
//进入业务阶段
//│
//├── 所有接口统一使用：
//│     randKey + deviceId + deviceIdServer
//│
//▼
//token 过期 / 签名错误
//│
//▼
//Reset Handshake → 重新握手

//┌─────────────────────────┐
//│       登录体系           │
//└─────────────────────────┘
//│
//┌────────────────────────────────────────────────────┐
//│                                                    │
//▼                                                    ▼
//手机号体系                                         个性账号体系
//│                                                    │
//├─ PwdLogin                                         ├─ SpecialPwdLogin
//├─ PicPwdLogin                                      ├─ SpecialPicPwdLogin
//├─ SmsLogin                                         ├─ SpecialSmsSend
//├─ PicSmsSend                                       ├─ SpecialPicSmsSend
//├─ SpecialConfirmSmsSend
//└─ SpecialCheckSmsLogin

//LoginArea          → 上报区服记录
//GetLoginArea       → 获取区服记录

//RealName               → 实名认证
//CheckActivation        → 激活码校验

//AutoLogin
//Logout