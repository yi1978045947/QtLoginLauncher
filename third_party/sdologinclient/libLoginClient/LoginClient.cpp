#include "stdafx.h"
#include "LoginClient.h"
#include "SdoBaseClient.h"
#include "HttpThread.h"
#include "CommonFun.h"
#include <objbase.h>
#include <libUtil/json/json.h>
#include "CurlHttpThread.h"
#include <libUtil/Base64Coding.h>
#include <libUtil/SafeStore_i.h>
#include "Tracer.h"
#include "UrlCode.h"
#include "SafeBase64.h"
#include <openssl/aes.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <set>
#include <algorithm>
#include <regex>
#include <string.h>
#include <time.h>
#include "GunionHandshake.h"
#include "GunionCrypto.h"
#include "ComputerInfo.h"
#include <libUtil/md5.h>
#include <Iphlpapi.h>
#include <windows.h>

#define  IntToString(iStr) #iStr

#define  HOSTNAME3 "hostname3"
#define  HOSTNAME4 "hostname4"
#define  HOSTNAME5 "hostname5"
#define  HOSTNAME6 "hostname6"
#define  HOSTNAME7 "hostname7"
#define  HOSTNAME8 "hostname8"
#define RAND_KEY_LEN 32
#define RSA_PUBLIC_KEY_PREFIX "-----BEGIN PUBLIC KEY-----\n"
#define RSA_PUBLIC_KEY_SUFFIX "\n-----END PUBLIC KEY-----"

#define URL_GET_PUBLIC_KEY "https://mgame.sdo.com/v1/basic/publickey"
#define URL_NEGOTIATE_KEY "https://mgame.sdo.com/v1/basic/handshake"

LoginClient::LoginClient(bool verifyCert)
{

	/**
	 * @brief 初始化旧版 HTTP 线程（基于 WinInet）
	 *
	 * 说明：
	 *   - 历史遗留逻辑一直使用 HttpThread（WinInet）
	 *   - 某些老业务流程仍依赖 WinInet 的行为，因此该线程仍需保留
	 *   - CurlHttpThread 引入后，两者将并行存在，用于灰度替换
	 */

	 // ---------------------------------------------------------------
	 // 初始化新版 Curl HTTP 线程（libcurl + OpenSSL）
	 // ---------------------------------------------------------------
	 /**
	  * @brief 初始化新版 HTTP 线程（基于 libcurl）
	  *
	  * 优势：
	  *   - 不受 Windows TLS 设置限制（不会被 TLS1.0/1.1 勾选影响）
	  *   - 支持完整 HTTPS / TLS1.2+，更稳定
	  *   - 完全兼容旧 HttpThread 的接口（ProcessRequest / Cancel / GetState）
	  *
	  * 注意：
	  *   - 若想完全切换到 curl，只需让业务调用 m_curlHttpThread
	  *   - 若某些业务仍需 WinInet，可以继续使用 m_httpThread
	  *   - 两个线程共存不会冲突
	  */

	  // ---------------------------------------------------------------
	  // 初始化新版 Curl HTTP 线程（libcurl + OpenSSL）
	  // ---------------------------------------------------------------
	  /**
	   * @brief 初始化新版 HTTP 线程（基于 libcurl）
	   *
	   * 优势：
	   *   - 不受 Windows TLS 设置影响（TLS1.0/1.1 勾选不影响程序）
	   *   - 完全使用 OpenSSL，证书链由业务可控
	   *   - 能稳定支持 HTTPS / TLS1.2/1.3
	   *
	   * 注意：
	   *   - WinInet 的旧 m_httpThread 依然保留，用于兼容极老流程
	   *   - 业务逐步迁移到 curl 线程
	   */
	m_httpThread = new CurlHttpThread(this);

	/**
	 * @brief 设置 curl 是否启用证书校验
	 *
	 * verifyCert=true  → 严格校验证书（正式环境）
	 * verifyCert=false → 不校验证书（调试抓包，如 HttpAnalyzer/Fiddler）
	 */
	m_verifyLoginClientCert = verifyCert;   // ← 同步记录 LoginClient 自己的状态
	m_httpThread->SetVerifyCert(verifyCert);

	m_enableGunionPreInit = false; // ← 判断是否走gunion逻辑

	// ---------------------------------------------------------------
	// 初始化请求超时设置（毫秒）
	// ---------------------------------------------------------------
	m_requestProcess.m_timeout = 10000;
	m_requestProcess.m_timeout2 = 10000;

	// ---------------------------------------------------------------
	// 初始化所有登录流程相关回调（全部置空，按需由上层业务设置）
	// ---------------------------------------------------------------
	m_checkCodeLoginCallback = nullptr;                 ///< 图形验证码登录回调
	m_dynamicLoginCallback = nullptr;                   ///< 动态密码登录回调
	m_fcmLoginCallback = nullptr;                       ///< 防沉迷验证登录回调
	m_loginResultCallback = nullptr;                    ///< 通用登录结果回调
	m_phoneCheckCodeCallback = nullptr;                 ///< 手机短信验证码发送回调
	m_checkAccountCallback = nullptr;                   ///< 检查账号状态回调
	m_getCodeKeyCallback = nullptr;                     ///< 获取二维码/Key 回调
	m_loginStatusCallback = nullptr;                    ///< 轮询登录状态回调
	m_extendLoginStateCallback = nullptr;               ///< 扩展状态上报回调
	m_getPushMessageStatusCallback = nullptr;           ///< 推送登录状态查询回调
	m_getAccountInfoCallback = nullptr;                 ///< 获取账号信息回调
	m_getLoginHistoryCallback = nullptr;                ///< 获取登录记录回调
	m_getSessionIdStatesCallBack = nullptr;             ///< 查询 SessionId 状态回调
	m_callbackKickoffAccountVerify = nullptr;           ///< 异地登录短信验证回调
	m_callbackKickoffAccountResult = nullptr;           ///< 异地登录确认结果回调
	m_getLoginUserInfoCallback = nullptr;               ///< 获取用户资料回调
	m_setLoginUserInfoCallback = nullptr;               ///< 设置用户资料回调
	m_getLoginAreaInfoCallback = nullptr;               ///< 获取登录大区信息回调
	m_UserPrivacyConfigCallback = nullptr;              ///< 旧版用户隐私配置回调
	m_FaceVerifyInitCallback = nullptr;                 ///< 人脸识别初始化回调
	m_GetFaceCodeResultCallback = nullptr;              ///< 人脸识别验证结果回调
	m_SendActionCallback = nullptr;                     ///< 人脸识别动作提交回调
	m_CreateAuchCodeCallback = nullptr;                 ///< 授权码创建回调
	///////////////////////////////////////////////////////////////////////////////
	// GunionWegame 回调初始化
	///////////////////////////////////////////////////////////////////////////////
	m_CreateGunionWegameInitCallback = nullptr;  ///< GunionWegame 初始化回调
	m_CreateGunionWegameLoginCallback = nullptr;  ///< GunionWegame 登录结果回调
	m_CreateGunionWegameSmsSendCallback = nullptr;  ///< GunionWegame 短信发送回调
	m_CreateGunionWegamePicSmsSendCallback = nullptr;  ///< GunionWegame 带图验短信发送回调
	m_CreateGunionWegameThirdAccountBindPhoneCallback = nullptr;  ///< GunionWegame 三方账号绑定手机回调
	m_CreateGunionWegameFillRealinfoCallback = nullptr;  ///< GunionWegame 实名补填回调
	m_CreateGunionWegameGetTicketCallback = nullptr;  ///< GunionWegame 获取登录票据回调
	m_CreateGunionWegameGetPayTicketCallback = nullptr;  ///< GunionWegame 获取支付票据回调

	// ---------------------------
	// 新版隐私协议
	// ---------------------------
	m_NewVersionUserPrivacyConfigCallback = nullptr;    ///< 新版隐私协议配置回调

	// ---------------------------
	// 登录器配置获取
	// ---------------------------
	m_GetClientConfigCallback = nullptr;                ///< 登录器客户端配置回调
	m_GetClientConfigCallback = nullptr;

	// ---------------------------
	// 新版下行短信登录系列回调
	// ---------------------------
	m_GoDownConfigInitCallback = nullptr;               ///< 初始化下行短信参数
	m_GoDownSendSmsCheckCodeCallback = nullptr;         ///< 发送短信前校验（带图验）
	m_GoDownconfirmSendSmsCallback = nullptr;           ///< 确认发送短信
	m_GoDownconfirmLoginCallback = nullptr;             ///< 确认短信后登录

	// ---------------------------
	// 快速登录
	// ---------------------------
	m_FastLoginLoginCallback = nullptr;                 ///< 快速登录结果回调
	m_GetTicketLoginCallback = nullptr;                 ///< 获取票据（ticket）回调
	m_BindSecurityPhoneLoginCallback = nullptr;         ///< 未绑定安全手机提示回调
	m_FastLoginActiveDeleteAccountLoginCallback = nullptr; ///< 快速登录主动删除账号
	m_CheckAccountTypeLoginCallback = nullptr;          ///< 检查快速列表账号类型回调

	// ---------------------------
	// 行为验证码
	// ---------------------------
	m_CalculateCallback = nullptr;                      ///< 行为验证计算回调

	// ---------------------------
	// 广告模块
	// ---------------------------
	m_InitAdvCallback = nullptr;                        ///< 广告系统初始化回调

	// ---------------------------
	// 监护人相关
	// ---------------------------
	m_GuardDianSendSmsCallback = nullptr;               ///< 监护人短信发送回调
	m_GuardDianSendSmsCheckCodeCallback = nullptr;      ///< 带图验短信发送回调
	m_GuardDianConfrimSendSmsResultCallback = nullptr;  ///< 监护人短信确认提交
	m_GuardDianCallback = nullptr;                      ///< 获取监护人信息回调

	// ---------------------------
	// 人身核验相关
	// ---------------------------
	m_FaceRecognitionUrlCallback = nullptr;               ///< 人脸打开界面Url回调
	m_QueryFaceVerifyTicketStatusCallback = nullptr;      ///< 查询人脸识别票据状态回调
	m_HolderRegistrationUrlCallback = nullptr;            ///< 人身核验信息收集界面Url回调

	// ---------------------------
	// 登录器激活码
	// ---------------------------
	m_ActiveLoginCallback = nullptr;
	m_ShowActiveLoginDlgCallback = nullptr;

	///////////////////////////////////////////////////////////////////////////////
	// 隐私协议相关
	///////////////////////////////////////////////////////////////////////////////
	m_CreateDoPirvateCallback = nullptr;                    ///< 隐私协议跳转回调

	///////////////////////////////////////////////////////////////////////////////
	// 短信验证码体系/短信、图验短信在GunionWegame的接口复用
	///////////////////////////////////////////////////////////////////////////////
	m_CreateCheckSmsLoginCallback = nullptr;                ///< 短信验证码登录回调

	///////////////////////////////////////////////////////////////////////////////
	// 实名认证 / 自动登录 / 登出 / 票据
	///////////////////////////////////////////////////////////////////////////////
	m_CreateCheckRealNameCallback = nullptr;                ///< 实名认证回调
	m_CreateCheckAutoLoginCallback = nullptr;               ///< 自动登录回调
	m_CreateCheckLogoutCallback = nullptr;                  ///< 登出回调

	///////////////////////////////////////////////////////////////////////////////
	// 普通账号密码登录体系
	///////////////////////////////////////////////////////////////////////////////
	m_CreateCheckPwdLoginCallback = nullptr;                ///< 密码登录回调
	m_CreateCheckPicPwdLoginCallback = nullptr;             ///< 图验密码登录回调

	///////////////////////////////////////////////////////////////////////////////
	// 第三方登录体系
	///////////////////////////////////////////////////////////////////////////////
	m_CreateCheckThirdLoginCallback = nullptr;              ///< 第三方登录回调（QQ/WX/WB/Wegame）

	///////////////////////////////////////////////////////////////////////////////
	// 个性账号登录体系
	///////////////////////////////////////////////////////////////////////////////
	m_CreateCheckSpecialPwdLoginCallback = nullptr;         ///< 个性账号静密登录回调
	m_CreateCheckSpecialPicPwdLoginCallback = nullptr;      ///< 个性账号图验静密登录回调
	m_CreateDoSpecialSmsSendCallback = nullptr;             ///< 个性账号短信发送回调
	m_CreateCheckSpecialPicSmsSendCallback = nullptr;       ///< 个性账号带图验短信发送回调
	m_CreateDoSpecialConfirmSmsSendCallback = nullptr;      ///< 个性账号安全手机确认短信发送回调
	m_CreateDoSpecialCheckSmsLoginCallback = nullptr;       ///< 个性账号短信登录回调

	///////////////////////////////////////////////////////////////////////////////
	// 区服记录体系
	///////////////////////////////////////////////////////////////////////////////
	m_CreateCheckSetLoginAreaLoginCallback = nullptr;       ///< 登录区服记录回调
	m_CreateCheckGetLoginAreaLoginCallback = nullptr;       ///< 获取区服配置回调

	///////////////////////////////////////////////////////////////////////////////
	// 支付体系
	///////////////////////////////////////////////////////////////////////////////
	m_CreateDoPayEntranceCallback = nullptr;                ///< 统一收银台支付回调
	m_CreateDoCheckPayOrderStatuseCallback = nullptr;       ///< 支付订单状态查询回调
	m_CreateDoCheckActivationCallback = nullptr;            ///< 激活码校验回调

	///////////////////////////////////////////////////////////////////////////////
	// 协议与合规体系
	///////////////////////////////////////////////////////////////////////////////
	m_CreateCheckGameAgreementUrlContentCallback = nullptr; ///< 获取服务协议/隐私协议内容回调

	///////////////////////////////////////////////////////////////////////////////
	// 监护人认证
	///////////////////////////////////////////////////////////////////////////////
	m_CreateCheckSetTutorInfoCallback = nullptr;            ///< 监护人信息收集回调

	///////////////////////////////////////////////////////////////////////////////
	// Wegame 登录体系
	///////////////////////////////////////////////////////////////////////////////
	m_CreateCheckGunionWegameLoginCallback = nullptr;       ///< Wegame 登录结果回调

	///////////////////////////////////////////////////////////////////////////////
	// Gunion 握手体系
	///////////////////////////////////////////////////////////////////////////////
	m_CreateCheckGunionGetPublicKeyCallback = nullptr;       ///< Gunion 获取公钥结果回调
	m_CreateCheckGunionHandShakeCallback = nullptr;       ///< Gunion 握手结果回调

	//---------------------------------------------------------------------
	// 请求码 → 回调函数映射表
	// 说明：所有网络请求返回后，统一通过 m_mapResponseFunc 决定
	//       调用 LoginClient 的哪个 ProcessXXXResponse()
	//---------------------------------------------------------------------

	// =====================
	//  登录动态 Key（第一步）
	// =====================
	m_mapResponseFunc[REQ_GetDynamicKey] = &LoginClient::ProcessGetDynamicKeyResponse;

	// =====================
	//  标准登录流程
	// =====================
	m_mapResponseFunc[REQ_StaticLogin] = &LoginClient::ProcessLoginResponse;
	m_mapResponseFunc[REQ_AutoLogin] = &LoginClient::ProcessLoginResponse;
	m_mapResponseFunc[REQ_CheckCodeLogin] = &LoginClient::ProcessLoginResponse;
	m_mapResponseFunc[REQ_FcmLogin] = &LoginClient::ProcessLoginResponse;     // 防沉迷登录
	m_mapResponseFunc[REQ_SsoLogin] = &LoginClient::ProcessLoginResponse;     // SSO 登录

	// =====================
	//  各渠道联合登录（WeGame / QQGame / Lenovo / 云游戏 等）
	// =====================
	m_mapResponseFunc[REQ_WeGameLogin] = &LoginClient::ProcessLoginResponse;
	m_mapResponseFunc[REQ_QQGameLogin] = &LoginClient::ProcessLoginResponse;
	m_mapResponseFunc[REQ_LenovoLogin] = &LoginClient::ProcessLoginResponse;
	m_mapResponseFunc[REQ_CloudGameLogin] = &LoginClient::ProcessLoginResponse;

	// =====================
	//  短信登录系列
	// =====================
	m_mapResponseFunc[REQ_SmsLogin] = &LoginClient::ProcessLoginResponse;
	m_mapResponseFunc[REQ_FastLogin] = &LoginClient::ProcessLoginResponse;            // 快速登录
	m_mapResponseFunc[REQ_PhoneCheckCodeLogin] = &LoginClient::ProcessLoginResponse;  // 手机验证码登录
	m_mapResponseFunc[REQ_CodeKeyLogin] = &LoginClient::ProcessLoginResponse;         // 图码验证码登录
	m_mapResponseFunc[REQ_DynamicLogin] = &LoginClient::ProcessLoginResponse;         // 动态密码
	m_mapResponseFunc[REQ_DynamicLoginVoice] = &LoginClient::ProcessLoginResponse;

	// =====================
	//  账号类型 / 验证码发送 / 扫码登录
	// =====================
	m_mapResponseFunc[REQ_CheckAccountType] = &LoginClient::ProcessCheckAccoutTypeResponse;
	m_mapResponseFunc[REQ_SendPhoneCheckCode] = &LoginClient::ProcessSendPhoneCheckCodeResponse;
	m_mapResponseFunc[REQ_GetQrCode] = &LoginClient::ProcessGetQrCodeResponse;

	// =====================
	//  登录状态轮询 / 扩展状态 / 退出登录
	// =====================
	m_mapResponseFunc[REQ_GetLoginStatus] = &LoginClient::ProcessGetLoginStatusResponse;
	m_mapResponseFunc[REQ_ExtendLoginState] = &LoginClient::ProcessExtendLoginStateResponse;
	m_mapResponseFunc[REQ_Logout] = &LoginClient::ProcessLogoutResponse;

	// =====================
	//  一键登录消息系列
	// =====================
	m_mapResponseFunc[REQ_PushMessageLogin] = &LoginClient::ProcessLoginResponse;
	m_mapResponseFunc[REQ_SendPushMessage] = &LoginClient::ProcessSendPushMessageResponse;
	m_mapResponseFunc[REQ_SendPushMessageVerifyCheckCode] = &LoginClient::ProcessSendPushMessageResponse;
	m_mapResponseFunc[REQ_GetPushMessageStatus] = &LoginClient::ProcessGetPushMessageStatusResponse;

	// =====================
	//  用户 / 账号相关查询
	// =====================
	m_mapResponseFunc[REQ_GetAccountInfo] = &LoginClient::ProcessGetAccountInfoResponse;
	m_mapResponseFunc[REQ_GetLoginHistory] = &LoginClient::ProcessGetLoginHistoryResponse;
	m_mapResponseFunc[REQ_GetLoginUserInfo] = &LoginClient::ProcessGetLoginUserInfoResponse;
	m_mapResponseFunc[REQ_SetLoginUserInfo] = &LoginClient::ProcessSetLoginUserInfoResponse;
	m_mapResponseFunc[REQ_GetLoginAreaInfo] = &LoginClient::ProcessGetLoginAreaInfoResponse;

	// =====================
	//  注册即登录
	// =====================
	m_mapResponseFunc[REQ_RltLogin] = &LoginClient::ProcessLoginResponse;
	m_mapResponseFunc[REQ_RltLoginKeepLogin] = &LoginClient::ProcessLoginResponse;

	// =====================
	//  公钥 / 加密相关
	// =====================
	m_mapResponseFunc[REQ_GetPublicKey] = &LoginClient::ProcessGetPublicKeyResponse;
	m_mapResponseFunc[REQ_GetClientVKey] = &LoginClient::ProcessGetClientVKeyResponse;

	// =====================
	//  运营活动类广告投放
	// =====================
	m_mapResponseFunc[REQ_GetPromotionInfo] = &LoginClient::ProcessGetPromotionInfoResponse;
	m_mapResponseFunc[REQ_PromotionInfoConfirm] = &LoginClient::ProcessPromotionInfoConfirmResponse;

	// =====================
	//  账号组登录流程
	// =====================
	m_mapResponseFunc[REQ_SendUserAccount] = &LoginClient::ProcessSendUserAccountResponse;
	m_mapResponseFunc[REQ_GetAccountGroup] = &LoginClient::ProcessGetAccountGroupResponse;
	m_mapResponseFunc[REQ_AccountGroupLogin] = &LoginClient::ProcessLoginResponse;

	// =====================
	//  三方平台轮询登录
	// =====================
	m_mapResponseFunc[REQ_ThirdPartyPollingLogin] = &LoginClient::ProcessLoginResponse;
	m_mapResponseFunc[REQ_ThirdPartyLogin] = &LoginClient::ProcessLoginResponse;

	// =====================
	//  异地踢下线流程
	// =====================
	m_mapResponseFunc[REQ_CancelPushMessageLogin] = &LoginClient::ProcessCancelPushMessageResponse;
	m_mapResponseFunc[REQ_GetSessionIdStates] = &LoginClient::ProcessGetSessionIdStateResponse;
	m_mapResponseFunc[REQ_KickoffVerify] = &LoginClient::ProcessKickoffVerifyResponse;
	m_mapResponseFunc[REQ_KickOffVerifyCheckCode] = &LoginClient::ProcessKickoffVerifyResponse;
	m_mapResponseFunc[REQ_KickoffAccount] = &LoginClient::ProcessKickoffResultResponse;

	// =====================
	//  营运短信通道（咪咕 / 普通短信）
	// =====================
	m_mapResponseFunc[REQ_SendMiGuSms] = &LoginClient::ProcessSendMiGuSmsRequestResponse;
	m_mapResponseFunc[REQ_SendSms] = &LoginClient::ProcessSendSmsRequestResponse;
	m_mapResponseFunc[REQ_CheckCodeToSendSms] = &LoginClient::ProcessCheckCodeToSendSmsRequestResponse;

	// =====================
	//  隐私协议 / 人脸识别
	// =====================
	m_mapResponseFunc[REQ_UserPrivacyConfig] = &LoginClient::ProcessUserPrivacyConfigRequestResponse;
	m_mapResponseFunc[REQ_FaceVerifyInit] = &LoginClient::ProcessFaceVerifyInitRequestResponse;
	m_mapResponseFunc[REQ_FaceCodeResult] = &LoginClient::ProcessGetFaceCodeResultRequestResponse;
	m_mapResponseFunc[REQ_FaceSendAction] = &LoginClient::ProcessSendActionRequestResponse;

	// =====================
	//  登录票据（跨游戏复用）
	// =====================
	m_mapResponseFunc[REQ_GetTicket] = &LoginClient::ProcessGetTicketRequestResponse;

	// =====================
	//  支付 / 订单 系列
	// =====================
	m_mapResponseFunc[REQ_CreateWeGameOrder] = &LoginClient::ProcessCreateWeGameOrderRequestResponse;
	m_mapResponseFunc[REQ_WeGameStatus] = &LoginClient::ProcessWeGameStatusRequestResponse;
	m_mapResponseFunc[REQ_CreateLxOrder] = &LoginClient::ProcessCreateLxOrderRequestResponse;
	m_mapResponseFunc[REQ_CreateQQGameOrder] = &LoginClient::ProcessCreateQQGameOrderRequestResponse;
	m_mapResponseFunc[REQ_QQGameIsLogin] = &LoginClient::ProcessQQGameIsLoginRequestResponse;
	m_mapResponseFunc[REQ_CreateSteamChannelOrder] = &LoginClient::ProcessCreateSteamChannelOrderRequestResponse;

	// =====================
	//  传奇4（独立业务）
	// =====================
	m_mapResponseFunc[REQ_CreateCQ4Order] = &LoginClient::ProcessCreateCQ4OrderRequestResponse;
	m_mapResponseFunc[REQ_CreateCQ4Query] = &LoginClient::ProcessCreateCQ4QueryRequestResponse;

	// =====================
	//  登录皮肤
	// =====================
	m_mapResponseFunc[REQ_ThirdLoginSkin] = &LoginClient::ProcessGetThirdLoginSkinRequestResponse;

	// =====================
	//  授权码（游戏盒子）
	// =====================
	m_mapResponseFunc[REQ_CheckAuthCode] = &LoginClient::ProcessCreateAuthCodeResponse;
	m_mapResponseFunc[REQ_SsoLoginAuthCode] = &LoginClient::ProcessLoginResponse;

	// =====================
	//  下行短信登录（新版）
	// =====================
	m_mapResponseFunc[REQ_GetClientConfig] = &LoginClient::ProcessCreateGetClientConfigRequestResponse;
	m_mapResponseFunc[REQ_NewVersionGoDownConfigInit] = &LoginClient::ProcessCreateGetGetGoDownConfigInitRequestResponse;
	m_mapResponseFunc[REQ_NewVersionGoDownSendSmsCheckCode] = &LoginClient::ProcessCreateGetGoDownSendSmsCheckCodeRequestResponse;
	m_mapResponseFunc[REQ_NewVersionGoDownconfirmSendSms] = &LoginClient::ProcessCreateGetGoDownconfirmSendSmsRequestResponse;
	m_mapResponseFunc[REQ_NewVersionGoDownconfirmLogin] = &LoginClient::ProcessCreateGetGoDownconfirmLoginRequestResponse;
	m_mapResponseFunc[REQ_NewVersionFastLogin] = &LoginClient::ProcessCreateGetFastLoginLoginRequestResponse;
	m_mapResponseFunc[REQ_NewVersionGetTicket] = &LoginClient::ProcessCreateGetTicketLoginRequestResponse;
	m_mapResponseFunc[REQ_FastLoginActiveDeleteAccount] = &LoginClient::ProcessCreateFastLoginActiveDeleteAccountRequestResponse;
	m_mapResponseFunc[REQ_CheckLoginTypeAccount] = &LoginClient::ProcessCreateCheckAccountLoginTypeRequestResponse;

	// =====================
	//  监护人相关
	// =====================
	m_mapResponseFunc[REQ_GuardDianSendSms] = &LoginClient::ProcessCreateGuardDianSendSmsRequestResponse;
	m_mapResponseFunc[REQ_GuardDianSendSmsCheckCode] = &LoginClient::ProcessCreateGuardDianSendSmsCheckCodeRequestResponse;
	m_mapResponseFunc[REQ_GuardDianConfrimSendSmsResult] = &LoginClient::ProcessCreateGuardDianConfrimSendSmsResultRequestResponse;

	// =====================
	//  激活码相关
	// =====================
	m_mapResponseFunc[REQ_GetActiveCodeLoginResult] = &LoginClient::ProcessCreateActiveCodeLoginResultRequestResponse;

	// =====================
	//  计算相关接口（用于某些环境/设备参数计算，例如加密参数、密钥校验等）
	// =====================
	m_mapResponseFunc[REQ_CheckCalculate] = &LoginClient::ProcessCreateCheckCalculateRequestResponse;

	// =====================
	//  广告系统初始化（拉取广告配置 / 展示策略 / 投放参数）
	// =====================
	m_mapResponseFunc[REQ_CheckInitAdv] = &LoginClient::ProcessCreateCheckInitAdvRequestResponse;

	// =====================
	//  卫士通 客户端初始化（卫士通 游戏需要的启动参数 / 会话数据）
	// =====================
	m_mapResponseFunc[REQ_UeInitClient] = &LoginClient::ProcessUeInitClientRequestResponse;

	// =====================
	//  QQ / WX / WB 第三方快速登录（走 SDO 三方绑定流程）
	// =====================
	m_mapResponseFunc[REQ_ThirdLogin] = &LoginClient::ProcessLoginResponse;

	// =====================
	//  人身核验相关流程
	// =====================
	m_mapResponseFunc[REQ_GetFaceRecognitionUrl] = &LoginClient::ProcessCreateGetFaceRecognitionUrlRequestResponse;
	m_mapResponseFunc[REQ_QueryFaceVerifyTicketStatus] = &LoginClient::ProcessCreateQueryFaceVerifyTicketStatusRequestResponse;
	m_mapResponseFunc[REQ_GetFaceHolderRegistrationUrl] = &LoginClient::ProcessCreateGetFaceHolderRegistrationUrlRequestResponse;

	// =====================
	//  Gunion-WeGame 登录相关接口
	//  说明：
	//    - 属于 Gunion-WeGame 登录体系的业务接口
	//    - 前置条件：已完成 Gunion 安全握手（GetPublicKey / NegotiateKey）
	//    - 涵盖登录、验证码、实名、绑定、票据等完整登录流程
	// =====================

	// Gunion 获取 RSA 公钥（握手第一步）
	//   - 服务端下发 RSA 公钥
	//   - 客户端后续使用该公钥加密 randKey
	m_mapResponseFunc[REQ_GunionGetPublicKey] = &LoginClient::ProcessCreateGunionGetPublicKeyResponse;

	// Gunion 握手 / 会话密钥协商（握手第二步）
	//   - 使用 RSA 公钥加密 randKey
	//   - 获取会话 token
	m_mapResponseFunc[REQ_GunionHandshake] = &LoginClient::ProcessCreateGunionHandshakeResponse;

	// Gunion-WeGame 初始化
	//   - 拉取登录所需的基础配置
	//   - 初始化 Gunion 会话相关状态
	m_mapResponseFunc[REQ_GunionWegameInit] = &LoginClient::ProcessCreateGunionWegameInitResponse;

	// Gunion-WeGame 登录
	//   - 使用账号 / 第三方 / 快速登录等方式进行登录
	m_mapResponseFunc[REQ_GunionWegameLogin] = &LoginClient::ProcessGunionWegameLoginResponse;

	// Gunion-WeGame 短信验证码发送
	//   - 普通短信验证码下发
	m_mapResponseFunc[REQ_GunionWegameSmsSend] = &LoginClient::ProcessCreateGunionWegameSmsSendResponse;

	// Gunion-WeGame 图形验证码 + 短信发送
	//   - 需要图形验证码校验的短信发送流程
	m_mapResponseFunc[REQ_GunionWegamePicSmsSend] = &LoginClient::ProcessCreateGunionWegamePicSmsSendResponse;

	// Gunion-WeGame 第三方账号绑定手机号
	//   - QQ / WX / WB 等第三方账号绑定手机
	m_mapResponseFunc[REQ_GunionWegameThirdAccountBindPhone] = &LoginClient::ProcessCreateGunionWegameThirdAccountBindPhoneResponse;

	// Gunion-WeGame 实名信息补填
	//   - 未实名或信息不完整时引导用户补填
	m_mapResponseFunc[REQ_GunionWegameFillRealinfo] = &LoginClient::ProcessCreateGunionWegameFillRealinfoResponse;

	// Gunion-WeGame 获取登录票据
	//   - 登录成功后获取业务使用的登录票据
	m_mapResponseFunc[REQ_GunionWegameGetTicket] = &LoginClient::ProcessCreateGunionWegameGetTicketResponse;

	// Gunion-WeGame 获取支付票据
	//   - 用于支付、充值等场景的专用票据
	m_mapResponseFunc[REQ_GunionWegameGetPayTicket] =
		&LoginClient::ProcessCreateGunionWegameGetPayTicketResponse;

	// ========================
	// Gunion 协议相关
	// ========================
	// Gunion - 获取隐私协议URL
	//   - 登录界面点击隐私协议时获取协议地址
	m_mapResponseFunc[REQ_GunionGetPrivateUrl] = &LoginClient::ProcessCreateGunionDoPrivateResponse;

	// Gunion - 获取游戏协议内容
	//   - 获取用户协议 / 游戏服务协议文本
	m_mapResponseFunc[REQ_GunionGetAgreementContent] = &LoginClient::ProcessCreateGunionGetGameAgreementUrlContentResponse;

	// ========================
	// Gunion 登录相关
	// ========================

	// Gunion 短信验证码发送
	//   - 普通短信验证码下发
	m_mapResponseFunc[REQ_GunionSmsSend] = &LoginClient::ProcessCreateGunionSmsSendResponse;

	// Gunion- 图形验证码 + 短信发送
	//   - 需要图形验证码校验的短信发送流程
	m_mapResponseFunc[REQ_GunionPicSmsSend] = &LoginClient::ProcessCreateGunionPicSmsSendResponse;

	// Gunion - offical账号短信验证码登录
	//   - 用户输入手机号 + 短信验证码登录
	m_mapResponseFunc[REQ_GunionSmsLogin] = &LoginClient::ProcessCreateGunionCheckSmsLoginResponse;

	// Gunion - officalpt账号短信验证码发送
	//   - 风控流程下发送短信验证码
	m_mapResponseFunc[REQ_GunionSpecialSmsSend] = &LoginClient::ProcessCreateGunionDoSpecialSmsSendResponse;

	// Gunion - officalpt账号短信图形验证码校验
	//   - 发送短信前需要校验图形验证码
	m_mapResponseFunc[REQ_GunionSpecialPicSmsSend] = &LoginClient::ProcessCreateGunionCheckSpeicalPicSmsSendResponse;

	// Gunion - officalpt账号短信发送确认
	//   - 风控流程中的短信发送确认
	m_mapResponseFunc[REQ_GunionSpecialConfirmSmsSend] = &LoginClient::ProcessCreateGunionCheckSpecialConfirmSmsSendLoginResponse;

	// Gunion - officalpt账号短信验证码登录
	//   - 风控流程中的短信验证码登录
	m_mapResponseFunc[REQ_GunionSpecialSmsLogin] = &LoginClient::ProcessCreateGunionCheckSpecialSmsLoginResponse;

	// Gunion - offical账号账号密码登录
	//   - 用户输入账号密码进行登录
	m_mapResponseFunc[REQ_GunionPwdLogin] = &LoginClient::ProcessCreateGunionCheckPwdLoginResponse;

	// Gunion - offical账号图形验证码密码登录
	//   - 密码登录触发图形验证码验证
	m_mapResponseFunc[REQ_GunionPicPwdLogin] = &LoginClient::ProcessCreateGunionCheckPicPwdLoginResponse;

	// Gunion - officalpt账号密码登录
	//   - 风控流程中的密码登录
	m_mapResponseFunc[REQ_GunionSpecialPwdLogin] = &LoginClient::ProcessCreateGunionCheckSpecialPwdLoginResponse;

	// Gunion - officalpt账号图形验证码密码登录
	//   - 风控密码登录 + 图形验证码校验
	m_mapResponseFunc[REQ_GunionSpecialPicPwdLogin] = &LoginClient::ProcessCreateGunionCheckSpecialPicPwdLoginResponse;

	// Gunion - 第三方渠道登录
	//   - 使用第三方平台账号登录（如 WeGame / Steam 等）
	m_mapResponseFunc[REQ_GunionThirdLogin] = &LoginClient::ProcessCreateGunionCheckThirdLoginResponse;

	// Gunion - 自动登录
	//   - 使用本地缓存Token自动登录
	m_mapResponseFunc[REQ_GunionAutoLogin] = &LoginClient::ProcessCreateGunionCheckAutoLoginResponse;

	// Gunion - 用户退出登录
	//   - 清理登录状态与Token
	m_mapResponseFunc[REQ_GunionLogout] = &LoginClient::ProcessCreateGunionCheckLogoutResponse;

	// Gunion - 实名认证校验
	//   - 登录成功后检查实名状态
	m_mapResponseFunc[REQ_GunionRealName] = &LoginClient::ProcessCreateGunionCheckRealNameResponse;

	// ========================
	// Gunion 区服相关 
	// ========================
	// Gunion - 获取登录区服
	//   - 查询用户最近登录的游戏区服
	m_mapResponseFunc[REQ_GunionGetLoginArea] = &LoginClient::ProcessCreateGunionCheckGetLoginAreaLoginResponse;

	// Gunion - 设置登录区服
	//   - 保存用户选择的区服
	m_mapResponseFunc[REQ_GunionSetLoginArea] = &LoginClient::ProcessCreateGunionCheckSetLoginAreaLoginResponse;

	// ========================
	// Gunion 支付相关
	// ========================
	// Gunion - 支付入口创建
	//   - 创建支付入口并返回支付页面
	m_mapResponseFunc[REQ_GunionPayEntrance] = &LoginClient::ProcessCreateGunionPayEntranceResponse;

	// Gunion - 查询支付订单状态
	//   - 查询订单是否支付成功
	m_mapResponseFunc[REQ_GunionPayOrderStatus] = &LoginClient::ProcessCreateGunionCheckPayOrderStatusResponse;

	// ========================
	// Gunion 激活码 
	// ========================
	// Gunion - 激活码兑换
	//   - 使用激活码兑换游戏奖励
	m_mapResponseFunc[REQ_GunionActivation] = &LoginClient::ProcessCreateGunionCheckActivationResponse;

	// ========================
	// Gunion 监护人 
	// ========================
	// Gunion - 未成年人监护信息设置
	//   - 填写或更新监护人信息
	m_mapResponseFunc[REQ_GunionSetTutorLogin] = &LoginClient::ProcessCreateGunionCheckSetTutorInfoResponse;

	// ========================
	// Gunion WeGame 渠道
	// ========================
	// Gunion-WeGame 渠道登录
	//   - 通过 WeGame SDK ticket 登录游戏
	m_mapResponseFunc[REQ_GunionWegameChannelLogin] = &LoginClient::ProcessCreateGunionCheckGunionWegameLoginResponse;

	// GunionLogin 获取 RSA 公钥（握手第一步）
	//   - 服务端下发 RSA 公钥
	//   - 客户端后续使用该公钥加密 randKey
	m_mapResponseFunc[REQ_GunionLoginGetPublicKey] = &LoginClient::ProcessCreateGunionLoginGetPublicKeyResponse;

	// GunionLogin 握手 / 会话密钥协商（握手第二步）
	//   - 使用 RSA 公钥加密 randKey
	//   - 获取会话 token
	m_mapResponseFunc[REQ_GunionLoginHandshake] = &LoginClient::ProcessCreateGunionLoginHandshakeResponse;

	// ----------------------------------------------
	// 初始化 Gunion-WeGame 握手状态机
	// ----------------------------------------------
	m_gunionHandshake = std::make_unique<GunionHandshake>(this);

	//---------------------------------------------------------------------
	// 构造函数末尾：事件初始化
	//---------------------------------------------------------------------
	m_waitEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr); ///< 用于线程等待的事件
	m_isPushMessageCheckCode = false;                            ///< 推送验证码状态标记
}

LoginClient::~LoginClient()
{
	//Logout();
	delete m_httpThread;

	if (m_waitEvent)
	{
		CloseHandle(m_waitEvent);
		m_waitEvent = nullptr;
	}
}

void LoginClient::ParseHttpAddr(const string& serverAddr, string& hostName, int& port)
{
	if (serverAddr.find("https") != string::npos)
		port = INTERNET_DEFAULT_HTTPS_PORT;
	else
		port = INTERNET_DEFAULT_HTTP_PORT;
	size_t pos = serverAddr.find("://");
	if (pos == string::npos)
		pos = 0;
	else
		pos += 3;
	hostName = serverAddr.substr(pos);
	pos = hostName.find(":");
	if (pos != string::npos)
	{
		port = atoi(hostName.substr(pos + 1).c_str());
		hostName = hostName.substr(0, pos);
	}
}

std::string LoginClient::BuildClientEnvironmentDeviceId()
{
	TRACET();

	char szMAC[128] = { 0 };
	int nMacLen = sizeof(szMAC);

	char szDiskSN[128] = { 0 };
	int nDiskSNLen = sizeof(szDiskSN);

	char szCpuInfo[128] = { 0 };
	int nCpuInfoLen = sizeof(szCpuInfo);

	char szSignatureMd5[1024] = { 0 };

	char szMacMd5[200] = { 0 };
	char szCpuInfoMd5[200] = { 0 };
	char szDiskSNMd5[200] = { 0 };

	// ------------------------------------------------------------
	// MAC 地址（优先，使用 Netbios）
	// ------------------------------------------------------------
	if (CComputerInfo::GetMacAddress(szMAC, nMacLen, true))
	{
		md5_string((const unsigned char*)szMAC, szMacMd5, nMacLen);
		strcat_s(szSignatureMd5, szMacMd5);
		strcat_s(szSignatureMd5, ":");
	}

	// ------------------------------------------------------------
	// CPU 信息
	// ------------------------------------------------------------
	int nCpuInfoSize = sizeof(szCpuInfo);
	int nNumProcessor = 0;
	if (CComputerInfo::GetCpuInfo(szCpuInfo, nCpuInfoSize, nNumProcessor))
	{
		md5_string((const unsigned char*)szCpuInfo, szCpuInfoMd5, nCpuInfoSize);
		strcat_s(szSignatureMd5, szCpuInfoMd5);
		strcat_s(szSignatureMd5, ":");
	}

	// ------------------------------------------------------------
	// 磁盘序列号
	// ------------------------------------------------------------
	if (CComputerInfo::GetDiskSN(szDiskSN, nDiskSNLen))
	{
		// 原实现是 nDiskSNLen - 1，这里保持一致
		md5_string((const unsigned char*)szDiskSN, szDiskSNMd5, nDiskSNLen - 1);
		strcat_s(szSignatureMd5, szDiskSNMd5);
	}

#ifdef _DEBUG
	// 调试输出（保持原行为）
	std::wstring wSign = ANSIToUnicode(szSignatureMd5);
	OutputDebugString(wSign.c_str());
#endif

	return std::string(szSignatureMd5);
}

std::string LoginClient::GetMacAddress()
{
	// --------------------------------------------------------------------
	// 说明：
	//  本函数用于获取客户端的物理网卡 MAC 地址，返回格式为：
	//      "XX-XX-XX-XX-XX-XX"
	//
	//  注意：在以下场景下，函数可能返回空字符串：
	//
	//   1. 机器当前未联网
	//      - 网线未插、Wi-Fi 未连接
	//      - 所有网卡均无有效 IP 地址
	//
	//   2. 仅存在虚拟 / VPN 网卡
	//      - VPN（OpenVPN / AnyConnect 等）
	//      - 虚拟机网卡（VMware / Hyper-V / WSL）
	//      - TAP / TUN / vEthernet 接口
	//
	//   3. MAC 地址被系统或安全策略隐藏
	//      - Wi-Fi MAC 随机化
	//      - 企业安全策略 / 防作弊软件
	//      - 驱动返回全 0 MAC（00-00-00-00-00-00）
	//
	//   4. 云电脑 / 服务器 / 精简系统环境
	//      - 云桌面、云主机
	//      - 精简版 Windows / 沙箱环境
	//
	//   5. 系统网络组件异常
	//      - GetAdaptersInfo 调用失败
	//
	//  设计约定：
	//   - 若无法获取到合法的 MAC 地址，返回空字符串
	//   - 调用方需允许 mac 为空
	// --------------------------------------------------------------------

	static const size_t buffer_size = 0x100;
	char szMacAddress[buffer_size] = { 0 };

	try
	{
		PIP_ADAPTER_INFO pAdapterInfo =
			(PIP_ADAPTER_INFO)malloc(sizeof(IP_ADAPTER_INFO));
		if (!pAdapterInfo)
			return "";

		ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);

		// 第一次调用，获取所需缓冲区大小
		if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
		{
			free(pAdapterInfo);
			pAdapterInfo = (PIP_ADAPTER_INFO)malloc(ulOutBufLen);
			if (!pAdapterInfo)
				return "";
		}

		DWORD dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);
		if (dwRetVal == NO_ERROR)
		{
			for (PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
				pAdapter != NULL;
				pAdapter = pAdapter->Next)
			{
				// 必须具备有效 IP 的网卡才参与判断
				if (pAdapter->IpAddressList.IpAddress.String == "")
					continue;

				sprintf_s(
					szMacAddress,
					buffer_size,
					"%02X-%02X-%02X-%02X-%02X-%02X",
					pAdapter->Address[0],
					pAdapter->Address[1],
					pAdapter->Address[2],
					pAdapter->Address[3],
					pAdapter->Address[4],
					pAdapter->Address[5]);

				break;
			}
		}
		else
		{
			free(pAdapterInfo);
			return "";
		}

		free(pAdapterInfo);
		return std::string(szMacAddress);
	}
	catch (...)
	{
		return "";
	}
}

int LoginClient::Initialize(
	const char* serverAddr, const char* backupServerAddr,
	int appId, int areaId, int groupId, int locale, int tag,
	int productId, const char* productVersion, int customSecurityLevel,
	SdoBase_CheckCodeLoginCallback checkCodeLoginCallback,
	SdoBase_DynamicLoginCallback dynamicLoginCallback,
	SdoBase_FcmLoginCallback fcmLoginCallback,
	SdoBase_LoginResultCallback loginResultCallback,
	SdoBase_GetDynamicKeyCallback getDynamicKeyCallback,
	SdoBase_SendPhoneCheckCodeCallback phoneCheckCodeCallback,
	SdoBase_CheckAccounTypeCallback checkAccountCallback,
	SdoBase_GetQrCodeCallback getQrCodeCallback,
	SdoBase_GetLoginStateCallback loginStatusCallback,
	SdoBase_ExtendLoginStateCallback extendLoginStateCallback,
	SdoBase_LogoutCallback logoutCallback,
	SdoBase_SendPushMessageCallback sendPushMessageCallback,
	bool enableGunionPreInit, const char* gunionServerAddr, const char* gunionBackupServerAddr
)
{
	char hostName[100] = { 0 };
	ParseHttpAddr(serverAddr, m_requestProcess.m_hostName, m_requestProcess.m_port);
	if (backupServerAddr != nullptr && strlen(backupServerAddr) > 0)
	{
		ParseHttpAddr(backupServerAddr, m_requestProcess.m_hostName2, m_requestProcess.m_port2);
	}

	if (gunionServerAddr != nullptr && strlen(gunionServerAddr) > 0)
	{
		ParseHttpAddr(gunionServerAddr, m_requestProcess.m_hostName7, m_requestProcess.m_port7);
	}

	if (gunionBackupServerAddr != nullptr && strlen(gunionBackupServerAddr) > 0)
	{
		ParseHttpAddr(gunionBackupServerAddr, m_requestProcess.m_hostName8, m_requestProcess.m_port8);
	}

	m_requestProcess.m_appid = appId;
	m_requestProcess.m_areaid = areaId;
	m_requestProcess.m_groupId = groupId;
	m_requestProcess.m_locale = locale;
	m_requestProcess.m_tag = tag;
	m_requestProcess.m_productId = productId;
	m_requestProcess.m_customSecurityLevel = customSecurityLevel;
	m_requestProcess.m_deviceId = "\0";
	m_requestProcess.m_macId = "\0";
	if (productVersion != nullptr)
		m_requestProcess.m_productVersion = productVersion;

	m_checkCodeLoginCallback = checkCodeLoginCallback;
	m_dynamicLoginCallback = dynamicLoginCallback;
	m_fcmLoginCallback = fcmLoginCallback;
	m_loginResultCallback = loginResultCallback;
	m_getDynamicKeyCallback = getDynamicKeyCallback;
	m_phoneCheckCodeCallback = phoneCheckCodeCallback;
	m_checkAccountCallback = checkAccountCallback;
	m_getCodeKeyCallback = getQrCodeCallback;
	m_loginStatusCallback = loginStatusCallback;
	m_extendLoginStateCallback = extendLoginStateCallback;
	m_logoutCallback = logoutCallback;
	m_sendPushMessageCallback = sendPushMessageCallback;

	//获取下行短信公共参数的uniqueId
	m_requestProcess.uniqueId = CreateGUID();

	// --------------------------------------------
	// 提前初始化 Gunion 会话
	// --------------------------------------------
	if (enableGunionPreInit)
	{
		//把执行gunion的标志位存储起来
		m_enableGunionPreInit = enableGunionPreInit;
		//执行Gunion获取公钥和握手逻辑
		EnsureGunionReady();
	}

	return 0;
}

int LoginClient::Initialize2(const char* serverAddr, const char* backupServerAddr,
	int appId, int areaId, int groupId, int locale, int tag,
	int productId, const char* productVersion, int customSecurityLevel)
{
	return Initialize(serverAddr, backupServerAddr, appId, areaId, groupId, locale, tag, productId, productVersion, customSecurityLevel,
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, true, "", "");
}

int LoginClient::SetTimeout(int timeout, int secTimeout)
{
	m_requestProcess.m_timeout = timeout;
	m_requestProcess.m_timeout2 = secTimeout;
	return 0;
}

int LoginClient::ModifyAppInfo(int appId, int areaId, int groupId)
{
	m_requestProcess.m_appid = appId;
	m_requestProcess.m_areaid = areaId;
	m_requestProcess.m_groupId = groupId;
	return 0;
}

//////////////////////////////////////////////////////////////////////////
// request

HttpRequest* LoginClient::GetDynamicKeyRequest(string publicKey)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	return m_requestProcess.GetGetDynamicKeyRequest(publicKey);
}

int LoginClient::GetDynamicKey()
{
	HttpRequest* request = GetDynamicKeyRequest(m_publicKey);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}


int LoginClient::StaticLogin(const char* inputUserId, const char* password,
	int accountDomain, int autoLoginFlag, int autoLoginKeepTime, int isSupportGeetest, int keepLoginFlag)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetStaticLoginRequest(inputUserId, password, accountDomain, autoLoginFlag, autoLoginKeepTime, isSupportGeetest, keepLoginFlag);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::StaticLoginWithGameAccount(const char* inputUserId, const char* password,
	int accountDomain, int autoLoginFlag, int autoLoginKeepTime, int isSupportGeetest, int inputUserType, int keepLoginFlag, const char* scene/*=nullptr*/)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetStaticLoginWithGameAccountRequest(inputUserId, password, accountDomain, autoLoginFlag, autoLoginKeepTime, isSupportGeetest, inputUserType, keepLoginFlag, scene);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::QQGameLogin(const char* openid, const char* openkey, bool is_limited, int company_id)
{
	TRACEI("openid:%s, openkey:%s, company_id:%d.", openid, openkey, company_id);

	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetQQGameLoginRequest(openid, openkey, is_limited, company_id);
	if (request == nullptr)
	{
		TRACEI("2");
		return ERROR_FORAT_URL;
	}
	TRACEI("request->url:[%s]", request->url.c_str());
	TRACEI("request->url2:[%s]", request->url2.c_str());
	TRACEI("request->hostName:[%s]", request->hostName.c_str());
	TRACEI("request->hostName2:[%s]", request->hostName2.c_str());
	TRACEI("request->port:[%d],request->port2:[%d]", request->port, request->port2);
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		TRACEI("2");
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::WeGameLogin(const char* rail_id, const char* rail_session_ticket, bool is_limited)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetWeGameLoginRequest(rail_id, rail_session_ticket, is_limited);

	TRACEI("WeGameLogin request->url=%s", request->url.c_str());

	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}
int LoginClient::WeGameLogin(const char* rail_id, const char* rail_session_ticket, bool is_limited, int company_id)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetWeGameLoginRequest(rail_id, rail_session_ticket, is_limited, company_id);

	TRACEI("WeGameLogin request->url=%s", request->url.c_str());

	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::ThirdLogin(const char* third_token, const char* companyId, const char* scene,
	const char* phone, const char* smsCode, const char* extend, const char* szIsLimited)
{

	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetThirdLoginRequest(third_token, companyId, scene, phone, smsCode, extend, szIsLimited);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::ThirdForLogin(const char* third_token, const char* companyId, const char* scene,
	const char* phone, const char* smsCode, const char* extend, const char* szIsLimited)
{

	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetForThirdLoginRequest(third_token, companyId, scene, phone, smsCode, extend, szIsLimited);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::StaticLogin2(const char* inputUserId, const char* password,
	int accountDomain, int autoLoginFlag, int autoLoginKeepTime, int isSupportGeetest, int keepLoginFlag)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetStaticLoginRequest2(inputUserId, password, accountDomain, autoLoginFlag, autoLoginKeepTime, isSupportGeetest, keepLoginFlag);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::CheckCodeLogin(const char* checkCode, const char* challenge, const char* validate, const char* seccode, const char* outinfo, int keepLoginFlag, const char* captchaInfo)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = nullptr;
	if (!m_isPushMessageCheckCode)
	{
		request = m_requestProcess.GetCheckCodeLoginRequest(checkCode, challenge, validate, seccode, outinfo, keepLoginFlag, captchaInfo);
	}
	else
	{
		request = m_requestProcess.GetSendPushMessageVerifyCheckCodeRequest(m_pushMsgSessionKey.c_str(), checkCode, keepLoginFlag, captchaInfo);
	}

	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::DynamicLogin(const char* password, int keepLoginFlag)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetDynamicLoginRequest(password, keepLoginFlag);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::FcmLogin(const char* realName, const char* idCard, const char* email, int keepLoginFlag)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetFcmLoginRequest(realName, idCard, email, keepLoginFlag);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	m_httpThread->SetState(0);
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::AutoLogin(const char* autoLoginSessionKey)
{

	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetAutoLoginRequest(autoLoginSessionKey);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::SsoLogin(const char* tgt, const char* scene)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetSsoLoginRequest(tgt, scene);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::SsoLogin(const char* tgt, const char* scene, int appId)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetSsoLoginRequest(tgt, scene, appId);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::GetAuthCodeRequest(const char* tgt, const char* scene)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetAuthCodeRequest(tgt, scene);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::CheckAuthCodeRequest(const char* authCode)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetCheckAuthCodeRequest(authCode);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::GetTicket()
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetTicketRequest();
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	TRACEI("request->url:[%s]", request->url.c_str());
	TRACEI("request->url2:[%s]", request->url2.c_str());
	TRACEI("request->hostName:[%s]", request->hostName.c_str());
	TRACEI("request->hostName2:[%s]", request->hostName2.c_str());
	TRACEI("request->port:[%d],request->port2:[%d]", request->port, request->port2);
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::CreateWeGameOrder(const char* ticket, const char* szOrderId, const char* szProductId, const char* szGroupId,
	const char* szAreaId, const char* szExtend, const char* szSign, const char* channel)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.CreateWeGameOrder(ticket, szOrderId, szProductId, szGroupId, szAreaId, szExtend, szSign, channel);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}

	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

//灵活展示三方界面
int LoginClient::GetThirdLoginSkin()
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetThirdLoginSkin();
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}

	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::CreateCQ4Order(const char* ticket, const char* szOrderId, const char* szProductId,
	const char* szGroupId, const char* szAreaId, const char* szExtend, const char* szSign,
	const char* channel, int iSSandboxAccount)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.CreateCQ4Order(ticket, szOrderId, szProductId,
		szGroupId, szAreaId, szExtend, szSign, channel, iSSandboxAccount);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}

	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::CreateCQ4Query(const char* ticket, const char* szOrderId, const char* szSign,
	const char* channel, int iSSandboxAccount)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.CreateCQ4Query(ticket, szOrderId, szSign, channel, iSSandboxAccount);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}

	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::CreateSteamChannelOrder(const char* ticket, const char* szOrderId, const char* szProductId, const char* szGroupId,
	const char* szAreaId, const char* szExtend, const char* szSign, const char* channel)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.CreateSteamChannelOrder(ticket, szOrderId, szProductId, szGroupId, szAreaId, szExtend, szSign, channel);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}

	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::CreateQQGameOrder(const char* ticket, const char* szOrderId, const char* szProductId, const char* szGroupId,
	const char* szAreaId, const char* szExtend, const char* szOpenId, const char* szOpenKey, const char* szPfKey, const char* szSign, const char* channel)
{
	TRACET();
	TRACEI("ticket:%s, szOrderId:%s, szOpenId:%s, szSign:%s.", ticket, szOrderId, szOpenId, szSign);
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.CreateQQGameOrder(ticket, szOrderId, szProductId, szGroupId, szAreaId, szExtend, szOpenId, szOpenKey, szPfKey, szSign, channel);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	TRACEI("request->url:[%s]", request->url.c_str());
	TRACEI("request->url2:[%s]", request->url2.c_str());
	TRACEI("request->hostName:[%s]", request->hostName.c_str());
	TRACEI("request->hostName2:[%s]", request->hostName2.c_str());
	TRACEI("request->port:[%d],request->port2:[%d]", request->port, request->port2);

	if (m_httpThread->ProcessRequest(request) != 0)
	{
		TRACEI("2");
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::QQGameIsLogin(const char* openid, const char* openkey, int company_id)
{
	TRACEI("openid:%s, openkey:%s, company_id:%d.", openid, openkey, company_id);

	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetQQGameIsLoginRequest(openid, openkey, company_id);
	if (request == nullptr)
	{
		TRACEI("2");
		return ERROR_FORAT_URL;
	}
	TRACEI("request->url:[%s]", request->url.c_str());
	TRACEI("request->url2:[%s]", request->url2.c_str());
	TRACEI("request->hostName:[%s]", request->hostName.c_str());
	TRACEI("request->hostName2:[%s]", request->hostName2.c_str());
	TRACEI("request->port:[%d],request->port2:[%d]", request->port, request->port2);
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		TRACEI("2");
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::WeGameStatus()
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.WeGameStatus();
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}

	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::CreateLxOrder(const char* ticket, const char* szOrderId, const char* szProductId, const char* szGroupId,
	const char* szAreaId, const char* szExtend, const char* szSign, const char* channel, const char* szLenovoTgt, const char* szRole)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.CreateLxOrder(ticket, szOrderId, szProductId, szGroupId, szAreaId, szExtend, szSign, channel, szLenovoTgt, szRole);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}

	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::FastLogin()
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetFastLoginRequest();
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::PhoneCheckCodeLogin(const char* checkCode)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetPhoneCheckCodeLoginRequest(checkCode);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::QrCodeLogin(int autoLoginFlag, int autoLoginKeepTime, int keepLoginFlag)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetQrCodeLoginRequest(autoLoginFlag, autoLoginKeepTime, keepLoginFlag);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::Logout()
{

	if (!m_requestProcess.m_tgt.empty())
	{
		m_requestProcess.SetReqParams(&m_mapReqParams);
		m_mapReqParams.clear();

		HttpRequest* request = m_requestProcess.GetLogoutRequest();
		if (request == nullptr)
		{
			return ERROR_FORAT_URL;
		}
		if (m_httpThread->ProcessRequest(request) != 0)
		{
			delete request;
			return ERROR_PROCESSING;
		}
		return 0;
	}
	return -1;
}

int LoginClient::CheckAccoutType(const char* inputUserId)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetCheckAccoutTypeRequest(inputUserId);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::SendPhoneCheckCode(const char* inputUserId, int type)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetSendPhoneCheckCodeRequest(inputUserId, type);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::KickoffAccountVerify(const char* tgt)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetKickoffAccountVerifyRequest(tgt);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::KickoffProtectCodeLogin(const char* tgt, const char* protectCode)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetKickoffProtectCodeRequest(tgt, protectCode);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::KickoffCheckCodeLogin(const char* tgt, const char* checkCode, int nKickoffAppid, int nKickoffAreaid)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetKickoffAccountRequest(tgt, checkCode, nKickoffAppid, nKickoffAreaid);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::GetSessionIdStates(const char* sessioId)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetSessionIdStatesRequest(sessioId);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::GetClientVKey(const char* tgt)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetClientVKeyRequest(tgt);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::SendUserAccount(const char* inputUserId)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.SendUserAccountRequest(inputUserId);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::GetQrCode()
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetGetQrCodeRequest();
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::GetLoginState()
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetGetLoginStateRequest();
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::GetLoginState2(const char* tgt)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetGetLoginStateRequest();
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::ExtendLoginState(const char* tgt)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetExtendLoginStateRequest(tgt);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

string LoginClient::GetTgt()
{
	return m_requestProcess.m_tgt;
}

string LoginClient::GetToken()
{
	return m_gunionToken;
}

bool LoginClient::GetEnableGunionPreInit()
{
	return m_enableGunionPreInit;
}

void LoginClient::SetTgt(const string& tgt)
{
	//if(m_enableGunionPreInit)
	//{
	//	m_gunionToken = tgt;
	//}
	//else
	{
		m_requestProcess.m_tgt = tgt;
	}
}

int LoginClient::GetPushMessageStatus(const char* inputUserId)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetPushMessageStatusRequest(inputUserId);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::PushMessageLogin(int autoLoginFlag, int autoLoginKeepTime, int keepLoginFlag)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetPushMessageLoginRequest(autoLoginFlag, autoLoginKeepTime, keepLoginFlag);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::CancelPushMessageLogin()
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetCancelPushMessageLoginRequest();
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::SendPushMessage(const char* inputUserId, const char* scene)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetSendPushMessageRequest(inputUserId, scene);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::RltLogin(const char* vkey)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetRltLoginRequest(vkey);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::GetAccountInfo(const char* tgt)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetGetAccountInfoRequest(tgt);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::GetLoginHistory(const char* tgt, int queryNumber)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetGetLoginHistoryRequest(tgt, queryNumber);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::GetLoginUserInfo(const char* tgt)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetGetLoginUserInfoRequest(tgt);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::SetLoginUserInfo(const char* tgt, const char* notename)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetSetLoginUserInfoRequest(tgt, notename);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::GetLoginAreaInfo(int nAppId)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetGetLoginAreaInfoRequest(nAppId);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::PromotionInfoConfirm(int days)
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetPromotionInfoConfirmRequest(days);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::GetPromotionInfo()
{
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetPromotionInfoRequest();
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}


//////////////////////////////////////////////////////////////////////////
// response

void LoginClient::ProcessResponse(int result, int requestCode, const string& response,
	const vector<string>& vecCookies, bool isUrl2)
{
	map<string, string> keyValues;
	ResponseProcess::Process(this, result, requestCode, response, vecCookies, keyValues);
	map<int, ResponseProcessFunc>::iterator iterFunc = m_mapResponseFunc.find(requestCode);
	if (iterFunc == m_mapResponseFunc.end())
	{
		return;
	}

	if (isUrl2)
	{
		keyValues["isUrl2"] = "true";
	}

	if (m_waitEvent)
	{
		if (WaitForSingleObject(m_waitEvent, 0) != WAIT_OBJECT_0)
		{
			SetEvent(m_waitEvent);
		}

	}

	m_mapRespParams = keyValues;
	MapToMap(m_mapValList, keyValues);
	(this->*(iterFunc->second))(requestCode, keyValues);

}

void LoginClient::SetHost3AndHost4(string hostname3, int hostport3, string hostname4, int hostport4)
{
	m_requestProcess.m_hostName = hostname3;
	m_requestProcess.m_port = hostport3;
	m_requestProcess.m_hostName2 = hostname4;
	m_requestProcess.m_port2 = hostport4;
}

void LoginClient::SetHost5AndHost6(string hostname5, int hostport5, string hostname6, int hostport6)
{
	m_requestProcess.m_hostName3 = hostname5;
	m_requestProcess.m_port3 = hostport5;
	m_requestProcess.m_hostName4 = hostname6;
	m_requestProcess.m_port4 = hostport6;
}

/**
 * @brief 判断错误码是否需要进行 GBK 编码处理
 *
 * 说明：
 *   - 这里集中维护“需要 GBK 转换的错误码列表”
 *   - 避免在 UI 或业务层分散判断
 */
bool LoginClient::NeedGbkForCode(int code)
{
	switch (code)
	{
	case -10130101:
	case -10130102:
	case -10130103:
	case -10130104:
	case -10130105:
	case -10130106:
	case -10130108:
	case -10130109:
		return true;
	default:
		return false;
	}
}

/**
 * @brief 根据错误码返回安全可显示的错误信息
 *
 * 处理逻辑：
 *   - 若错误码需要 GBK：
 *       执行 Utf8ToGbk 转换
 *   - 否则：
 *       直接返回原始 UTF-8 字符串
 *
 * 设计目标：
 *   - 对上层 UI 隐藏字符集差异
 *   - 保证所有错误提示均可正常显示中文
 */
std::string LoginClient::GetSafeMsg(const char* utf8Msg, int code)
{
	if (NeedGbkForCode(code))
		return Utf8ToGbk(utf8Msg);

	return utf8Msg;
}

void LoginClient::ProcessGetDynamicKeyResponse(int reqeustCode, map<string, string>& keyValues)
{
	m_requestProcess.m_guid = keyValues["guid"];

	string dKey = keyValues["dynamicKey"];

	//   	dKey = "fC6TJzUDJ3jR8uCoRdApX0xDrFqLOAxR";
	//   	m_guid = "2DD9E365961E43FE8E7662CF7D58F9F2";

	//   	char s1[6];
	//  	memset(s1, 0, sizeof(s1));
	//  
	//   	char s[MAX_PATH];
	//   	memset(s, 0, sizeof(s));
	//   	sdoa_encrypt(s1, 6, m_guid.c_str(), s);
	//   	dKey = s;

	string isUrl2 = keyValues["isUrl2"];
	if (dKey.length() > 0 && m_guid.length() > 0 && isUrl2 == "true")
	{

#ifdef _DEBUG
		OutputDebugStringA(dKey.c_str());
#endif

		string temp = CBase64Coding::Decode(dKey);

		if (temp.size() > 0)
		{
			char key[1024] = { 0 };
			memset(key, 0, sizeof(key));
			int len = sdoa_3des_cbc_decrypt(temp.c_str(), temp.size(), m_guid.c_str(), key);
			if (len > 0)
			{
				keyValues["dynamicKey"].assign(key, len);
				m_mapValList["dynamicKey"].assign(key, len);
			}
			else
			{
				keyValues["resultCode"] = IntToString(ERROR_HTTP_AUTHEN);
				keyValues["failReason"] = "HTTP 3DES DECRYPT FAILED!";
			}
		}
		else
		{
			keyValues["resultCode"] = IntToString(ERROR_HTTP_AUTHEN);
			keyValues["failReason"] = "HTTP BASE64 DECODE FAILED!";
		}
	}

	if (m_getDynamicKeyCallback != nullptr)
	{
		m_getDynamicKeyCallback(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
			keyValues["dynamicKey"].c_str(),
			keyValues["guid"].c_str(),
			(SdoBaseHandle*)this
		);
	}

	SdoBase_GetDynamicKeyCallback callback = (SdoBase_GetDynamicKeyCallback)m_mapCallback["SdoBase_GetDynamicKeyCallback"];
	if (callback != nullptr)
	{
		callback(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
			keyValues["dynamicKey"].c_str(),
			keyValues["guid"].c_str(),
			(SdoBaseHandle*)this
		);
	}
}

void LoginClient::ProcessLoginResponse(int reqeustCode, map<string, string>& keyValues)
{
	int resultCode = atoi(keyValues["resultCode"].c_str());
	if (reqeustCode == REQ_StaticLogin ||
		reqeustCode == REQ_AutoLogin ||
		reqeustCode == REQ_PhoneCheckCodeLogin ||
		reqeustCode == REQ_CheckCodeLogin ||
		reqeustCode == REQ_PushMessageLogin ||
		reqeustCode == REQ_CodeKeyLogin ||
		reqeustCode == REQ_AccountGroupLogin ||
		reqeustCode == REQ_ThirdPartyPollingLogin ||
		reqeustCode == REQ_ThirdPartyLogin ||
		reqeustCode == REQ_WeGameLogin ||
		reqeustCode == REQ_QQGameLogin ||
		reqeustCode == REQ_LenovoLogin ||
		reqeustCode == REQ_ThirdLogin
		)
	{
		m_requestProcess.m_autoLoginSessionKey = keyValues["autoLoginSessionKey"];
		m_requestProcess.m_autoLoginMaxAge = keyValues["autoLoginMaxAge"];
	}

	if (reqeustCode == REQ_CodeKeyLogin)
	{
		//扫码登录才有m_inputUserId
		m_requestProcess.m_inputUserId = keyValues["inputUserId"];
	}
	else if (reqeustCode == REQ_FcmLogin)
	{
		if (resultCode == 0)
		{
			m_requestProcess.SetFlowId("");
		}
	}
	else
	{
		m_requestProcess.m_inputUserId = "";
	}
	string nextAction = keyValues["nextAction"];

	if (nextAction == "8")
	{
		m_requestProcess.m_guid = keyValues["guid"];
		int needCheckCode = 1;
		const char* url = keyValues["picUrl"].c_str();
		int width = 0;
		int height = 0;

		if (keyValues["imagecodeType"] == "4")
		{	//极验3.0
			needCheckCode = 4;
			url = keyValues["gt_url"].c_str();
			width = atoi(keyValues["width"].c_str());
			height = atoi(keyValues["height"].c_str());
		}
		if (keyValues["imagecodeType"] == "16")
		{	//极验4.0
			needCheckCode = 16;
			url = keyValues["gt_url"].c_str();
			width = atoi(keyValues["width"].c_str());
			height = atoi(keyValues["height"].c_str());
		}
		if (m_checkCodeLoginCallback != nullptr)
		{
			m_checkCodeLoginCallback(resultCode, Utf8ToGbk(keyValues["failReason"].c_str()).c_str(), url, needCheckCode, width, height, (SdoBaseHandle*)this);
		}
		SdoBase_CheckCodeLoginCallback callback = (SdoBase_CheckCodeLoginCallback)m_mapCallback["SdoBase_CheckCodeLoginCallback"];
		if (callback != nullptr)
		{
			callback(resultCode, Utf8ToGbk(keyValues["failReason"].c_str()).c_str(), url, needCheckCode, width, height, (SdoBaseHandle*)this);
		}
	}
	else if (nextAction == "13" || nextAction == "18" || nextAction == "100")
	{
		m_requestProcess.m_guid = keyValues["guid"];
		if (nextAction == "13")
			m_requestProcess.m_loginType = "1";
		else if (nextAction == "18")
			m_requestProcess.m_loginType = "2";
		else if (nextAction == "100")
			m_requestProcess.m_loginType = "3";


		if (m_dynamicLoginCallback != nullptr)
		{

			m_dynamicLoginCallback(
				resultCode,
				Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
				atoi(m_requestProcess.m_loginType.c_str()),
				atoi(keyValues["deviceType"].c_str()),
				Utf8ToGbk(keyValues["deviceDisplayType"].c_str()).c_str(),
				Utf8ToGbk(keyValues["challenge"].c_str()).c_str(),
				"other",
				"",
				keyValues["inputUserId"].c_str(),
				(SdoBaseHandle*)this
			);
		}

		SdoBase_DynamicLoginCallback callback = (SdoBase_DynamicLoginCallback)m_mapCallback["SdoBase_DynamicLoginCallback"];
		if (callback != nullptr)
		{
			callback(
				resultCode,
				Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
				atoi(m_requestProcess.m_loginType.c_str()),
				atoi(keyValues["deviceType"].c_str()),
				Utf8ToGbk(keyValues["deviceDisplayType"].c_str()).c_str(),
				Utf8ToGbk(keyValues["challenge"].c_str()).c_str(),
				"other",
				"",
				keyValues["inputUserId"].c_str(),
				(SdoBaseHandle*)this
			);
		}
	}
	else if (nextAction == "202")
	{
		if (resultCode == 0)
		{
			m_requestProcess.m_tgt = keyValues["tgt"];
			if (reqeustCode == REQ_FcmLogin)
			{
				m_requestProcess.SetFlowId("");
			}

		}

		if (m_fcmLoginCallback != nullptr)
		{
			m_fcmLoginCallback(
				resultCode,
				Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
				atoi(keyValues["isNew"].c_str()) != 0,
				(SdoBaseHandle*)this
			);
		}

		SdoBase_FcmLoginCallback callback = (SdoBase_FcmLoginCallback)m_mapCallback["SdoBase_FcmLoginCallback"];
		if (callback != nullptr)
		{
			callback(
				resultCode,
				Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
				atoi(keyValues["isNew"].c_str()) != 0,
				(SdoBaseHandle*)this
			);
		}
	}
	else if (nextAction == "203")
	{
		//std::string safePhoneTip=keyValues["safePhoneTip"];
		if (m_BindSecurityPhoneLoginCallback)
		{
			m_BindSecurityPhoneLoginCallback(
				resultCode,
				Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
				(SdoBaseHandle*)this
			);
		}
		SdoBase_OtherLoginSecurityPhoneCallback callback = (SdoBase_OtherLoginSecurityPhoneCallback)m_mapCallback["SdoBase_OtherLoginSecurityPhoneCallback"];
		if (callback != nullptr)
		{
			callback(
				resultCode,
				Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
				(SdoBaseHandle*)this
			);
		}
	}
	else if (nextAction == "301")
	{
		//把上下文存储起来
		guard_dian_flowId = keyValues["flowId"];

		if (resultCode == 0)
		{
			m_requestProcess.m_tgt = keyValues["tgt"];

		}

		if (m_GuardDianCallback != nullptr)
		{
			m_GuardDianCallback(
				resultCode,
				Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
				keyValues["flowId"].c_str(),
				(SdoBaseHandle*)this
			);
		}

		SdoBaseGuardDianCallback callback = (SdoBaseGuardDianCallback)m_mapCallback["SdoBaseGuardDianCallback"];
		if (callback != nullptr)
		{
			callback(
				resultCode,
				Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
				keyValues["flowId"].c_str(),
				(SdoBaseHandle*)this
			);
		}
	}
	else if (nextAction == "210")
	{
		//需要展示激活逻辑
		if (resultCode == 0)
		{
			m_requestProcess.m_tgt = keyValues["tgt"];
		}

		if (m_ShowActiveLoginDlgCallback != nullptr)
		{
			m_ShowActiveLoginDlgCallback(
				resultCode,
				Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
				keyValues["tgt"].c_str());
		}
	}
	else
	{
		if (resultCode == 0)
		{
			m_requestProcess.m_tgt = keyValues["tgt"];
		}

		if (m_loginResultCallback != nullptr)
		{
			m_loginResultCallback(
				resultCode,
				Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
				keyValues["sndaId"].c_str(),
				//keyValues["ticket"].c_str(),
				(reqeustCode == REQ_SsoLoginAuthCode) ? keyValues["authorization"].c_str() : keyValues["ticket"].c_str(),
				keyValues["accountUpgradeUrl"].c_str(),
				keyValues["mobile"].c_str(),
				(reqeustCode == REQ_SsoLogin || reqeustCode == REQ_SsoLoginAuthCode) ? "" : m_requestProcess.m_autoLoginSessionKey.c_str(),
				(reqeustCode == REQ_SsoLogin || reqeustCode == REQ_SsoLoginAuthCode) ? 0 : atoi(m_requestProcess.m_autoLoginMaxAge.c_str()),
				//(reqeustCode == REQ_SsoLoginAuthCode)?"":m_requestProcess.m_autoLoginSessionKey.c_str(),
				//(reqeustCode == REQ_SsoLoginAuthCode)?0:atoi(m_requestProcess.m_autoLoginMaxAge.c_str()),
				atoi(keyValues["popWindowFlag"].c_str()),
				keyValues["redirectURL"].empty() ? nullptr : keyValues["redirectURL"].c_str(),
				reqeustCode == REQ_RltLoginKeepLogin ? keyValues["inputUserId"].c_str() : m_requestProcess.m_inputUserId.empty() ? nullptr : m_requestProcess.m_inputUserId.c_str(),
				keyValues["mid"].c_str(),
				keyValues["noteName"].c_str(),
				keyValues["displayAccount"].c_str(),
				(reqeustCode == REQ_WeGameLogin) ? "310" : keyValues["companyId"].c_str(),
				atoi(keyValues["isNew"].c_str()) != 0,
				keyValues["appMid"].c_str(),
				keyValues["tgt"].c_str(),
				keyValues["keepLoginKey"].c_str(),
				keyValues["flowId"].c_str(),
				keyValues["isScanned"].c_str(),
				(SdoBaseHandle*)this
			);
		}

		SdoBase_LoginResultCallback callback = (SdoBase_LoginResultCallback)m_mapCallback["SdoBase_LoginResultCallback"];
		if (callback != nullptr)
		{
			callback(
				resultCode,
				Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
				keyValues["sndaId"].c_str(),
				keyValues["ticket"].c_str(),
				keyValues["accountUpgradeUrl"].c_str(),
				keyValues["mobile"].c_str(),
				(reqeustCode == REQ_SsoLogin || reqeustCode == REQ_SsoLoginAuthCode) ? "" : m_requestProcess.m_autoLoginSessionKey.c_str(),
				(reqeustCode == REQ_SsoLogin || reqeustCode == REQ_SsoLoginAuthCode) ? 0 : atoi(m_requestProcess.m_autoLoginMaxAge.c_str()),
				//(reqeustCode == REQ_SsoLoginAuthCode)?"":m_requestProcess.m_autoLoginSessionKey.c_str(),
				//(reqeustCode == REQ_SsoLoginAuthCode)?0:atoi(m_requestProcess.m_autoLoginMaxAge.c_str()),
				atoi(keyValues["popWindowFlag"].c_str()),
				keyValues["redirectURL"].empty() ? nullptr : keyValues["redirectURL"].c_str(),
				reqeustCode == REQ_RltLoginKeepLogin ? keyValues["inputUserId"].c_str() : m_requestProcess.m_inputUserId.empty() ? nullptr : m_requestProcess.m_inputUserId.c_str(),
				keyValues["mid"].c_str(),
				keyValues["noteName"].c_str(),
				keyValues["displayAccount"].c_str(),
				(reqeustCode == REQ_WeGameLogin) ? "310" : keyValues["companyId"].c_str(),
				atoi(keyValues["isNew"].c_str()) != 0,
				keyValues["appMid"].c_str(),
				keyValues["tgt"].c_str(),
				keyValues["keepLoginKey"].c_str(),
				keyValues["flowid"].c_str(),
				keyValues["isScanned"].c_str(),
				(SdoBaseHandle*)this
			);
		}
	}
}

void LoginClient::ProcessCheckAccoutTypeResponse(int reqeustCode, map<string, string>& keyValues)
{
	if (m_checkAccountCallback != nullptr)
	{
		m_checkAccountCallback(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
			atoi(keyValues["type"].c_str()),
			atoi(keyValues["level"].c_str()),
			atoi(keyValues["existing"].c_str()),
			keyValues["mobileMask"].c_str(),
			atoi(keyValues["fromWoa"].c_str()),
			atoi(keyValues["hasPwdLoginRecord"].c_str()),
			atoi(keyValues["recommendLoginType"].c_str()),
			atoi(keyValues["hasCheckCodeLoginRecord"].c_str()),
			Utf8ToGbk(keyValues["ptMask"].c_str()).c_str()
			, (SdoBaseHandle*)this
		);
	}

	SdoBase_CheckAccounTypeCallback callback = (SdoBase_CheckAccounTypeCallback)m_mapCallback["SdoBase_CheckAccounTypeCallback"];
	if (callback != nullptr)
	{
		callback(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
			atoi(keyValues["type"].c_str()),
			atoi(keyValues["level"].c_str()),
			atoi(keyValues["existing"].c_str()),
			keyValues["mobileMask"].c_str(),
			atoi(keyValues["fromWoa"].c_str()),
			atoi(keyValues["hasPwdLoginRecord"].c_str()),
			atoi(keyValues["recommendLoginType"].c_str()),
			atoi(keyValues["hasCheckCodeLoginRecord"].c_str()),
			Utf8ToGbk(keyValues["ptMask"].c_str()).c_str()
			, (SdoBaseHandle*)this
		);
	}

}

void LoginClient::ProcessSendPhoneCheckCodeResponse(int reqeustCode, map<string, string>& keyValues)
{
	m_requestProcess.m_checkCodeSessionKey = keyValues["checkCodeSessionKey"];

	if (m_phoneCheckCodeCallback != nullptr)
	{
		m_phoneCheckCodeCallback(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str()
			, (SdoBaseHandle*)this
		);
	}

	SdoBase_SendPhoneCheckCodeCallback callback = (SdoBase_SendPhoneCheckCodeCallback)m_mapCallback["SdoBase_SendPhoneCheckCodeCallback"];
	if (callback != nullptr)
	{
		callback(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str()
			, (SdoBaseHandle*)this
		);
	}
}

void LoginClient::ProcessGetQrCodeResponse(int reqeustCode, map<string, string>& keyValues)
{
	m_requestProcess.m_codeKey = keyValues["codeKey"];
	if (m_getCodeKeyCallback != nullptr)
	{

		m_getCodeKeyCallback(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
			keyValues["picData"].c_str(),
			(int)keyValues["picData"].length()
			, (SdoBaseHandle*)this
		);
	}

	SdoBase_GetQrCodeCallback callback = (SdoBase_GetQrCodeCallback)m_mapCallback["SdoBase_GetQrCodeCallback"];
	if (callback != nullptr)
	{
		callback(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
			keyValues["picData"].c_str(),
			(int)keyValues["picData"].length()
			, (SdoBaseHandle*)this
		);
	}
}

void LoginClient::ProcessGetLoginStatusResponse(int reqeustCode, map<string, string>& keyValues)
{
	if (m_loginStatusCallback != nullptr)
	{
		m_loginStatusCallback(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
			keyValues["resultCode"] == "0"
			, (SdoBaseHandle*)this
		);
	}

	SdoBase_GetLoginStateCallback callback = (SdoBase_GetLoginStateCallback)m_mapCallback["SdoBase_GetLoginStateCallback"];
	if (callback != nullptr)
	{
		callback(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
			keyValues["resultCode"] == "0"
			, (SdoBaseHandle*)this
		);
	}
}

void LoginClient::ProcessExtendLoginStateResponse(int reqeustCode, map<string, string>& keyValues)
{
	if (atoi(keyValues["resultCode"].c_str()) == 0
		&& !keyValues["tgt"].empty())
	{
		m_requestProcess.m_tgt = keyValues["tgt"];
	}

	if (m_extendLoginStateCallback != nullptr)
	{
		m_extendLoginStateCallback(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str()
			, (SdoBaseHandle*)this
		);
	}

	SdoBase_ExtendLoginStateCallback callback = (SdoBase_ExtendLoginStateCallback)m_mapCallback["SdoBase_LogoutCallback"];
	if (callback != nullptr)
	{
		callback(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str()
			, (SdoBaseHandle*)this
		);
	}
}

void LoginClient::ProcessLogoutResponse(int reqeustCode, map<string, string>& keyValues)
{
	if (m_logoutCallback != nullptr)
	{
		m_logoutCallback(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str()
			, (SdoBaseHandle*)this
		);
	}

	SdoBase_LogoutCallback callback = (SdoBase_LogoutCallback)m_mapCallback["SdoBase_LogoutCallback"];
	if (callback != nullptr)
	{
		callback(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str()
			, (SdoBaseHandle*)this
		);
	}
}

void LoginClient::ProcessSendPushMessageResponse(int reqeustCode, map<string, string>& keyValues)
{
	if (atoi(keyValues["resultCode"].c_str()) == 0)
	{
		m_requestProcess.m_pushMsgSessionKey = keyValues["pushMsgSessionKey"];
	}

	int resultCode = atoi(keyValues["resultCode"].c_str());
	string nextAction = keyValues["nextAction"];
	if (nextAction == "8")
	{
		m_requestProcess.m_guid = keyValues["guid"];
		m_isPushMessageCheckCode = true;
		m_pushMsgSessionKey = keyValues["pushMsgSessionKey"];

		int needCheckCode = 1;
		const char* url = keyValues["picUrl"].c_str();
		int width = 0;
		int height = 0;

		if (keyValues["imagecodeType"] == "4")
		{	//极验3.0
			needCheckCode = 4;
			url = keyValues["gt_url"].c_str();
			width = atoi(keyValues["width"].c_str());
			height = atoi(keyValues["height"].c_str());
		}
		if (keyValues["imagecodeType"] == "16")
		{	//极验4.0
			needCheckCode = 16;
			url = keyValues["gt_url"].c_str();
			width = atoi(keyValues["width"].c_str());
			height = atoi(keyValues["height"].c_str());
		}

		if (m_checkCodeLoginCallback != nullptr)
		{
			m_checkCodeLoginCallback(resultCode, Utf8ToGbk(keyValues["failReason"].c_str()).c_str(), url, needCheckCode, width, height, (SdoBaseHandle*)this);
		}
		SdoBase_CheckCodeLoginCallback callback = (SdoBase_CheckCodeLoginCallback)m_mapCallback["SdoBase_CheckCodeLoginCallback"];
		if (callback != nullptr)
		{
			callback(resultCode, Utf8ToGbk(keyValues["failReason"].c_str()).c_str(), url, needCheckCode, width, height, (SdoBaseHandle*)this);
		}
		return;
	}

	m_isPushMessageCheckCode = false;
	m_pushMsgSessionKey = "";

	if (m_sendPushMessageCallback != nullptr)
	{
		m_sendPushMessageCallback(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
			keyValues["pushMsgSerialNum"].c_str()
			, (SdoBaseHandle*)this
		);
	}

	SdoBase_SendPushMessageCallback callback = (SdoBase_SendPushMessageCallback)m_mapCallback["SdoBase_SendPushMessageCallback"];
	if (callback != nullptr)
	{
		callback(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
			keyValues["pushMsgSerialNum"].c_str()
			, (SdoBaseHandle*)this
		);
	}
}

void LoginClient::ProcessGetPushMessageStatusResponse(int reqeustCode, map<string, string>& keyValues)
{
	if (m_getPushMessageStatusCallback != nullptr)
	{
		m_getPushMessageStatusCallback(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
			atoi(keyValues["appInstallStatus"].c_str()),
			atoi(keyValues["appVersionStatus"].c_str()),
			atoi(keyValues["appOnlineStatus"].c_str()),
			atoi(keyValues["pushMessageSwitchStatus"].c_str()),
			atoi(keyValues["blackListStatus"].c_str())
			, (SdoBaseHandle*)this
		);
	}

	SdoBase_GetPushMessageStatusCallback callback = (SdoBase_GetPushMessageStatusCallback)m_mapCallback["SdoBase_GetPushMessageStatusCallback"];

	if (callback)
	{
		callback(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
			atoi(keyValues["appInstallStatus"].c_str()),
			atoi(keyValues["appVersionStatus"].c_str()),
			atoi(keyValues["appOnlineStatus"].c_str()),
			atoi(keyValues["pushMessageSwitchStatus"].c_str()),
			atoi(keyValues["blackListStatus"].c_str())
			, (SdoBaseHandle*)this
		);
	}
}

void LoginClient::ProcessGetAccountInfoResponse(int reqeustCode, map<string, string>& keyValues)
{
	if (m_getAccountInfoCallback != nullptr)
	{
		m_getAccountInfoCallback(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
			atoi(keyValues["appInstallStatus"].c_str()),
			atoi(keyValues["bindPhoneStatus"].c_str()),
			Utf8ToGbk(keyValues["companyId"].c_str()).c_str(),
			Utf8ToGbk(keyValues["mid"].c_str()).c_str(),
			(SdoBaseHandle*)this
		);
	}

	SdoBase_GetAccountInfoCallback callback = (SdoBase_GetAccountInfoCallback)m_mapCallback["SdoBase_GetAccountInfoCallback"];

	if (callback)
	{
		callback(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
			atoi(keyValues["appInstallStatus"].c_str()),
			atoi(keyValues["bindPhoneStatus"].c_str()),
			Utf8ToGbk(keyValues["companyId"].c_str()).c_str(),
			Utf8ToGbk(keyValues["mid"].c_str()).c_str(),
			(SdoBaseHandle*)this
		);
	}
}

void LoginClient::ProcessGetLoginHistoryResponse(int reqeustCode, map<string, string>& keyValues)
{
	if (m_getLoginHistoryCallback)
	{
		m_getLoginHistoryCallback(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
			Utf8ToGbk(keyValues["endpointIp"].c_str()).c_str(),
			Utf8ToGbk(keyValues["appName"].c_str()).c_str(),
			Utf8ToGbk(keyValues["requestTime"].c_str()).c_str(),
			Utf8ToGbk(keyValues["ipLocation"].c_str()).c_str(),
			(SdoBaseHandle*)this
		);
	}

	SdoBase_GetLoginHistoryCallback callback = (SdoBase_GetLoginHistoryCallback)m_mapCallback["SdoBase_GetLoginHistoryCallback"];
	if (callback)
	{
		callback(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
			Utf8ToGbk(keyValues["endpointIp"].c_str()).c_str(),
			Utf8ToGbk(keyValues["appName"].c_str()).c_str(),
			Utf8ToGbk(keyValues["requestTime"].c_str()).c_str(),
			Utf8ToGbk(keyValues["ipLocation"].c_str()).c_str(),
			(SdoBaseHandle*)this
		);
	}
}

void LoginClient::ProcessGetLoginUserInfoResponse(int reqeustCode, map<string, string>& keyValues)
{
	if (m_getLoginUserInfoCallback)
	{
		m_getLoginUserInfoCallback(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
			Utf8ToGbk(keyValues["sndaId"].c_str()).c_str(),
			Utf8ToGbk(keyValues["displayAccount"].c_str()).c_str(),
			Utf8ToGbk(keyValues["inputUserId"].c_str()).c_str(),
			Utf8ToGbk(keyValues["noteName"].c_str()).c_str(),
			(SdoBaseHandle*)this
		);
	}

	SdoBase_GetLoginUserInfoCallback callback = (SdoBase_GetLoginUserInfoCallback)m_mapCallback["SdoBase_GetLoginUserInfoCallback"];
	if (callback)
	{
		callback(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
			Utf8ToGbk(keyValues["sndaId"].c_str()).c_str(),
			Utf8ToGbk(keyValues["displayAccount"].c_str()).c_str(),
			Utf8ToGbk(keyValues["inputUserId"].c_str()).c_str(),
			Utf8ToGbk(keyValues["noteName"].c_str()).c_str(),
			(SdoBaseHandle*)this
		);
	}
}

void LoginClient::ProcessSetLoginUserInfoResponse(int reqeustCode, map<string, string>& keyValues)
{
	if (m_setLoginUserInfoCallback)
	{
		m_setLoginUserInfoCallback(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
			(SdoBaseHandle*)this
		);
	}

	SdoBase_SetLoginUserInfoCallback callback = (SdoBase_SetLoginUserInfoCallback)m_mapCallback["SdoBase_SetLoginUserInfoCallback"];
	if (callback)
	{
		callback(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
			(SdoBaseHandle*)this
		);
	}
}

void LoginClient::ProcessGetLoginAreaInfoResponse(int reqeustCode, map<string, string>& keyValues)
{
	string areaListResponse = keyValues["areaList"], areaGroupListResponse = keyValues["areaGroupList"];
	string areaCode = "", areaName = "", groupCode = "", groupName = "";
	do
	{
		if (0 == areaListResponse.size()) break;
		areaListResponse = "{ \"areaList\":" + areaListResponse + "}";
		json_object* root = json_tokener_parse(areaListResponse.c_str());
		if (is_error(root)) break;
		json_object* data = json_object_object_get(root, "areaList");
		if (is_error(root)) { json_object_put(root); break; }

		int len = json_object_array_length(data);
		array_list* p = json_object_get_array(data);
		for (int i = 0; i < len; ++i)
		{
			json_object* elem = json_object_array_get_idx(data, i);
			json_object* jcode = json_object_object_get(elem, "areaCode");
			json_object* jname = json_object_object_get(elem, "areaName");
			if (is_error(jcode) || is_error(jname)) { json_object_put(elem); continue; }
			const char* cvalue = json_object_get_string(jcode);
			const char* nvalue = json_object_get_string(jname);
			if (0 != i) areaCode += ",", areaName += ",";
			areaCode += cvalue; areaName += nvalue;
		}
	} while (0);

	do
	{
		if (0 == areaGroupListResponse.size()) break;
		areaGroupListResponse = "{ \"areaGroupList\":" + areaGroupListResponse + "}";
		json_object* root = json_tokener_parse(areaGroupListResponse.c_str());
		if (is_error(root)) break;

		json_object* data = json_object_object_get(root, "areaGroupList");
		if (is_error(root)) { json_object_put(root); break; }

		int len = json_object_array_length(data);
		array_list* p = json_object_get_array(data);
		for (int i = 0; i < len; ++i)
		{
			json_object* elem = json_object_array_get_idx(data, i);
			json_object* jcode = json_object_object_get(elem, "areaCode");
			json_object* jname = json_object_object_get(elem, "areaName");

			json_object* gcode = json_object_object_get(elem, "groupCode");
			json_object* gname = json_object_object_get(elem, "groupName");
			if (is_error(jcode) || is_error(jname) || is_error(gcode) || is_error(gname)) { json_object_put(elem); continue; }
			const char* cvalue = json_object_get_string(jcode);
			const char* nvalue = json_object_get_string(jname);
			const char* gcvalue = json_object_get_string(gcode);
			const char* gnvalue = json_object_get_string(gname);
			if (0 != i) areaCode += ",", areaName += ",", groupCode += ",", groupName += ",";
			areaCode += cvalue; areaName += nvalue; groupCode += gcvalue; groupName += gnvalue;
		}
	} while (0);

	if (m_getLoginAreaInfoCallback)
	{
		m_getLoginAreaInfoCallback(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
			Utf8ToGbk(areaCode.c_str()).c_str(),
			Utf8ToGbk(areaName.c_str()).c_str(),
			Utf8ToGbk(groupCode.c_str()).c_str(),
			Utf8ToGbk(groupName.c_str()).c_str(),
			(SdoBaseHandle*)this
		);
	}

	SdoBase_GetLoginAreaInfoCallback callback = (SdoBase_GetLoginAreaInfoCallback)m_mapCallback["SdoBase_GetLoginAreaInfoCallback"];
	if (callback)
	{
		callback(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
			Utf8ToGbk(areaCode.c_str()).c_str(),
			Utf8ToGbk(areaName.c_str()).c_str(),
			Utf8ToGbk(groupCode.c_str()).c_str(),
			Utf8ToGbk(groupName.c_str()).c_str(),
			(SdoBaseHandle*)this
		);
	}
}

void LoginClient::ProcessGetSessionIdStateResponse(int reqeustCode, map<string, string>& keyValues)
{
	if (m_getSessionIdStatesCallBack)
	{
		m_getSessionIdStatesCallBack(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
			Utf8ToGbk(keyValues["tgtArray"].c_str()).c_str(),
			Utf8ToGbk(keyValues["loginStateArray"].c_str()).c_str(),
			(SdoBaseHandle*)this
		);
	}

	SdoBase_GetSessionIdStatesCallBack callback = (SdoBase_GetSessionIdStatesCallBack)m_mapCallback["SdoBase_GetSessionIdStatesCallBack"];
	if (callback)
	{
		callback(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
			Utf8ToGbk(keyValues["tgtArray"].c_str()).c_str(),
			Utf8ToGbk(keyValues["loginStateArray"].c_str()).c_str(),
			(SdoBaseHandle*)this
		);
	}
}

string LoginClient::CreateGUID()
{
	string strGuid;
	char tmp[33];
	GUID guid;
	if (S_OK == ::CoCreateGuid(&guid))
	{
		_snprintf_s(tmp, sizeof(tmp), "%08X%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X",
			guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
			guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
	}

	strGuid = tmp;

	return strGuid;
}


string StringToHex(string strString)
{
	string strHex;
	int nLen = strString.length();

	if (nLen % 2 != 0)
	{
		return "";
	}

	char* pbyBuffer = (char*)strString.c_str();
	BYTE     byValue = 0;

	for (int i = 0; i < nLen - 1; i++)
	{
		if (pbyBuffer[i] >= 48 && pbyBuffer[i] <= 57)
		{
			byValue = pbyBuffer[i] - '0';
		}
		else if (pbyBuffer[i] >= 65 && pbyBuffer[i] <= 70)
		{
			byValue = pbyBuffer[i] - 55;
		}
		else if (pbyBuffer[i] >= 97 && pbyBuffer[i] <= 102)
		{
			byValue = pbyBuffer[i] - 87;
		}
		else
		{
			return "";
		}

		if (pbyBuffer[i + 1] >= 48 && pbyBuffer[i + 1] <= 57)
		{
			byValue = byValue * 16 + pbyBuffer[i + 1] - '0';
		}
		else if (pbyBuffer[i + 1] >= 65 && pbyBuffer[i + 1] <= 70)
		{
			byValue = byValue * 16 + pbyBuffer[i + 1] - '0' - 7;
		}
		else if (pbyBuffer[i + 1] >= 97 && pbyBuffer[i + 1] <= 102)
		{
			byValue = byValue * 16 + pbyBuffer[i + 1] - 87;
		}
		else
		{
			return "";
		}

		strHex.append(1, (char)byValue);
	}

	return strHex;
}

int LoginClient::ParesePublicKey(int reqeustCode, map<string, string>& keyValues, string& publicKey)
{
	ProcessGetPublicKeyResponse(reqeustCode, keyValues);
	publicKey = m_publicKey;
	return 0;
}

void LoginClient::ProcessGetPublicKeyResponse(int reqeustCode, map<string, string>& keyValues)
{
	string pKey = keyValues["key"];

	// pKey = "MTAyNLfh2CZBdVIvqwVrJ2KdmMBqWGIgEjgAJZUkkDB5jzriCqMtQ3Xmm6QOCDLyC4XJOmqUReX9Ot0KVm/Phm8aq0mfqqi0ZOA97P87vXbnTO9i/0BX4nrlSlw5m/r7V6pyis56h1l9zWUk9FwQWkR2HvaXL6LsiJ18SGld+ScHOyLzAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAM=";

	if (atoi(keyValues["resultCode"].c_str()) != 0 || pKey.length() == 0)
	{
		return;
	}
	m_guid = CreateGUID();
	// m_guid = "zhangputao";	

	char szTempBuffer[2048] = { 0 };
	UINT nLen = 2048;

	string pTemp = CBase64Coding::Decode(pKey);

	HRESULT hr = RSAEx((BYTE*)m_guid.c_str(), m_guid.size(), (BYTE*)szTempBuffer, nLen, (CHAR*)pTemp.c_str(), pTemp.size());

	if (hr == 0)
		m_publicKey.assign(szTempBuffer, nLen);

}

void LoginClient::ProcessGetPromotionInfoResponse(int reqeustCode, map<string, string>& keyValues)
{
	if (keyValues["promotionUrl"].length() > 0)
	{
		ParsePromotionUrl(keyValues["promotionUrl"], m_requestProcess.m_promotionId);
	}

	SdoBase_GetPromotionInfoCallback callback = (SdoBase_GetPromotionInfoCallback)m_mapCallback["SdoBase_GetPromotionInfoCallback"];
	if (callback != nullptr)
	{
		callback(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
			keyValues["promotionUrl"].c_str()
			, (SdoBaseHandle*)this
		);
	}
}

void LoginClient::ProcessPromotionInfoConfirmResponse(int reqeustCode, map<string, string>& keyValues)
{
	SdoBase_PromotionInfoConfirmCallback callback = (SdoBase_PromotionInfoConfirmCallback)m_mapCallback["SdoBase_PromotionInfoConfirmCallback"];
	if (callback != nullptr)
	{
		callback(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
			(SdoBaseHandle*)this
		);
	}
}

void LoginClient::ProcessGetClientVKeyResponse(int reqeustCode, map<string, string>& keyValues)
{
	SdoBase_GetClientVKeyCallback callback = (SdoBase_GetClientVKeyCallback)m_mapCallback["SdoBase_GetClientVKeyCallback"];
	if (callback != nullptr)
	{
		callback(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
			keyValues["clientVKey"].c_str(),
			(SdoBaseHandle*)this
		);
	}
}

void LoginClient::ProcessSendUserAccountResponse(int reqeustCode, map<string, string>& keyValues)
{
	SdoBase_SendUserAccountCallback callback = (SdoBase_SendUserAccountCallback)m_mapCallback["SdoBase_SendUserAccountCallback"];
	if (callback != nullptr)
	{
		callback(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
			(SdoBaseHandle*)this
		);
	}
}

void LoginClient::ProcessCancelPushMessageResponse(int reqeustCode, map<string, string>& keyValues)
{
	SdoBase_CancelPushMessageCallback callback = (SdoBase_CancelPushMessageCallback)m_mapCallback["SdoBase_CancelPushMessageCallback"];
	if (callback != nullptr)
	{
		callback(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
			(SdoBaseHandle*)this
		);
	}
}

void LoginClient::ProcessGetAccountGroupResponse(int reqeustCode, map<string, string>& keyValues)
{
	SdoBase_GetAccountGroupCallback callback = (SdoBase_GetAccountGroupCallback)m_mapCallback["SdoBase_GetAccountGroupCallback"];
	if (callback != nullptr)
	{
		callback(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
			Utf8ToGbk(keyValues["sndaIdArray"].c_str()).c_str(),
			Utf8ToGbk(keyValues["maskAccountArray"].c_str()).c_str(),
			Utf8ToGbk(keyValues["accountArray"].c_str()).c_str(),
			Utf8ToGbk(keyValues["mid"].c_str()).c_str(),
			Utf8ToGbk(keyValues["noteNameArray"].c_str()).c_str(),
			Utf8ToGbk(keyValues["companyIdArray"].c_str()).c_str(),
			(SdoBaseHandle*)this
		);
	}
}

void LoginClient::ProcessKickoffVerifyResponse(int reqeustCode, map<string, string>& keyValues)
{
	int resultCode = atoi(keyValues["resultCode"].c_str());
	string nextAction = keyValues["nextAction"];
	if (nextAction == "8")
	{
		m_requestProcess.m_guid = keyValues["guid"];

		unsigned int imageCodeType = atoi(keyValues["imagecodeType"].c_str());
		if (m_checkCodeLoginCallback != nullptr)
		{
			m_checkCodeLoginCallback(
				resultCode,
				Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
				keyValues["checkCodeUrl"].c_str()
				, (2 == imageCodeType ? 1 : 0)
				, atoi(keyValues["sdg_width"].c_str())
				, atoi(keyValues["sdg_height"].c_str())
				, (SdoBaseHandle*)this
			);
		}

		SdoBase_CheckCodeLoginCallback callback = (SdoBase_CheckCodeLoginCallback)m_mapCallback["SdoBase_CheckCodeLoginCallback"];
		if (callback != nullptr)
		{
			callback(
				resultCode,
				Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
				keyValues["checkCodeUrl"].c_str()
				, (2 == imageCodeType ? 1 : 0)
				, atoi(keyValues["sdg_width"].c_str())
				, atoi(keyValues["sdg_height"].c_str())
				, (SdoBaseHandle*)this
			);
		}
	}
	else
	{
		if (resultCode == 0)
		{
			m_requestProcess.m_tgt = keyValues["tgt"];
		}

		SdoBase_KickoffAccountVerifyCallback callback = (SdoBase_KickoffAccountVerifyCallback)m_mapCallback["SdoBase_KickoffAccountVerifyCallback"];
		if (m_callbackKickoffAccountVerify != nullptr)
		{
			m_callbackKickoffAccountVerify(
				resultCode,
				Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
				(SdoBaseHandle*)this
			);
		}
		else if (callback != nullptr)
		{
			callback(
				resultCode,
				Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
				(SdoBaseHandle*)this
			);
		}
	}
}

void LoginClient::ProcessKickoffResultResponse(int reqeustCode, map<string, string>& keyValues)
{
	SdoBase_KickoffAccountResultCallback callback = (SdoBase_KickoffAccountResultCallback)m_mapCallback["SdoBase_KickoffAccountResultCallback"];
	if (m_callbackKickoffAccountResult != nullptr)
	{
		m_callbackKickoffAccountResult(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str()
			, (SdoBaseHandle*)this
		);
	}
	if (callback != nullptr)
	{
		callback(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str()
			, (SdoBaseHandle*)this
		);
	}
}

void LoginClient::ProcessSendMiGuSmsRequestResponse(int reqeustCode, map<string, string>& keyValues)
{
	SdoBase_SendMiGuSmsCallback callback = (SdoBase_SendMiGuSmsCallback)m_mapCallback["SdoBase_SendMiGuSmsCallback"];
	if (callback != nullptr)
	{
		callback(
			atoi(keyValues["resultCode"].c_str()),
			Utf8ToGbk(keyValues["failReason"].c_str()).c_str()
			, (SdoBaseHandle*)this
		);
	}
}

void LoginClient::ProcessSendSmsRequestResponse(int reqeustCode, map<string, string>& keyValues)
{
	//needCheckCode 1表示图形验证码   4表示极验
	SdoBase_SendSmsCallback callback = (SdoBase_SendSmsCallback)m_mapCallback["SdoBase_SendSmsCallback"];
	int needCheckCode = keyValues["nextAction"] == "8" ? 1 : 0;
	const char* url = keyValues["picUrl"].c_str();
	int width = 0;
	int height = 0;

	if (keyValues["imagecodeType"] == "4") {	//极验
		needCheckCode = 4;
		url = keyValues["gt_url"].c_str();
		width = atoi(keyValues["width"].c_str());
		height = atoi(keyValues["height"].c_str());
	}
	if (callback != nullptr)
	{
		callback(
			atoi(keyValues["resultCode"].c_str()), Utf8ToGbk(keyValues["failReason"].c_str()).c_str(), keyValues["smsSessionKey"].c_str()
			, url, needCheckCode, width, height, (SdoBaseHandle*)this
		);
	}
}

void LoginClient::ProcessCheckCodeToSendSmsRequestResponse(int reqeustCode, map<string, string>& keyValues)
{
	SdoBase_CheckCodeToSendSmsCallback callback = (SdoBase_CheckCodeToSendSmsCallback)m_mapCallback["SdoBase_CheckCodeToSendSmsCallback"];
	int needCheckCode = keyValues["nextAction"] == "8" ? 1 : 0;
	const char* url = keyValues["picUrl"].c_str();
	int width = 0;
	int height = 0;

	if (keyValues["imagecodeType"] == "4") {	//极验
		needCheckCode = 4;
		url = keyValues["gt_url"].c_str();
		width = atoi(keyValues["width"].c_str());
		height = atoi(keyValues["height"].c_str());
	}
	if (callback != nullptr)
	{
		callback(
			atoi(keyValues["resultCode"].c_str()), Utf8ToGbk(keyValues["failReason"].c_str()).c_str(), keyValues["smsSessionKey"].c_str()
			, url, needCheckCode, width, height, (SdoBaseHandle*)this
		);
	}
}

void LoginClient::ProcessUserPrivacyConfigRequestResponse(int reqeustCode, map<string, string>& keyValues)
{
	if (m_UserPrivacyConfigCallback != nullptr) {
		m_UserPrivacyConfigCallback(atoi(keyValues["resultCode"].c_str()), Utf8ToGbk(keyValues["failReason"].c_str()).c_str(), keyValues["privacyPolicyUrl"].c_str(),
			atoi(keyValues["privacyPolicyVersion"].c_str()), keyValues["servicerAgreementUrl"].c_str(), atoi(keyValues["serviceAgreementVersion"].c_str()));
	}
}

void LoginClient::ProcessFaceVerifyInitRequestResponse(int reqeustCode, map<string, string>& keyValues)
{
	if (m_FaceVerifyInitCallback != nullptr) {
		m_FaceVerifyInitCallback(atoi(keyValues["resultCode"].c_str()), Utf8ToGbk(keyValues["resultMsg"].c_str()).c_str(), atoi(keyValues["openFace"].c_str()), keyValues["imageData"].c_str(),
			keyValues["contextId"].c_str(), keyValues["phone"].c_str(), atoi(keyValues["showSkip"].c_str()));
	}
}

void LoginClient::ProcessGetFaceCodeResultRequestResponse(int reqeustCode, map<string, string>& keyValues)
{
	if (m_GetFaceCodeResultCallback != nullptr) {
		m_GetFaceCodeResultCallback(atoi(keyValues["resultCode"].c_str()), Utf8ToGbk(keyValues["resultMsg"].c_str()).c_str(), keyValues["nextAction"].c_str(), Utf8ToGbk(keyValues["promptMsg"].c_str()).c_str(), keyValues["ticket"].c_str());
	}
}

void LoginClient::ProcessSendActionRequestResponse(int reqeustCode, map<string, string>& keyValues)
{
	if (m_SendActionCallback != nullptr) {
		m_SendActionCallback(atoi(keyValues["resultCode"].c_str()), Utf8ToGbk(keyValues["resultMsg"].c_str()).c_str(), keyValues["ticket"].c_str());
	}
}

void LoginClient::ProcessGetTicketRequestResponse(int reqeustCode, map<string, string>& keyValues)
{
	if (m_GetTicketCallBack != nullptr) {
		m_GetTicketCallBack(atoi(keyValues["resultCode"].c_str()), Utf8ToGbk(keyValues["resultMsg"].c_str()).c_str(), keyValues["ticket"].c_str());
	}
}

void LoginClient::ProcessCreateWeGameOrderRequestResponse(int reqeustCode, map<string, string>& keyValues)
{
	if (m_CreateWeGameOrderCallback != nullptr) {
		m_CreateWeGameOrderCallback(atoi(keyValues["resultCode"].c_str()), Utf8ToGbk(keyValues["resultMsg"].c_str()).c_str(), keyValues["money"].c_str(), Utf8ToGbk(keyValues["itemName"].c_str()).c_str()
			, keyValues["payOrderNo"].c_str(), keyValues["paymentUrl"].c_str(), keyValues["orderNo"].c_str(), keyValues["wgOrderNo"].c_str());
	}
}

////三方登录界面灵活展示
void LoginClient::ProcessGetThirdLoginSkinRequestResponse(int reqeustCode, map<string, string>& keyValues)
{
	if (m_CreateThirdLoginSkinCallback != nullptr)
	{
		m_CreateThirdLoginSkinCallback(atoi(keyValues["resultCode"].c_str()), Utf8ToGbk(keyValues["resultMsg"].c_str()).c_str(), keyValues["status"].c_str());
	}
}

void LoginClient::ProcessCreateAuthCodeResponse(int reqeustCode, map<string, string>& keyValues)
{
	if (m_CreateAuchCodeCallback != nullptr)
	{
		m_CreateAuchCodeCallback(atoi(keyValues["resultCode"].c_str()), Utf8ToGbk(keyValues["failReason"].c_str()).c_str(), keyValues["ticket"].c_str(), keyValues["tgt"].c_str());
	}
}

//传奇4创建订单的htpp请求响应
void LoginClient::ProcessCreateCQ4OrderRequestResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_CreateCQ4OrderCallback != nullptr)
	{
		m_CreateCQ4OrderCallback(atoi(keyValues["resultCode"].c_str()), Utf8ToGbk(keyValues["resultMsg"].c_str()).c_str(), keyValues["orderNo"].c_str(), keyValues["money"].c_str(), Utf8ToGbk(keyValues["itemName"].c_str()).c_str()
			, keyValues["payOrderNo"].c_str(), keyValues["paymentUrl"].c_str());
	}
}

//传奇4查询订单的http请求响应
void LoginClient::ProcessCreateCQ4QueryRequestResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_CreateCQ4QueryCallback != nullptr)
	{
		m_CreateCQ4QueryCallback(atoi(keyValues["resultCode"].c_str()), Utf8ToGbk(keyValues["resultMsg"].c_str()).c_str(), keyValues["status"].c_str());
	}
}

void LoginClient::ProcessCreateSteamChannelOrderRequestResponse(int reqeustCode, map<string, string>& keyValues)
{
	if (m_CreateSteamChannelOrderCallback != nullptr) {
		//m_CreateSteamChannelOrderCallback(atoi(keyValues["resultCode"].c_str()),Utf8ToGbk(keyValues["resultMsg"].c_str()).c_str(),keyValues["money"].c_str(),Utf8ToGbk(keyValues["itemName"].c_str()).c_str()
		//	,keyValues["payOrderNo"].c_str(),keyValues["paymentUrl"].c_str(),keyValues["orderNo"].c_str(),keyValues["wgOrderNo"].c_str());
		m_CreateSteamChannelOrderCallback(atoi(keyValues["resultCode"].c_str()), Utf8ToGbk(keyValues["resultMsg"].c_str()).c_str(), keyValues["payOrderNo"].c_str());
	}
}

void LoginClient::ProcessCreateQQGameOrderRequestResponse(int reqeustCode, map<string, string>& keyValues)
{
	if (m_CreateQQGameOrderCallback != nullptr) {
		m_CreateQQGameOrderCallback(atoi(keyValues["resultCode"].c_str()), Utf8ToGbk(keyValues["resultMsg"].c_str()).c_str(), keyValues["url_params"].c_str());
	}
}

void LoginClient::ProcessQQGameIsLoginRequestResponse(int reqeustCode, map<string, string>& keyValues)
{
	if (m_QQGameIsLoginCallback != nullptr) {
		m_QQGameIsLoginCallback(atoi(keyValues["resultCode"].c_str()), Utf8ToGbk(keyValues["resultMsg"].c_str()).c_str());
	}
}

void LoginClient::ProcessWeGameStatusRequestResponse(int reqeustCode, map<string, string>& keyValues)
{
	if (m_WeGameStatusCallback != nullptr) {
		m_WeGameStatusCallback(atoi(keyValues["resultCode"].c_str()), Utf8ToGbk(keyValues["resultMsg"].c_str()).c_str(), keyValues["status"].c_str());
	}
}

void LoginClient::ProcessCreateLxOrderRequestResponse(int reqeustCode, map<string, string>& keyValues)
{
	if (m_CreateLxOrderCallback != nullptr) {
		m_CreateLxOrderCallback(atoi(keyValues["resultCode"].c_str()), Utf8ToGbk(keyValues["resultMsg"].c_str()).c_str(), keyValues["money"].c_str(), Utf8ToGbk(keyValues["itemName"].c_str()).c_str()
			, keyValues["payOrderNo"].c_str(), keyValues["payment_url"].c_str(), keyValues["orderNo"].c_str());
	}
}

void LoginClient::ProcessUeInitClientRequestResponse(int reqeustCode, map<string, string>& keyValues)
{
	m_requestProcess.SetFlowId(keyValues["ueFlowId"]);
	if (m_UeInitClientCallback != nullptr) {
		m_UeInitClientCallback(atoi(keyValues["resultCode"].c_str()), Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
			atoi(keyValues["operationModel"].c_str()), keyValues["key"].c_str(), keyValues["license"].c_str(), keyValues["passportIdAuth"].c_str(),
			keyValues["westoneAppId"].c_str());
	}
}

void LoginClient::ProcessCreateGetClientConfigRequestResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_GetClientConfigCallback)
	{
		std::map<std::string, std::string>::iterator it = keyValues.find("config");
		m_GetClientConfigCallback(atoi(keyValues["resultCode"].c_str()), Utf8ToGbk(keyValues["failReason"].c_str()).c_str(), it != keyValues.end() ? it->second.c_str() : "{}");
	}
}

void LoginClient::ProcessCreateGetGetGoDownConfigInitRequestResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_GoDownConfigInitCallback)
	{
		//把上下文存储起来
		sms_flowId = keyValues["flowId"];
		std::string nextAction = keyValues["nextAction"];
		std::string captchaParams = keyValues["captchaParams"];
		std::string safePhoneTip = keyValues["safePhoneTip"];

		std::string picUrl;

		std::map<std::string, std::string>::iterator it = keyValues.find("captchaParams");
		if (it != keyValues.end())
		{
			const std::string& captchaParamsStr = it->second;

			json_object* captchaObj = json_tokener_parse(captchaParamsStr.c_str());

			if (captchaObj && !is_error(captchaObj))
			{
				json_object* picUrlObj = json_object_object_get(captchaObj, "picUrl");
				if (picUrlObj && json_object_is_type(picUrlObj, json_type_string))
				{
					picUrl = json_object_get_string(picUrlObj);
				}

				json_object_put(captchaObj);
			}
		}


		m_GoDownConfigInitCallback(atoi(keyValues["resultCode"].c_str()), Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
			nextAction.c_str(), picUrl.c_str(), Utf8ToGbk(safePhoneTip.c_str()).c_str());
	}
}

void LoginClient::ProcessCreateGetGoDownSendSmsCheckCodeRequestResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_GoDownSendSmsCheckCodeCallback)
	{
		std::string nextAction = keyValues["nextAction"];
		std::string captchaParams = keyValues["captchaParams"];
		std::string safePhoneTip = keyValues["safePhoneTip"];

		std::string picUrl;

		std::map<std::string, std::string>::iterator it = keyValues.find("captchaParams");
		if (it != keyValues.end())
		{
			const std::string& captchaParamsStr = it->second;

			json_object* captchaObj = json_tokener_parse(captchaParamsStr.c_str());

			if (captchaObj && !is_error(captchaObj))
			{
				json_object* picUrlObj = json_object_object_get(captchaObj, "picUrl");
				if (picUrlObj && json_object_is_type(picUrlObj, json_type_string))
				{
					picUrl = json_object_get_string(picUrlObj);
				}

				json_object_put(captchaObj);
			}
		}

		m_GoDownSendSmsCheckCodeCallback(atoi(keyValues["resultCode"].c_str()), Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
			nextAction.c_str(), picUrl.c_str(), Utf8ToGbk(safePhoneTip.c_str()).c_str());
	}
}

void LoginClient::ProcessCreateGetGoDownconfirmSendSmsRequestResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_GoDownconfirmSendSmsCallback)
	{
		std::string nextAction = keyValues["nextAction"];
		m_GoDownconfirmSendSmsCallback(atoi(keyValues["resultCode"].c_str()), Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
			nextAction.c_str());
	}
}

void LoginClient::ProcessCreateGetGoDownconfirmLoginRequestResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_GoDownconfirmLoginCallback)
	{
		//把上下文存储起来
		std::string nextAction = keyValues["nextAction"];

		if (nextAction == "BIND_REALNAME")
		{
			//实名认证
			if (atoi(keyValues["resultCode"].c_str()) == 0)
			{
				m_requestProcess.m_tgt = keyValues["tgt"];
				if (reqeustCode == REQ_FcmLogin)
				{
					m_requestProcess.SetFlowId("");
				}
			}

			if (m_fcmLoginCallback != nullptr)
			{
				m_fcmLoginCallback(
					atoi(keyValues["resultCode"].c_str()),
					Utf8ToGbk(keyValues["realNameRemindTip"].c_str()).c_str(),
					atoi(keyValues["isNew"].c_str()) != 0,
					(SdoBaseHandle*)this
				);
			}

			SdoBase_FcmLoginCallback callback = (SdoBase_FcmLoginCallback)m_mapCallback["SdoBase_FcmLoginCallback"];
			if (callback != nullptr)
			{
				callback(
					atoi(keyValues["resultCode"].c_str()),
					Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
					atoi(keyValues["isNew"].c_str()) != 0,
					(SdoBaseHandle*)this
				);
			}
		}
		else if (nextAction == "VERIFY_SECURITY_CARD")
		{
			//动态密码安全卡认证

			//动密关于安全卡登录需要m_guid
			m_requestProcess.m_guid = keyValues["guid"];

			m_requestProcess.m_loginType = "2";

			if (m_dynamicLoginCallback != nullptr)
			{
				m_dynamicLoginCallback(
					atoi(keyValues["resultCode"].c_str()),
					Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
					2, //2是返回安全卡认证,现在登录器只留了一个动密关于安全手机 其他密保什么的已经移除
					atoi(keyValues["deviceType"].c_str()),
					Utf8ToGbk(keyValues["deviceDisplayType"].c_str()).c_str(),
					Utf8ToGbk(keyValues["challenge"].c_str()).c_str(),
					"go_down_login_type",
					keyValues["dynamicKey"].c_str(),
					keyValues["inputUserId"].c_str()
					, (SdoBaseHandle*)this
				);
			}

			SdoBase_DynamicLoginCallback callback = (SdoBase_DynamicLoginCallback)m_mapCallback["SdoBase_DynamicLoginCallback"];
			if (callback != nullptr)
			{
				callback(
					atoi(keyValues["resultCode"].c_str()),
					Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
					2, //2是返回安全卡认证,现在登录器只留了一个动密关于安全手机 其他密保什么的已经移除
					atoi(keyValues["deviceType"].c_str()),
					Utf8ToGbk(keyValues["deviceDisplayType"].c_str()).c_str(),
					Utf8ToGbk(keyValues["challenge"].c_str()).c_str(),
					"go_down_login_type",
					keyValues["dynamicKey"].c_str(),
					keyValues["inputUserId"].c_str()
					, (SdoBaseHandle*)this
				);
			}
		}
		else if (nextAction == "301")
		{
			//把上下文存储起来
			guard_dian_flowId = keyValues["flowId"];

			if (atoi(keyValues["resultCode"].c_str()) == 0)
			{
				m_requestProcess.m_tgt = keyValues["tgt"];
			}

			if (m_GuardDianCallback != nullptr)
			{
				m_GuardDianCallback(
					atoi(keyValues["resultCode"].c_str()),
					Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
					keyValues["flowId"].c_str(),
					(SdoBaseHandle*)this
				);
			}

			SdoBaseGuardDianCallback callback = (SdoBaseGuardDianCallback)m_mapCallback["SdoBaseGuardDianCallback"];
			if (callback != nullptr)
			{
				callback(
					atoi(keyValues["resultCode"].c_str()),
					Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
					keyValues["flowId"].c_str(),
					(SdoBaseHandle*)this
				);
			}
		}
		else if (nextAction == "210")
		{
			//需要展示激活逻辑
			if (atoi(keyValues["resultCode"].c_str()) == 0)
			{
				m_requestProcess.m_tgt = keyValues["tgt"];
			}

			if (m_ShowActiveLoginDlgCallback != nullptr)
			{
				m_ShowActiveLoginDlgCallback(
					atoi(keyValues["resultCode"].c_str()),
					Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
					keyValues["tgt"].c_str());
			}
		}
		else
		{
			if (atoi(keyValues["resultCode"].c_str()) == 0)
			{
				m_requestProcess.m_tgt = keyValues["tgt"];
			}

			//登录成功返回登录成功的参数
			m_GoDownconfirmLoginCallback(atoi(keyValues["resultCode"].c_str()), Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
				nextAction.c_str(), atoi(keyValues["isNew"].c_str()) != 0, keyValues["sndaId"].c_str(), keyValues["tgt"].c_str(),
				keyValues["ticket"].c_str(), keyValues["keepLoginKey"].c_str());
		}
	}
}

void LoginClient::ProcessCreateGetFastLoginLoginRequestResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_FastLoginLoginCallback)
	{
		//把上下文存储起来
		std::string nextAction = keyValues["nextAction"];

		if (nextAction == "BIND_REALNAME")
		{
			//实名认证
			if (atoi(keyValues["resultCode"].c_str()) == 0)
			{
				m_requestProcess.m_tgt = keyValues["tgt"];
				if (reqeustCode == REQ_FcmLogin)
				{
					m_requestProcess.SetFlowId("");
				}

			}

			if (m_fcmLoginCallback != nullptr)
			{
				m_fcmLoginCallback(
					atoi(keyValues["resultCode"].c_str()),
					Utf8ToGbk(keyValues["realNameRemindTip"].c_str()).c_str(),
					atoi(keyValues["isNew"].c_str()) != 0,
					(SdoBaseHandle*)this
				);
			}

			SdoBase_FcmLoginCallback callback = (SdoBase_FcmLoginCallback)m_mapCallback["SdoBase_FcmLoginCallback"];
			if (callback != nullptr)
			{
				callback(
					atoi(keyValues["resultCode"].c_str()),
					Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
					atoi(keyValues["isNew"].c_str()) != 0,
					(SdoBaseHandle*)this
				);
			}
		}
		else if (nextAction == "VERIFY_SECURITY_CARD")
		{
			//动态密码安全卡认证

			//动密关于安全卡登录需要m_guid
			m_requestProcess.m_guid = keyValues["guid"];

			m_requestProcess.m_loginType = "2";

			if (m_dynamicLoginCallback != nullptr)
			{
				m_dynamicLoginCallback(
					atoi(keyValues["resultCode"].c_str()),
					Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
					2, //2是返回安全卡认证,现在登录器只留了一个动密关于安全手机 其他密保什么的已经移除
					atoi(keyValues["deviceType"].c_str()),
					Utf8ToGbk(keyValues["deviceDisplayType"].c_str()).c_str(),
					Utf8ToGbk(keyValues["challenge"].c_str()).c_str(),
					"go_down_login_type",
					keyValues["dynamicKey"].c_str(),
					keyValues["inputUserId"].c_str()
					, (SdoBaseHandle*)this
				);
			}

			SdoBase_DynamicLoginCallback callback = (SdoBase_DynamicLoginCallback)m_mapCallback["SdoBase_DynamicLoginCallback"];
			if (callback != nullptr)
			{
				callback(
					atoi(keyValues["resultCode"].c_str()),
					Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
					2, //2是返回安全卡认证,现在登录器只留了一个动密关于安全手机 其他密保什么的已经移除
					atoi(keyValues["deviceType"].c_str()),
					Utf8ToGbk(keyValues["deviceDisplayType"].c_str()).c_str(),
					Utf8ToGbk(keyValues["challenge"].c_str()).c_str(),
					"go_down_login_type",
					keyValues["dynamicKey"].c_str(),
					keyValues["inputUserId"].c_str(),
					(SdoBaseHandle*)this
				);
			}
		}
		else if (nextAction == "210")
		{
			//需要展示激活逻辑
			if (atoi(keyValues["resultCode"].c_str()) == 0)
			{
				m_requestProcess.m_tgt = keyValues["tgt"];
			}

			if (m_ShowActiveLoginDlgCallback != nullptr)
			{
				m_ShowActiveLoginDlgCallback(
					atoi(keyValues["resultCode"].c_str()),
					Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
					keyValues["tgt"].c_str());
			}
		}
		else
		{
			if (atoi(keyValues["resultCode"].c_str()) == 0)
			{
				m_requestProcess.m_tgt = keyValues["tgt"];
			}

			//登录成功返回登录成功的参数
			m_FastLoginLoginCallback(atoi(keyValues["resultCode"].c_str()), Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
				nextAction.c_str(), atoi(keyValues["isNew"].c_str()) != 0, keyValues["sndaId"].c_str(), keyValues["tgt"].c_str(),
				keyValues["ticket"].c_str(), auto_login_fast_key.c_str());
		}
	}
}

void LoginClient::ProcessCreateGetTicketLoginRequestResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_GetTicketLoginCallback)
	{
		m_GetTicketLoginCallback(atoi(keyValues["resultCode"].c_str()), Utf8ToGbk(keyValues["failReason"].c_str()).c_str(), keyValues["ticket"].c_str());
	}
}

void LoginClient::ProcessCreateFastLoginActiveDeleteAccountRequestResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_FastLoginActiveDeleteAccountLoginCallback)
	{
		m_FastLoginActiveDeleteAccountLoginCallback(atoi(keyValues["resultCode"].c_str()), Utf8ToGbk(keyValues["failReason"].c_str()).c_str());
	}
}

void LoginClient::ProcessCreateCheckAccountLoginTypeRequestResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_CheckAccountTypeLoginCallback)
	{
		m_CheckAccountTypeLoginCallback(atoi(keyValues["resultCode"].c_str()), Utf8ToGbk(keyValues["failReason"].c_str()).c_str(), atoi(keyValues["acctType"].c_str()));
	}
}

void LoginClient::ProcessCreateCheckCalculateRequestResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_CalculateCallback)
	{
		std::map<std::string, std::string>::iterator it = keyValues.find("config");
		m_CalculateCallback(atoi(keyValues["resultCode"].c_str()), Utf8ToGbk(keyValues["failReason"].c_str()).c_str(), it != keyValues.end() ? it->second.c_str() : "{}");
	}
}

void LoginClient::ProcessCreateCheckInitAdvRequestResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_InitAdvCallback)
	{
		//判断是否网络超时,接口请求没有发出去
		int code = atoi(keyValues["resultCode"].c_str());
		std::string safeMsg = keyValues["failReason"];
		TRACEI("GunionInitAdv response -> code: %d, safeMsg: %s", code, safeMsg.c_str());
		m_InitAdvCallback(atoi(keyValues["resultCode"].c_str()), Utf8ToGbk(keyValues["failReason"].c_str()).c_str(), keyValues["traceId"].c_str());
	}
}

void LoginClient::ProcessCreateGuardDianSendSmsRequestResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_GuardDianSendSmsCallback)
	{
		//把监护人上下文存储起来
		guard_dian_flowId = keyValues["flowId"];
		std::string nextAction = keyValues["nextAction"];
		std::string captchaParams = keyValues["captchaParams"];

		std::string picUrl;

		std::map<std::string, std::string>::iterator it = keyValues.find("captchaParams");
		if (it != keyValues.end())
		{
			const std::string& captchaParamsStr = it->second;

			json_object* captchaObj = json_tokener_parse(captchaParamsStr.c_str());

			if (captchaObj && !is_error(captchaObj))
			{
				json_object* picUrlObj = json_object_object_get(captchaObj, "picUrl");
				if (picUrlObj && json_object_is_type(picUrlObj, json_type_string))
				{
					picUrl = json_object_get_string(picUrlObj);
				}

				json_object_put(captchaObj);
			}
		}

		m_GuardDianSendSmsCallback(atoi(keyValues["resultCode"].c_str()), Utf8ToGbk(keyValues["failReason"].c_str()).c_str()
			, guard_dian_flowId.c_str(), nextAction.c_str(), picUrl.c_str(), atoi(keyValues["captchaType"].c_str()));
	}
}

void LoginClient::ProcessCreateGuardDianSendSmsCheckCodeRequestResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_GuardDianSendSmsCheckCodeCallback)
	{
		//把监护人上下文存储起来
		guard_dian_flowId = keyValues["flowId"];
		std::string nextAction = keyValues["nextAction"];
		std::string captchaParams = keyValues["captchaParams"];

		std::string picUrl;

		std::map<std::string, std::string>::iterator it = keyValues.find("captchaParams");
		if (it != keyValues.end())
		{
			const std::string& captchaParamsStr = it->second;

			json_object* captchaObj = json_tokener_parse(captchaParamsStr.c_str());

			if (captchaObj && !is_error(captchaObj))
			{
				json_object* picUrlObj = json_object_object_get(captchaObj, "picUrl");
				if (picUrlObj && json_object_is_type(picUrlObj, json_type_string))
				{
					picUrl = json_object_get_string(picUrlObj);
				}

				json_object_put(captchaObj);
			}
		}

		m_GuardDianSendSmsCheckCodeCallback(atoi(keyValues["resultCode"].c_str()), Utf8ToGbk(keyValues["failReason"].c_str()).c_str()
			, guard_dian_flowId.c_str(), nextAction.c_str(), picUrl.c_str(), atoi(keyValues["captchaType"].c_str()));
	}
}

void LoginClient::ProcessCreateGuardDianConfrimSendSmsResultRequestResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_GuardDianConfrimSendSmsResultCallback)
	{
		m_requestProcess.m_tgt = keyValues["tgt"];

		std::string nextAction = keyValues["nextAction"];
		std::string checkAgeRemindTip = keyValues["checkAgeRemindTip"];

		m_GuardDianConfrimSendSmsResultCallback(atoi(keyValues["resultCode"].c_str()), Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
			atoi(keyValues["needCheckAge"].c_str()), checkAgeRemindTip.c_str(), nextAction.c_str(),
			keyValues["tgt"].c_str(), keyValues["ticket"].c_str(), keyValues["sndaId"].c_str(),
			keyValues["inputUserId"].c_str(), atoi(keyValues["isNew"].c_str()) != 0, keyValues["keepLoginKey"].c_str());
	}
}

void LoginClient::ProcessCreateActiveCodeLoginResultRequestResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_ActiveLoginCallback)
	{
		if ((keyValues["tgt"]).size() > 0)
		{
			m_requestProcess.m_tgt = keyValues["tgt"];
		}

		std::string nextAction = keyValues["nextAction"];
		std::string checkAgeRemindTip = keyValues["checkAgeRemindTip"];

		m_ActiveLoginCallback(atoi(keyValues["resultCode"].c_str()), Utf8ToGbk(keyValues["failReason"].c_str()).c_str(),
			nextAction.c_str(), atoi(keyValues["isNew"].c_str()) != 0, keyValues["sndaId"].c_str(), keyValues["tgt"].c_str(),
			keyValues["ticket"].c_str(), auto_login_fast_key.c_str());
	}
}

void LoginClient::ProcessCreateGetFaceRecognitionUrlRequestResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_FaceRecognitionUrlCallback)
	{
		std::string failReasonGbk = Utf8ToGbk(keyValues["failReason"].c_str());
		m_FaceRecognitionUrlCallback(atoi(keyValues["resultCode"].c_str()), failReasonGbk.c_str(), keyValues["faceRecognitionUrl"].c_str());
	}
}

void LoginClient::ProcessCreateQueryFaceVerifyTicketStatusRequestResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_QueryFaceVerifyTicketStatusCallback)
	{
		//{
		//	"resultCode": 0,
		//		"resultMsg": "success",
		//		"data": {
		//			"verifyResult": true,
		//				"failReason": null
		//	}
		//}
		//{
		//	"resultCode": 0,
		//		"resultMsg": "success",
		//		"data": {
		//			"verifyResult": false,
		//				"failReason": "人脸识别失败，请重试"
		//	}
		//}
		std::string resultMsgGbk = Utf8ToGbk(keyValues["resultMsg"].c_str());
		std::string failReasonGbk = Utf8ToGbk(keyValues["failReason"].c_str());

		bool verifyResult = false;
		std::string& v1 = keyValues["verifyResult"];
		if (!v1.empty())
		{
			if (v1 == "true" || v1 == "1")
				verifyResult = true;
			else
				verifyResult = false;
		}

		bool needPopup = false;
		std::string& v2 = keyValues["needPopup"];
		if (!v2.empty())
		{
			if (v2 == "true" || v2 == "1")
				needPopup = true;
			else
				needPopup = false;
		}

		m_QueryFaceVerifyTicketStatusCallback(atoi(keyValues["resultCode"].c_str()), resultMsgGbk.c_str(), verifyResult, failReasonGbk.c_str(), needPopup);
	}
}

void LoginClient::ProcessCreateGetFaceHolderRegistrationUrlRequestResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_HolderRegistrationUrlCallback)
	{
		std::string failReasonGbk = Utf8ToGbk(keyValues["failReason"].c_str());
		m_HolderRegistrationUrlCallback(atoi(keyValues["resultCode"].c_str()), failReasonGbk.c_str(), keyValues["holderRegistrationUrl"].c_str());
	}
}

// ---------------------- Gunion登录相关响应处理 ----------------------
void LoginClient::ProcessCreateGunionGetPublicKeyResponse(
	int requestCode,
	map<string, string>& keyValues)
{
	TRACET();

	// 1. 取 code
	int code = ERROR_RESPONSE;
	auto itCode = keyValues.find("code");
	if (itCode != keyValues.end())
	{
		code = atoi(itCode->second.c_str());
	}

	// 2. 生成 safeMsg
	std::string safeMsg;
	auto itMsg = keyValues.find("msg");
	if (itMsg != keyValues.end())
	{
		safeMsg = GetSafeMsg(itMsg->second.c_str(), code);
	}
	else
	{
		safeMsg = GetSafeMsg("", code);
	}
	TRACEI("GunionGetPublicKey response -> code: %d, safeMsg: %s", code, safeMsg.c_str());
	// 3. code 失败 → 通知握手失败
	if (code != 0)
	{
		if (m_gunionHandshake)
		{
			m_gunionHandshake->OnPublicKeyFailed(code);
		}
		return;
	}

	// 4. 取 RSA 公钥
	auto itKey = keyValues.find("key");
	if (itKey == keyValues.end() || itKey->second.empty())
	{
		// 公钥缺失 → 协议失败
		if (m_gunionHandshake)
		{
			m_gunionHandshake->OnPublicKeyFailed(code);
		}
		return;
	}

	// 5. 构造完整 PEM 公钥
	m_gunionPublicKey.clear();
	m_gunionPublicKey.reserve(itKey->second.size() + 64);

	m_gunionPublicKey += RSA_PUBLIC_KEY_PREFIX;
	m_gunionPublicKey += itKey->second;
	m_gunionPublicKey += RSA_PUBLIC_KEY_SUFFIX;

	// 6. 通知握手状态机：PublicKey 已就绪
	if (m_gunionHandshake && !m_gunionHandshake->IsFinished())
	{
		m_gunionHandshake->OnPublicKeyReady(m_gunionPublicKey);
	}
}

void LoginClient::ProcessCreateGunionHandshakeResponse(
	int requestCode,
	map<string, string>& keyValues)
{
	TRACET();

	// 1. 取 code
	int code = ERROR_RESPONSE;
	auto itCode = keyValues.find("code");
	if (itCode != keyValues.end())
	{
		code = atoi(itCode->second.c_str());
	}
	TRACEI("GunionHandshake response -> code: %d", code);
	// 2. code 失败 → 通知握手失败
	if (code != 0)
	{
		if (m_gunionHandshake)
		{
			m_gunionHandshake->OnHandShakeFailed(code);
		}
		return;
	}

	// 3. 取 token
	auto itToken = keyValues.find("token");
	if (itToken == keyValues.end() || itToken->second.empty())
	{
		// token 缺失 → 视为握手失败
		if (m_gunionHandshake)
		{
			m_gunionHandshake->OnHandShakeFailed(ERROR_INVALID_TOKEN);
		}
		return;
	}

	// 4. 保存会话 token
	m_gunionToken = itToken->second;

	// 5. 通知握手成功
	if (m_gunionHandshake)
	{
		m_gunionHandshake->OnHandShakeSuccess();
	}
}

void LoginClient::ProcessCreateGunionWegameInitResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();

	if (m_CreateGunionWegameInitCallback)
	{
		std::string offcial_url_button;

		// 解析login_button
		struct json_object* root = json_tokener_parse(keyValues["login_button"].c_str());
		if (root != nullptr && !is_error(root))
		{
			// 检查是否是数组
			if (json_object_get_type(root) == json_type_array)
			{
				int array_len = json_object_array_length(root);
				if (array_len > 1) // 确保数组中有至少两个元素
				{
					// 获取数组中的第二个元素
					struct json_object* item = json_object_array_get_idx(root, 0);
					if (item != nullptr && json_object_get_type(item) == json_type_string)
					{
						const char* item_value = json_object_get_string(item);
						if (std::string(item_value) == "officialPt")
						{
							offcial_url_button = "officialPt";
						}
						else
						{
							offcial_url_button = "official";
						}
					}
				}
			}
			// 释放 JSON 对象
			json_object_put(root);
		}

		// 解析 pt_input_box_prompts
		std::string sms_login_prompt, static_login_prompt;

		if (keyValues.find("pt_input_box_prompts") != keyValues.end())
		{
			std::string pt_input_box_prompts_str = keyValues["pt_input_box_prompts"];
			struct json_object* pt_root = json_tokener_parse(pt_input_box_prompts_str.c_str());
			if (pt_root != nullptr && !is_error(pt_root))
			{
				struct json_object* sms_obj = json_object_object_get(pt_root, "sms_login_prompt");
				if (sms_obj != nullptr)
				{
					sms_login_prompt = json_object_get_string(sms_obj);
				}

				struct json_object* static_obj = json_object_object_get(pt_root, "static_login_prompt");
				if (static_obj != nullptr)
				{
					static_login_prompt = json_object_get_string(static_obj);
				}

				json_object_put(pt_root);
			}
		}

		new_device_id_server = keyValues["new_device_id_server"];
		//判断是否网络超时,接口请求没有发出去
		int code = atoi(keyValues["code"].c_str());
		std::string safeMsg = GetSafeMsg(keyValues["msg"].c_str(), code);
		TRACEI("GunionWegameInit response -> code: %d, safeMsg: %s", code, safeMsg.c_str());
		m_CreateGunionWegameInitCallback(
			code,
			safeMsg.c_str(),
			keyValues["weixin_appId"].c_str(),
			keyValues["weixin_key"].c_str(),
			keyValues["qq_appId"].c_str(),
			keyValues["qq_key"].c_str(),
			keyValues["weibo_appKey"].c_str(),
			keyValues["weibo_redirectUrl"].c_str(),
			(Utf8ToGbk(keyValues["voicetip_one"]).c_str()),
			(Utf8ToGbk(keyValues["voicetip_two"]).c_str()),
			keyValues["voicetip_button"].c_str(),
			keyValues["game_name"].c_str(),
			keyValues["brand_logo"].c_str(),
			keyValues["brand_name"].c_str(),
			keyValues["is_match"].c_str(),
			keyValues["new_device_id_server"].c_str(),
			offcial_url_button.c_str(),
			keyValues["login_icon"].c_str(),
			keyValues["official_auth_func"].c_str(),
			atoi(keyValues["accountDeletionPeriod"].c_str()),
			(Utf8ToGbk(sms_login_prompt).c_str()),
			(Utf8ToGbk(static_login_prompt).c_str()),
			m_gunionToken.c_str(), m_gunionRandKey.c_str());
	}
}

void LoginClient::ProcessGunionWegameLoginResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	int resultCode = atoi(keyValues["code"].c_str());
	if (m_CreateGunionWegameLoginCallback != nullptr)
	{
		//判断是否网络超时,接口请求没有发出去
		int code = atoi(keyValues["code"].c_str());
		std::string safeMsg = GetSafeMsg(keyValues["msg"].c_str(), code);
		TRACEI("GunionWegameLogin response -> code: %d, safeMsg: %s", code, safeMsg.c_str());
		m_CreateGunionWegameLoginCallback(
			code,
			safeMsg.c_str(),
			keyValues["userid"].c_str(),
			keyValues["ticket"].c_str(),
			atoi(keyValues["isNewUser"].c_str()) != 0,
			atoi(keyValues["showBindPhone"].c_str()),
			atoi(keyValues["canSkipBindPhone"].c_str()),
			atoi(keyValues["skipBindPhonePeriod"].c_str()),
			atoi(keyValues["canChangePhone"].c_str()),
			keyValues["bindPhone"].c_str(),
			keyValues["bindPhoneSndaId"].c_str(),
			(SdoBaseHandle*)this
		);
	}

	SdoBase_CreateGunionWegameLoginCallback callback = (SdoBase_CreateGunionWegameLoginCallback)m_mapCallback["SdoBase_LoginResultCallback"];
	if (callback != nullptr)
	{
		int code = atoi(keyValues["code"].c_str());
		std::string safeMsg = GetSafeMsg(keyValues["msg"].c_str(), code);

		callback(
			code,
			safeMsg.c_str(),
			keyValues["userid"].c_str(),
			keyValues["ticket"].c_str(),
			atoi(keyValues["isNewUser"].c_str()) != 0,
			atoi(keyValues["showBindPhone"].c_str()),
			atoi(keyValues["canSkipBindPhone"].c_str()),
			atoi(keyValues["skipBindPhonePeriod"].c_str()),
			atoi(keyValues["canChangePhone"].c_str()),
			keyValues["bindPhone"].c_str(),
			keyValues["bindPhoneSndaId"].c_str(),
			(SdoBaseHandle*)this
		);
	}
}

void LoginClient::ProcessCreateGunionWegameSmsSendResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_CreateGunionWegameSmsSendCallback)
	{
		const char* back_url_union;
		gunion_wegame_check_sms_send_CodeGuid = keyValues["checkCodeGuid"];

		//极验参数
		std::string success;
		std::string new_captcha;
		std::string challenge; //极验流水号
		std::string gt;        //极验id
		int gt_width = 0;
		int gt_height = 0;

		std::string ghome_back_url;

		const char* back_url = (keyValues["checkCodeUrl"]).c_str();
		ghome_back_url = back_url;

		int needCheckCode = keyValues["nextAction"] == "8" ? 1 : 0;
		int width = 0;
		int height = 0;

		if (keyValues["nextAction"] == "8")
		{
			struct json_object* root = json_tokener_parse(keyValues["captchaParams"].c_str());

			// 解析JSON字符串
			if (!is_error(root))
			{

				if (keyValues["imagecodeType"] == "4")
				{
					//极验
					json_object* gtData = json_object_object_get(root, "gtData");

					if (gtData)
					{
						json_object* json_success = json_object_object_get(gtData, "success");
						json_object* json_new_captcha = json_object_object_get(gtData, "new_captcha");
						json_object* json_challenge = json_object_object_get(gtData, "challenge");
						json_object* json_gt = json_object_object_get(gtData, "gt");
						json_object* json_gt_url = json_object_object_get(gtData, "gt_url");

						if (json_success)
						{
							success = json_object_get_string(json_success);
						}
						if (json_new_captcha)
						{
							new_captcha = json_object_get_string(json_new_captcha);
						}
						if (json_challenge)
						{
							challenge = json_object_get_string(json_challenge);
						}
						if (json_gt)
						{
							gt = json_object_get_string(json_gt);
						}
						if (json_gt_url)
						{
							back_url = json_object_get_string(json_gt_url);
							ghome_back_url = back_url;
						}
						//极验
						needCheckCode = 4;
					}
				}
				else if (keyValues["imagecodeType"] == "1")
				{
					//普通图验
					json_object* json_picUrl = json_object_object_get(root, "picUrl");
					if (json_picUrl)
					{
						back_url = json_object_get_string(json_picUrl);
						ghome_back_url = back_url;
					}
				}
			}

			// 释放 JSON 对象
			json_object_put(root);
		}

		if (keyValues["nextAction"] == "8")
		{
			if (keyValues["imagecodeType"] == "1")
			{
				gt_width = 140;
				gt_height = 37;
			}
			else if (keyValues["imagecodeType"] == "4")
			{
				gt_width = atoi((keyValues["gt_width"]).c_str());
				gt_height = atoi((keyValues["gt_height"]).c_str());
			}
		}

		back_url_union = ghome_back_url.c_str();

		//判断是否网络超时,接口请求没有发出去
		int code = atoi(keyValues["code"].c_str());
		std::string safeMsg = GetSafeMsg(keyValues["msg"].c_str(), code);
		TRACEI("GunionWegameSmsSend response -> code: %d, safeMsg: %s", code, safeMsg.c_str());
		{
			if (m_CreateGunionWegameSmsSendCallback != nullptr)
			{
				m_CreateGunionWegameSmsSendCallback(
					code,
					safeMsg.c_str(),
					back_url_union,
					needCheckCode,
					gt_width,
					gt_height,
					(SdoBaseHandle*)this);
			}
		}
	}
}

void LoginClient::ProcessCreateGunionWegamePicSmsSendResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_CreateGunionWegamePicSmsSendCallback)
	{
		gunion_wegame_check_sms_send_CodeGuid = keyValues["checkCodeGuid"];

		//极验参数
		std::string success;
		std::string new_captcha;
		std::string challenge; //极验流水号
		std::string gt;        //极验id
		std::string gt_width;
		std::string gt_height;

		std::string ghome_back_url;

		const char* back_url = (keyValues["checkCodeUrl"]).c_str();
		ghome_back_url = back_url;

		int needCheckCode = keyValues["nextAction"] == "8" ? 1 : 0;
		int width = 0;
		int height = 0;

		if (keyValues["nextAction"] == "8")
		{
			struct json_object* root = json_tokener_parse(keyValues["captchaParams"].c_str());

			// 解析JSON字符串
			if (!is_error(root))
			{

				if (keyValues["imagecodeType"] == "4")
				{
					//极验
					json_object* gtData = json_object_object_get(root, "gtData");

					if (gtData)
					{
						json_object* json_success = json_object_object_get(gtData, "success");
						json_object* json_new_captcha = json_object_object_get(gtData, "new_captcha");
						json_object* json_challenge = json_object_object_get(gtData, "challenge");
						json_object* json_gt = json_object_object_get(gtData, "gt");
						json_object* json_gt_url = json_object_object_get(gtData, "gt_url");

						if (json_success)
						{
							success = json_object_get_string(json_success);
						}
						if (json_new_captcha)
						{
							new_captcha = json_object_get_string(json_new_captcha);
						}
						if (json_challenge)
						{
							challenge = json_object_get_string(json_challenge);
						}
						if (json_gt)
						{
							gt = json_object_get_string(json_gt);
						}
						if (json_gt_url)
						{
							back_url = json_object_get_string(json_gt_url);
							ghome_back_url = back_url;
						}
						//极验
						needCheckCode = 4;
					}
				}
				else if (keyValues["imagecodeType"] == "1")
				{
					//普通图验
					json_object* json_picUrl = json_object_object_get(root, "picUrl");
					if (json_picUrl)
					{
						back_url = json_object_get_string(json_picUrl);
						ghome_back_url = back_url;
					}

				}
			}

			// 释放 JSON 对象
			json_object_put(root);
		}

		//判断是否网络超时,接口请求没有发出去
		int code = atoi(keyValues["code"].c_str());
		std::string safeMsg = GetSafeMsg(keyValues["msg"].c_str(), code);
		TRACEI("GunionWegamePicSmsSend response -> code: %d, safeMsg: %s", code, safeMsg.c_str());
		{
			if (m_CreateGunionWegamePicSmsSendCallback != nullptr)
			{
				m_CreateGunionWegamePicSmsSendCallback(
					code,
					safeMsg.c_str(),
					ghome_back_url.c_str(),
					needCheckCode,
					140,
					37,
					(SdoBaseHandle*)this);
			}
		}
	}
}

void LoginClient::ProcessCreateGunionWegameThirdAccountBindPhoneResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_CreateGunionWegameThirdAccountBindPhoneCallback)
	{
		const char* realMessage = "";
		std::string realTextGbk;
		std::map<std::string, std::string>::iterator it = keyValues.find("realInfoFlowId");
		if (it != keyValues.end())
		{
			realTextGbk = Utf8ToGbk(it->second); // 转换成 GBK
			realMessage = realTextGbk.c_str();
		}

		//判断是否网络超时,接口请求没有发出去
		int code = atoi(keyValues["code"].c_str());
		std::string safeMsg = GetSafeMsg(keyValues["msg"].c_str(), code);
		TRACEI("GunionWegameThirdAccountBindPhone response -> code: %d, safeMsg: %s", code, safeMsg.c_str());
		{
			if (m_CreateGunionWegameThirdAccountBindPhoneCallback != nullptr)
			{
				m_CreateGunionWegameThirdAccountBindPhoneCallback(
					code,
					safeMsg.c_str(),
					keyValues["bindPhone"].c_str(),
					keyValues["bindPhoneSndaId"].c_str(),
					keyValues["thirdNickName"].c_str(),
					realMessage);
			}
		}
	}
}

void LoginClient::ProcessCreateGunionWegameFillRealinfoResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_CreateGunionWegameFillRealinfoCallback)
	{
		//判断是否网络超时,接口请求没有发出去
		int code = atoi(keyValues["code"].c_str());
		std::string safeMsg = GetSafeMsg(keyValues["msg"].c_str(), code);
		TRACEI("GunionWegameFillRealinfo response -> code: %d, safeMsg: %s", code, safeMsg.c_str());
		{
			if (m_CreateGunionWegameFillRealinfoCallback != nullptr)
			{
				m_CreateGunionWegameFillRealinfoCallback(
					code,
					safeMsg.c_str());
			}
		}
	}
}

void LoginClient::ProcessCreateGunionWegameGetTicketResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	{
		if (m_CreateGunionWegameGetTicketCallback != nullptr)
		{
			//判断是否网络超时,接口请求没有发出去
			int code = atoi(keyValues["code"].c_str());
			std::string safeMsg = GetSafeMsg(keyValues["msg"].c_str(), code);
			TRACEI("GunionWegameGetTicket response -> code: %d, safeMsg: %s", code, safeMsg.c_str());
			m_CreateGunionWegameGetTicketCallback(
				code,
				safeMsg.c_str(),
				keyValues["ticket"].c_str());
		}
	}
}

void LoginClient::ProcessCreateGunionWegameGetPayTicketResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	{
		if (m_CreateGunionWegameGetPayTicketCallback != nullptr)
		{
			//判断是否网络超时,接口请求没有发出去
			int code = atoi(keyValues["code"].c_str());
			std::string safeMsg = GetSafeMsg(keyValues["msg"].c_str(), code);
			TRACEI("GunionWegameegameGetPayTicket response -> code: %d, safeMsg: %s", code, safeMsg.c_str());
			m_CreateGunionWegameGetPayTicketCallback(
				code,
				safeMsg.c_str(),
				keyValues["ticket"].c_str());
		}
	}
}
// ---------------------- Gunion-WeGame 登录相关响应处理 ----------------------

// ---------------------- Gunion- 登录相关响应处理 ----------------------
void LoginClient::ProcessCreateGunionDoPrivateResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_CreateDoPirvateCallback)
	{
		//判断是否网络超时,接口请求没有发出去
		int code = atoi(keyValues["return_code"].c_str());
		std::string safeMsg = GetSafeMsg(keyValues["return_message"].c_str(), code);
		TRACEI("GunionDoPrivate response -> code: %d, safeMsg: %s", code, safeMsg.c_str());

		std::string url;

		struct json_object* root = json_tokener_parse(keyValues["agreements"].c_str());
		// 解析JSON字符串
		if (!is_error(root))
		{
			// 检查是否是数组
			if (json_object_get_type(root) == json_type_array)
			{
				int array_len = json_object_array_length(root);
				for (int i = 0; i < array_len; i++)
				{
					struct json_object* item = json_object_array_get_idx(root, i);
					struct json_object* type_obj = json_object_object_get(item, "type");

					if (type_obj != NULL && json_object_get_type(type_obj) == json_type_int)
					{
						int type = json_object_get_int(type_obj);
						if (type == 0)
						{
							// 如果 type 为 0，获取 URL
							struct json_object* url_obj = json_object_object_get(item, "url");
							if (url_obj != NULL && json_object_get_type(url_obj) == json_type_string)
							{
								url = json_object_get_string(url_obj);
								break;
							}
						}
					}
				}
			}
			// 释放 JSON 对象
			json_object_put(root);
		}

		m_CreateDoPirvateCallback(
			atoi(keyValues["return_code"].c_str()),
			(Utf8ToGbk(keyValues["return_message"]).c_str()),
			url.c_str());
	}
}

void LoginClient::ProcessCreateGunionGetGameAgreementUrlContentResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_CreateCheckGameAgreementUrlContentCallback)
	{
		//判断是否网络超时,接口请求没有发出去
		int code = atoi(keyValues["return_code"].c_str());
		std::string safeMsg = GetSafeMsg(keyValues["return_message"].c_str(), code);
		TRACEI("GunioGetGameAgreementUrlContent response -> code: %d, safeMsg: %s", code, safeMsg.c_str());
		std::string agreementServiceUrl;
		std::string agreementServiceCon;
		std::string agreementPrivacyUrl;
		std::string agreementPrivacyCon;
		// agreements 是一个数组 JSON 字符串
		struct json_object* root = json_tokener_parse(keyValues["agreements"].c_str());
		if (!is_error(root))
		{
			// 检查是否是数组
			if (json_object_get_type(root) == json_type_array)
			{
				int array_len = json_object_array_length(root);
				for (int i = 0; i < array_len; i++)
				{
					struct json_object* item = json_object_array_get_idx(root, i);
					if (!item) continue;

					struct json_object* type_obj = json_object_object_get(item, "type");
					if (!type_obj) continue;

					int type = json_object_get_int(type_obj);

					struct json_object* url_obj = json_object_object_get(item, "url");
					struct json_object* con_obj = json_object_object_get(item, "content");

					const char* url = (url_obj) ? json_object_get_string(url_obj) : "";
					const char* content = (con_obj) ? json_object_get_string(con_obj) : "";

					if (type == 1)  // 服务协议
					{
						agreementServiceUrl = url;
						agreementServiceCon = content;
					}
					else if (type == 2)  // 隐私协议
					{
						agreementPrivacyUrl = url;
						agreementPrivacyCon = content;
					}
				}
			}

			json_object_put(root);
		}

		m_CreateCheckGameAgreementUrlContentCallback(
			atoi(keyValues["return_code"].c_str()),
			(Utf8ToGbk(keyValues["return_message"]).c_str()),
			agreementServiceUrl.c_str(), Utf8ToGbk(agreementServiceCon).c_str(),
			agreementPrivacyUrl.c_str(), Utf8ToGbk(agreementPrivacyCon).c_str());
	}
}

void LoginClient::ProcessCreateGunionSmsSendResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_CreateDoSmsSendCallback)
	{
		const char* back_url_union;
		gunion_wegame_check_sms_send_CodeGuid = keyValues["checkCodeGuid"];

		//极验参数
		std::string success;
		std::string new_captcha;
		std::string challenge; //极验流水号
		std::string gt;        //极验id
		int gt_width = 0;
		int gt_height = 0;

		std::string ghome_back_url;

		const char* back_url = (keyValues["checkCodeUrl"]).c_str();
		ghome_back_url = back_url;

		int needCheckCode = keyValues["nextAction"] == "8" ? 1 : 0;
		int width = 0;
		int height = 0;

		if (keyValues["nextAction"] == "8")
		{
			struct json_object* root = json_tokener_parse(keyValues["captchaParams"].c_str());

			// 解析JSON字符串
			if (!is_error(root))
			{

				if (keyValues["imagecodeType"] == "4")
				{
					//极验
					json_object* gtData = json_object_object_get(root, "gtData");

					if (gtData)
					{
						json_object* json_success = json_object_object_get(gtData, "success");
						json_object* json_new_captcha = json_object_object_get(gtData, "new_captcha");
						json_object* json_challenge = json_object_object_get(gtData, "challenge");
						json_object* json_gt = json_object_object_get(gtData, "gt");
						json_object* json_gt_url = json_object_object_get(gtData, "gt_url");

						if (json_success)
						{
							success = json_object_get_string(json_success);
						}
						if (json_new_captcha)
						{
							new_captcha = json_object_get_string(json_new_captcha);
						}
						if (json_challenge)
						{
							challenge = json_object_get_string(json_challenge);
						}
						if (json_gt)
						{
							gt = json_object_get_string(json_gt);
						}
						if (json_gt_url)
						{
							back_url = json_object_get_string(json_gt_url);
							ghome_back_url = back_url;
						}
						//极验
						needCheckCode = 4;
					}
				}
				else if (keyValues["imagecodeType"] == "1")
				{
					//普通图验
					json_object* json_picUrl = json_object_object_get(root, "picUrl");
					if (json_picUrl)
					{
						back_url = json_object_get_string(json_picUrl);
						ghome_back_url = back_url;
					}
				}
			}

			// 释放 JSON 对象
			json_object_put(root);
		}

		if (keyValues["nextAction"] == "8")
		{
			if (keyValues["imagecodeType"] == "1")
			{
				gt_width = 140;
				gt_height = 37;
			}
			else if (keyValues["imagecodeType"] == "4")
			{
				gt_width = atoi((keyValues["gt_width"]).c_str());
				gt_height = atoi((keyValues["gt_height"]).c_str());
			}
		}

		back_url_union = ghome_back_url.c_str();

		//判断是否网络超时,接口请求没有发出去
		int code = atoi(keyValues["code"].c_str());
		std::string safeMsg = GetSafeMsg(keyValues["msg"].c_str(), code);
		TRACEI("GunionSmsSend response -> code: %d, safeMsg: %s", code, safeMsg.c_str());
		{
			if (m_CreateDoSmsSendCallback != nullptr)
			{
				m_CreateDoSmsSendCallback(
					code,
					safeMsg.c_str(),
					back_url_union,
					needCheckCode,
					gt_width,
					gt_height,
					(SdoBaseHandle*)this);
			}
		}
	}
}

void LoginClient::ProcessCreateGunionPicSmsSendResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_CreateCheckPicSmsSendCallback)
	{
		gunion_wegame_check_sms_send_CodeGuid = keyValues["checkCodeGuid"];

		//极验参数
		std::string success;
		std::string new_captcha;
		std::string challenge; //极验流水号
		std::string gt;        //极验id
		std::string gt_width;
		std::string gt_height;

		std::string ghome_back_url;

		const char* back_url = (keyValues["checkCodeUrl"]).c_str();
		ghome_back_url = back_url;

		int needCheckCode = keyValues["nextAction"] == "8" ? 1 : 0;
		int width = 0;
		int height = 0;

		if (keyValues["nextAction"] == "8")
		{
			struct json_object* root = json_tokener_parse(keyValues["captchaParams"].c_str());

			// 解析JSON字符串
			if (!is_error(root))
			{

				if (keyValues["imagecodeType"] == "4")
				{
					//极验
					json_object* gtData = json_object_object_get(root, "gtData");

					if (gtData)
					{
						json_object* json_success = json_object_object_get(gtData, "success");
						json_object* json_new_captcha = json_object_object_get(gtData, "new_captcha");
						json_object* json_challenge = json_object_object_get(gtData, "challenge");
						json_object* json_gt = json_object_object_get(gtData, "gt");
						json_object* json_gt_url = json_object_object_get(gtData, "gt_url");

						if (json_success)
						{
							success = json_object_get_string(json_success);
						}
						if (json_new_captcha)
						{
							new_captcha = json_object_get_string(json_new_captcha);
						}
						if (json_challenge)
						{
							challenge = json_object_get_string(json_challenge);
						}
						if (json_gt)
						{
							gt = json_object_get_string(json_gt);
						}
						if (json_gt_url)
						{
							back_url = json_object_get_string(json_gt_url);
							ghome_back_url = back_url;
						}
						//极验
						needCheckCode = 4;
					}
				}
				else if (keyValues["imagecodeType"] == "1")
				{
					//普通图验
					json_object* json_picUrl = json_object_object_get(root, "picUrl");
					if (json_picUrl)
					{
						back_url = json_object_get_string(json_picUrl);
						ghome_back_url = back_url;
					}

				}
			}

			// 释放 JSON 对象
			json_object_put(root);
		}

		//判断是否网络超时,接口请求没有发出去
		int code = atoi(keyValues["code"].c_str());
		std::string safeMsg = GetSafeMsg(keyValues["msg"].c_str(), code);
		TRACEI("GunionPicSmsSend response -> code: %d, safeMsg: %s", code, safeMsg.c_str());
		{
			if (m_CreateCheckPicSmsSendCallback != nullptr)
			{
				m_CreateCheckPicSmsSendCallback(
					code,
					safeMsg.c_str(),
					ghome_back_url.c_str(),
					needCheckCode,
					140,
					37,
					(SdoBaseHandle*)this);
			}
		}
	}
}

void LoginClient::ProcessCreateGunionCheckSmsLoginResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_CreateCheckSmsLoginCallback)
	{
		const char* realMessage = "";
		std::string realTextGbk;
		std::map<std::string, std::string>::iterator it = keyValues.find("realInfoNotification");
		if (it != keyValues.end())
		{
			realTextGbk = Utf8ToGbk(it->second); // 转换成 GBK
			realMessage = realTextGbk.c_str();
		}

		//判断是否网络超时,接口请求没有发出去
		int code = atoi(keyValues["code"].c_str());
		std::string safeMsg = GetSafeMsg(keyValues["msg"].c_str(), code);
		TRACEI("GunionCheckSmsLogin response -> code: %d, safeMsg: %s", code, safeMsg.c_str());
		{
			std::string strTutorNotification = Utf8ToGbk(keyValues["tutorNotification"]);
			if (m_CreateCheckSmsLoginCallback != NULL)
			{
				m_CreateCheckSmsLoginCallback(code,
					safeMsg.c_str(),
					keyValues["userid"].c_str(),
					keyValues["ticket"].c_str(),
					keyValues["autokey"].c_str(),
					atoi(keyValues["has_realInfo"].c_str()),
					atoi(keyValues["isNewUser"].c_str()),
					realMessage, atoi(keyValues["activation"].c_str()),
					atoi(keyValues["isAdult"].c_str()),
					atoi(keyValues["age"].c_str()),
					atoi(keyValues["needTutor"].c_str()),
					strTutorNotification.c_str(),
					keyValues["selectLoginContext"].c_str(),
					keyValues["selectAccountList"].c_str(),
					m_gunionToken.c_str(),
					m_gunionRandKey.c_str());
			}
		}
	}
}

void LoginClient::ProcessCreateGunionDoSpecialSmsSendResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_CreateDoSpecialSmsSendCallback)
	{
		const char* back_url_union;
		//把响应参数存起来
		ghome_check_special_sms_send_guid = keyValues["smsSendGuid"];

		//极验参数
		std::string success;
		std::string new_captcha;
		std::string challenge; //极验流水号
		std::string gt;        //极验id
		int gt_width = 0;
		int gt_height = 0;

		std::string ghome_back_url;

		const char* back_url = (keyValues["checkCodeUrl"]).c_str();
		ghome_back_url = back_url;

		int needCheckCode = keyValues["nextAction"] == "8" ? 1 : 0;
		int width = 0;
		int height = 0;

		if (keyValues["nextAction"] == "8")
		{
			struct json_object* root = json_tokener_parse(keyValues["captchaParams"].c_str());

			// 解析JSON字符串
			if (!is_error(root))
			{

				if (keyValues["imagecodeType"] == "4")
				{
					//极验
					json_object* gtData = json_object_object_get(root, "gtData");

					if (gtData)
					{
						json_object* json_success = json_object_object_get(gtData, "success");
						json_object* json_new_captcha = json_object_object_get(gtData, "new_captcha");
						json_object* json_challenge = json_object_object_get(gtData, "challenge");
						json_object* json_gt = json_object_object_get(gtData, "gt");
						json_object* json_gt_url = json_object_object_get(gtData, "gt_url");

						if (json_success)
						{
							success = json_object_get_string(json_success);
						}
						if (json_new_captcha)
						{
							new_captcha = json_object_get_string(json_new_captcha);
						}
						if (json_challenge)
						{
							challenge = json_object_get_string(json_challenge);
						}
						if (json_gt)
						{
							gt = json_object_get_string(json_gt);
						}
						if (json_gt_url)
						{
							back_url = json_object_get_string(json_gt_url);
							ghome_back_url = back_url;
						}
						//极验
						needCheckCode = 4;
					}
				}
				else if (keyValues["imagecodeType"] == "1")
				{
					//普通图验
					json_object* json_picUrl = json_object_object_get(root, "picUrl");
					if (json_picUrl)
					{
						back_url = json_object_get_string(json_picUrl);
						ghome_back_url = back_url;

						//MessageBoxA(NULL,ghome_back_url.c_str(),"ghome_back_url",0);
					}
				}
			}

			// 释放 JSON 对象
			json_object_put(root);
		}

		if (keyValues["nextAction"] == "8")
		{
			if (keyValues["imagecodeType"] == "1")
			{
				gt_width = 140;
				gt_height = 37;
			}
			else if (keyValues["imagecodeType"] == "4")
			{
				gt_width = atoi((keyValues["gt_width"]).c_str());
				gt_height = atoi((keyValues["gt_height"]).c_str());
			}
		}

		//判断是否网络超时,接口请求没有发出去
		int code = atoi(keyValues["code"].c_str());
		std::string safeMsg = GetSafeMsg(keyValues["msg"].c_str(), code);
		TRACEI("GunionCheckSpecialSmsSend response -> code: %d, safeMsg: %s", code, safeMsg.c_str());

		back_url_union = ghome_back_url.c_str();
		if (m_CreateDoSpecialSmsSendCallback != NULL)
		{
			m_CreateDoSpecialSmsSendCallback(
				code,
				safeMsg.c_str(),
				back_url_union,
				needCheckCode,
				gt_width,
				gt_height,
				keyValues["smsSendGuid"].c_str(),
				keyValues["lastSafePhone"].c_str(),
				(SdoBaseHandle*)this);
		}
	}
}

void LoginClient::ProcessCreateGunionCheckSpeicalPicSmsSendResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_CreateCheckSpecialPicSmsSendCallback)
	{
		ghome_check_special_sms_send_guid = keyValues["smsSendGuid"];

		//极验参数
		std::string success;
		std::string new_captcha;
		std::string challenge; //极验流水号
		std::string gt;        //极验id
		std::string gt_width;
		std::string gt_height;

		std::string ghome_back_url;

		const char* back_url = (keyValues["checkCodeUrl"]).c_str();
		ghome_back_url = back_url;

		int needCheckCode = keyValues["nextAction"] == "8" ? 1 : 0;
		int width = 0;
		int height = 0;

		if (keyValues["nextAction"] == "8")
		{
			struct json_object* root = json_tokener_parse(keyValues["captchaParams"].c_str());

			// 解析JSON字符串
			if (!is_error(root))
			{

				if (keyValues["imagecodeType"] == "4")
				{
					//极验
					json_object* gtData = json_object_object_get(root, "gtData");

					if (gtData)
					{
						json_object* json_success = json_object_object_get(gtData, "success");
						json_object* json_new_captcha = json_object_object_get(gtData, "new_captcha");
						json_object* json_challenge = json_object_object_get(gtData, "challenge");
						json_object* json_gt = json_object_object_get(gtData, "gt");
						json_object* json_gt_url = json_object_object_get(gtData, "gt_url");

						if (json_success)
						{
							success = json_object_get_string(json_success);
						}
						if (json_new_captcha)
						{
							new_captcha = json_object_get_string(json_new_captcha);
						}
						if (json_challenge)
						{
							challenge = json_object_get_string(json_challenge);
						}
						if (json_gt)
						{
							gt = json_object_get_string(json_gt);
						}
						if (json_gt_url)
						{
							back_url = json_object_get_string(json_gt_url);
							ghome_back_url = back_url;
						}
						//极验
						needCheckCode = 4;
					}
				}
				else if (keyValues["imagecodeType"] == "1")
				{
					//普通图验
					json_object* json_picUrl = json_object_object_get(root, "picUrl");
					if (json_picUrl)
					{
						back_url = json_object_get_string(json_picUrl);
						ghome_back_url = back_url;
					}

				}
			}

			// 释放 JSON 对象
			json_object_put(root);
		}

		//判断是否网络超时,接口请求没有发出去
		int code = atoi(keyValues["code"].c_str());
		std::string safeMsg = GetSafeMsg(keyValues["msg"].c_str(), code);
		TRACEI("GunionCheckSpecialPicSmsSend response -> code: %d, safeMsg: %s", code, safeMsg.c_str());
		if (m_CreateCheckSpecialPicSmsSendCallback != NULL)
		{
			m_CreateCheckSpecialPicSmsSendCallback(
				code,
				safeMsg.c_str(),
				ghome_back_url.c_str(),
				needCheckCode,
				140,
				37,
				keyValues["smsSendGuid"].c_str(),
				keyValues["lastSafePhone"].c_str(),
				(SdoBaseHandle*)this);
		}
	}
}

void LoginClient::ProcessCreateGunionCheckSpecialConfirmSmsSendLoginResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	{
		if (m_CreateDoSpecialConfirmSmsSendCallback != NULL)
		{
			//判断是否网络超时,接口请求没有发出去
			int code = atoi(keyValues["code"].c_str());
			std::string safeMsg = GetSafeMsg(keyValues["msg"].c_str(), code);
			TRACEI("GunionCheckSpecialConfirmSmsSend response -> code: %d, safeMsg: %s", code, safeMsg.c_str());
			m_CreateDoSpecialConfirmSmsSendCallback(
				code,
				safeMsg.c_str());
		}
	}
}

void LoginClient::ProcessCreateGunionCheckSpecialSmsLoginResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_CreateDoSpecialCheckSmsLoginCallback)
	{
		const char* realMessage = "";
		std::string realTextGbk;
		std::map<std::string, std::string>::iterator it = keyValues.find("realInfoNotification");
		if (it != keyValues.end())
		{
			realTextGbk = Utf8ToGbk(it->second); // 转换成 GBK
			realMessage = realTextGbk.c_str();
		}
		std::string strTutorNotification = Utf8ToGbk(keyValues["tutorNotification"]);

		//判断是否网络超时,接口请求没有发出去
		int code = atoi(keyValues["code"].c_str());
		std::string safeMsg = GetSafeMsg(keyValues["msg"].c_str(), code);
		TRACEI("GunionCheckSpecialSmsLogin response -> code: %d, safeMsg: %s", code, safeMsg.c_str());
		if (m_CreateDoSpecialCheckSmsLoginCallback != NULL)
		{
			m_CreateDoSpecialCheckSmsLoginCallback(
				code,
				safeMsg.c_str(),
				keyValues["userid"].c_str(),
				keyValues["ticket"].c_str(),
				keyValues["autokey"].c_str(),
				atoi(keyValues["has_realInfo"].c_str()),
				atoi(keyValues["isNewUser"].c_str()),
				atoi(keyValues["isPt"].c_str()),
				realMessage,
				atoi(keyValues["activation"].c_str()),
				atoi(keyValues["isAdult"].c_str()),
				atoi(keyValues["age"].c_str()),
				atoi(keyValues["needTutor"].c_str()),
				strTutorNotification.c_str(),
				m_gunionToken.c_str(),
				m_gunionRandKey.c_str());
		}
	}
}

void LoginClient::ProcessCreateGunionCheckRealNameResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	//当blockLogin== 1表示需要拦截登录行为时，请通过toast展示blockNotification并返回至登录页面
	if (m_CreateCheckRealNameCallback)
	{
		//判断是否网络超时,接口请求没有发出去
		int code = atoi(keyValues["code"].c_str());
		std::string safeMsg = GetSafeMsg(keyValues["msg"].c_str(), code);
		TRACEI("GunionCheckRealName response -> code: %d, safeMsg: %s", code, safeMsg.c_str());
		{
			std::string strTutorNotification = Utf8ToGbk(keyValues["tutorNotification"]);
			std::string strBlockNotification = Utf8ToGbk(keyValues["blockNotification"]);
			if (m_CreateCheckRealNameCallback != NULL)
			{
				m_CreateCheckRealNameCallback(
					code,
					safeMsg.c_str(),
					atoi(keyValues["isAdult"].c_str()),
					atoi(keyValues["age"].c_str()),
					atoi(keyValues["needTutor"].c_str()),
					strTutorNotification.c_str(),
					atoi(keyValues["blockLogin"].c_str()),
					strBlockNotification.c_str());
			}
		}
	}
}

void LoginClient::ProcessCreateGunionCheckAutoLoginResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_CreateCheckAutoLoginCallback)
	{
		const char* realMessage = "";
		std::string realTextGbk;
		std::map<std::string, std::string>::iterator it = keyValues.find("realInfoNotification");
		if (it != keyValues.end())
		{
			realTextGbk = Utf8ToGbk(it->second); // 转换成 GBK
			realMessage = realTextGbk.c_str();
		}

		//判断是否网络超时,接口请求没有发出去
		int code = atoi(keyValues["code"].c_str());
		std::string safeMsg = GetSafeMsg(keyValues["msg"].c_str(), code);
		TRACEI("GunionCheckAutoLogin response -> code: %d, safeMsg: %s", code, safeMsg.c_str());
		{
			std::string strTutorNotification = Utf8ToGbk(keyValues["tutorNotification"]);
			if (m_CreateCheckAutoLoginCallback != NULL)
			{
				m_CreateCheckAutoLoginCallback(
					code,
					safeMsg.c_str(),
					keyValues["userid"].c_str(),
					keyValues["ticket"].c_str(),
					keyValues["autokey"].c_str(),
					atoi(keyValues["has_realInfo"].c_str()),
					atoi(keyValues["isNewUser"].c_str()),
					realMessage,
					atoi(keyValues["activation"].c_str()),
					atoi(keyValues["isAdult"].c_str()),
					atoi(keyValues["age"].c_str()),
					atoi(keyValues["needTutor"].c_str()),
					strTutorNotification.c_str(),
					m_gunionToken.c_str(),
					m_gunionRandKey.c_str());
			}
		}
	}
}

void LoginClient::ProcessCreateGunionCheckLogoutResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_CreateCheckLogoutCallback)
	{
		bool flag = true;
		//if (atoi(keyValues["code"].c_str()) == 0)
		//{
		//	//清除随机秘钥和token
		//	flag = GhomeClearKeyAndToken(ghome_handle);
		//}

		//判断是否网络超时,接口请求没有发出去
		int code = atoi(keyValues["code"].c_str());
		std::string safeMsg = GetSafeMsg(keyValues["msg"].c_str(), code);
		TRACEI("GunionCheckLogout response -> code: %d, safeMsg: %s", code, safeMsg.c_str());
		//if (code == 0)
		{
			//登出之后状态清除，并且重新握手
			//m_gunionToken = "";
			//m_gunionRandKey = "";
			//m_gunionHandshake->Reset();
			//m_gunionHandshake->Start();
		}

		if (m_CreateCheckLogoutCallback != NULL)
		{
			m_CreateCheckLogoutCallback(
				code,
				safeMsg.c_str(),
				true);
		}
	}
}

void LoginClient::ProcessCreateGunionCheckPwdLoginResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_CreateCheckPwdLoginCallback)
	{
		//判断是否网络超时,接口请求没有发出去
		int code = atoi(keyValues["code"].c_str());
		std::string safeMsg = GetSafeMsg(keyValues["msg"].c_str(), code);
		TRACEI("GunionCheckPwdLogin response -> code: %d, safeMsg: %s", code, safeMsg.c_str());

		const char* realMessage = "";
		std::string realTextGbk;
		std::map<std::string, std::string>::iterator it = keyValues.find("realInfoNotification");
		if (it != keyValues.end())
		{
			realTextGbk = Utf8ToGbk(it->second); // 转换成 GBK
			realMessage = realTextGbk.c_str();
		}

		const char* back_url_union;

		ghome_check_static_login_CodeGuid = keyValues["checkCodeGuid"];

		//极验参数
		std::string success;
		std::string new_captcha;
		std::string challenge; //极验流水号
		std::string gt;        //极验id
		int gt_width = 0;
		int gt_height = 0;

		std::string gunion_back_url;

		const char* back_url = (keyValues["checkCodeUrl"]).c_str();
		gunion_back_url = back_url;

		int needCheckCode = keyValues["nextAction"] == "8" ? 1 : 0;
		int width = 0;
		int height = 0;

		if (keyValues["nextAction"] == "8")
		{
			struct json_object* root = json_tokener_parse(keyValues["captchaParams"].c_str());

			// 解析JSON字符串
			if (!is_error(root))
			{

				if (keyValues["imagecodeType"] == "4")
				{
					//极验
					json_object* gtData = json_object_object_get(root, "gtData");

					if (gtData)
					{
						json_object* json_success = json_object_object_get(gtData, "success");
						json_object* json_new_captcha = json_object_object_get(gtData, "new_captcha");
						json_object* json_challenge = json_object_object_get(gtData, "challenge");
						json_object* json_gt = json_object_object_get(gtData, "gt");
						json_object* json_gt_url = json_object_object_get(gtData, "gt_url");

						if (json_success)
						{
							success = json_object_get_string(json_success);
						}
						if (json_new_captcha)
						{
							new_captcha = json_object_get_string(json_new_captcha);
						}
						if (json_challenge)
						{
							challenge = json_object_get_string(json_challenge);
						}
						if (json_gt)
						{
							gt = json_object_get_string(json_gt);
						}
						if (json_gt_url)
						{
							back_url = json_object_get_string(json_gt_url);
							gunion_back_url = back_url;
						}
						//极验
						needCheckCode = 4;
					}
				}
				else if (keyValues["imagecodeType"] == "1")
				{
					//普通图验
					json_object* json_picUrl = json_object_object_get(root, "picUrl");
					if (json_picUrl)
					{
						back_url = json_object_get_string(json_picUrl);
						gunion_back_url = back_url;
					}

				}
			}

			// 释放 JSON 对象
			json_object_put(root);
		}

		if (keyValues["nextAction"] == "8")
		{
			if (keyValues["imagecodeType"] == "1")
			{
				gt_width = 140;
				gt_height = 37;
			}
			else if (keyValues["imagecodeType"] == "4")
			{
				gt_width = atoi((keyValues["sdg_width"]).c_str());
				gt_height = atoi((keyValues["sdg_height"]).c_str());
			}
		}

		if (keyValues["nextAction"] == "8")
		{
			back_url_union = gunion_back_url.c_str();
			if (m_CreateDoSmsSendCallback != NULL)
			{
				m_CreateDoSmsSendCallback(
					code,
					safeMsg.c_str(),
					back_url_union,
					needCheckCode,
					gt_width, gt_height,
					(SdoBaseHandle*)this);
			}
		}
		else
		{
			std::string strTutorNotification = Utf8ToGbk(keyValues["tutorNotification"]);
			if (m_CreateCheckPwdLoginCallback != NULL)
			{
				m_CreateCheckPwdLoginCallback(
					code,
					safeMsg.c_str(),
					keyValues["userid"].c_str(),
					keyValues["ticket"].c_str(),
					keyValues["autokey"].c_str(),
					atoi(keyValues["has_realInfo"].c_str()),
					atoi(keyValues["isNewUser"].c_str()),
					realMessage, atoi(keyValues["activation"].c_str()),
					atoi(keyValues["isAdult"].c_str()),
					atoi(keyValues["age"].c_str()),
					atoi(keyValues["needTutor"].c_str()),
					strTutorNotification.c_str(),
					keyValues["selectLoginContext"].c_str(),
					keyValues["selectAccountList"].c_str(),
					m_gunionToken.c_str(),
					m_gunionRandKey.c_str());
			}
		}
	}
}

void LoginClient::ProcessCreateGunionCheckPicPwdLoginResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_CreateCheckPicPwdLoginCallback)
	{
		const char* realMessage = "";
		std::string realTextGbk;
		std::map<std::string, std::string>::iterator it = keyValues.find("realInfoNotification");
		if (it != keyValues.end())
		{
			realTextGbk = Utf8ToGbk(it->second); // 转换成 GBK
			realMessage = realTextGbk.c_str();
		}

		ghome_check_static_login_CodeGuid = keyValues["checkCodeGuid"];

		//极验参数
		std::string success;
		std::string new_captcha;
		std::string challenge; //极验流水号
		std::string gt;        //极验id
		std::string gt_width;
		std::string gt_height;

		std::string ghome_back_url;

		const char* back_url = (keyValues["checkCodeUrl"]).c_str();
		ghome_back_url = back_url;

		int needCheckCode = keyValues["nextAction"] == "8" ? 1 : 0;
		int width = 0;
		int height = 0;

		if (keyValues["nextAction"] == "8")
		{
			struct json_object* root = json_tokener_parse(keyValues["captchaParams"].c_str());

			// 解析JSON字符串
			if (!is_error(root))
			{

				if (keyValues["imagecodeType"] == "4")
				{
					//极验
					json_object* gtData = json_object_object_get(root, "gtData");

					if (gtData)
					{
						json_object* json_success = json_object_object_get(gtData, "success");
						json_object* json_new_captcha = json_object_object_get(gtData, "new_captcha");
						json_object* json_challenge = json_object_object_get(gtData, "challenge");
						json_object* json_gt = json_object_object_get(gtData, "gt");
						json_object* json_gt_url = json_object_object_get(gtData, "gt_url");

						if (json_success)
						{
							success = json_object_get_string(json_success);
						}
						if (json_new_captcha)
						{
							new_captcha = json_object_get_string(json_new_captcha);
						}
						if (json_challenge)
						{
							challenge = json_object_get_string(json_challenge);
						}
						if (json_gt)
						{
							gt = json_object_get_string(json_gt);
						}
						if (json_gt_url)
						{
							back_url = json_object_get_string(json_gt_url);
							ghome_back_url = back_url;
						}
						//极验
						needCheckCode = 4;
					}
				}
				else if (keyValues["imagecodeType"] == "1")
				{
					//普通图验
					json_object* json_picUrl = json_object_object_get(root, "picUrl");
					if (json_picUrl)
					{
						back_url = json_object_get_string(json_picUrl);
						ghome_back_url = back_url;
					}

				}
			}

			// 释放 JSON 对象
			json_object_put(root);
		}
		std::string strTutorNotification = Utf8ToGbk(keyValues["tutorNotification"]);
		if (m_CreateCheckPicPwdLoginCallback != NULL)
		{
			//判断是否网络超时,接口请求没有发出去
			int code = atoi(keyValues["code"].c_str());
			std::string safeMsg = GetSafeMsg(keyValues["msg"].c_str(), code);
			TRACEI("GunionCheckPicPwdLogin response -> code: %d, safeMsg: %s", code, safeMsg.c_str());
			m_CreateCheckPicPwdLoginCallback(
				code,
				safeMsg.c_str(),
				keyValues["userid"].c_str(),
				keyValues["ticket"].c_str(),
				keyValues["autokey"].c_str(),
				atoi(keyValues["has_realInfo"].c_str()),
				atoi(keyValues["isNewUser"].c_str()),
				ghome_back_url.c_str(),
				needCheckCode, 140, 37, realMessage,
				atoi(keyValues["activation"].c_str())
				, atoi(keyValues["isAdult"].c_str()),
				atoi(keyValues["age"].c_str())
				, atoi(keyValues["needTutor"].c_str()),
				strTutorNotification.c_str(),
				keyValues["selectLoginContext"].c_str(),
				keyValues["selectAccountList"].c_str(),
				m_gunionToken.c_str(),
				m_gunionRandKey.c_str());
		}
	}
}

void LoginClient::ProcessCreateGunionCheckThirdLoginResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_CreateCheckThirdLoginCallback)
	{
		for (std::map<std::string, std::string>::iterator iter = keyValues.begin(); iter != keyValues.end(); ++iter)
		{
			if (iter->first == "realInfoNotification")
			{
				iter->second = Utf8ToGbk(iter->second);
			}
		}
		std::string strTutorNotification = Utf8ToGbk(keyValues["tutorNotification"]);
		if (m_CreateCheckThirdLoginCallback != NULL)
		{
			//判断是否网络超时,接口请求没有发出去
			int code = atoi(keyValues["code"].c_str());
			std::string safeMsg = GetSafeMsg(keyValues["msg"].c_str(), code);
			TRACEI("GunionCheckPicPwdLogin response -> code: %d, safeMsg: %s", code, safeMsg.c_str());
			m_CreateCheckThirdLoginCallback(
				code,
				safeMsg.c_str(),
				keyValues["userid"].c_str(),
				keyValues["ticket"].c_str(),
				keyValues["autokey"].c_str(),
				atoi(keyValues["has_realInfo"].c_str()),
				atoi(keyValues["isNewUser"].c_str()),
				atoi(keyValues["activation"].c_str()),
				atoi(keyValues["isAdult"].c_str()),
				atoi(keyValues["age"].c_str()),
				atoi(keyValues["needTutor"].c_str()),
				strTutorNotification.c_str());
		}
	}
}

void LoginClient::ProcessCreateGunionCheckSpecialPwdLoginResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_CreateCheckSpecialPwdLoginCallback)
	{
		const char* realMessage = "";
		std::string realTextGbk;
		std::map<std::string, std::string>::iterator it = keyValues.find("realInfoNotification");
		if (it != keyValues.end())
		{
			realTextGbk = Utf8ToGbk(it->second); // 转换成 GBK
			realMessage = realTextGbk.c_str();
		}

		const char* back_url_union;

		//安全手机图片验证码上下文
		ghome_check_special_check_code_guid = keyValues["checkCodeGuid"];

		//安全手机短信验证码上下文
		ghome_check_special_sms_send_guid = keyValues["smsSendGuid"];
		//极验参数
		std::string success;
		std::string new_captcha;
		std::string challenge; //极验流水号
		std::string gt;        //极验id
		int gt_width = 0;
		int gt_height = 0;

		std::string ghome_back_url;

		const char* back_url = (keyValues["checkCodeUrl"]).c_str();
		ghome_back_url = back_url;

		int needCheckCode = keyValues["nextAction"] == "8" ? 1 : 0;
		int width = 0;
		int height = 0;

		if (keyValues["nextAction"] == "8")
		{
			struct json_object* root = json_tokener_parse(keyValues["captchaParams"].c_str());

			// 解析JSON字符串
			if (!is_error(root))
			{

				if (keyValues["imagecodeType"] == "4")
				{
					//极验
					json_object* gtData = json_object_object_get(root, "gtData");

					if (gtData)
					{
						json_object* json_success = json_object_object_get(gtData, "success");
						json_object* json_new_captcha = json_object_object_get(gtData, "new_captcha");
						json_object* json_challenge = json_object_object_get(gtData, "challenge");
						json_object* json_gt = json_object_object_get(gtData, "gt");
						json_object* json_gt_url = json_object_object_get(gtData, "gt_url");

						if (json_success)
						{
							success = json_object_get_string(json_success);
						}
						if (json_new_captcha)
						{
							new_captcha = json_object_get_string(json_new_captcha);
						}
						if (json_challenge)
						{
							challenge = json_object_get_string(json_challenge);
						}
						if (json_gt)
						{
							gt = json_object_get_string(json_gt);
						}
						if (json_gt_url)
						{
							back_url = json_object_get_string(json_gt_url);
							ghome_back_url = back_url;
						}
						//极验
						needCheckCode = 4;
					}
				}
				else if (keyValues["imagecodeType"] == "1")
				{
					//普通图验
					json_object* json_picUrl = json_object_object_get(root, "picUrl");
					if (json_picUrl)
					{
						back_url = json_object_get_string(json_picUrl);
						ghome_back_url = back_url;
					}

				}
			}

			// 释放 JSON 对象
			json_object_put(root);
		}

		if (keyValues["nextAction"] == "8")
		{
			if (keyValues["imagecodeType"] == "1")
			{
				gt_width = 140;
				gt_height = 37;
			}
			else if (keyValues["imagecodeType"] == "4")
			{
				gt_width = atoi((keyValues["sdg_width"]).c_str());
				gt_height = atoi((keyValues["sdg_height"]).c_str());
			}
		}

		if (keyValues["nextAction"] == "8")
		{
			back_url_union = ghome_back_url.c_str();
			if (m_CreateDoSmsSendCallback != NULL)
			{
				//判断是否网络超时,接口请求没有发出去
				int code = atoi(keyValues["code"].c_str());
				std::string safeMsg = GetSafeMsg(keyValues["msg"].c_str(), code);
				TRACEI("GunionSmsSend response -> code: %d, safeMsg: %s", code, safeMsg.c_str());
				m_CreateDoSmsSendCallback(
					code,
					safeMsg.c_str(),
					back_url_union,
					needCheckCode,
					gt_width,
					gt_height,
					(SdoBaseHandle*)this);
			}
		}
		else
		{
			std::string strTutorNotification = Utf8ToGbk(keyValues["tutorNotification"]);
			if (m_CreateCheckSpecialPwdLoginCallback != NULL)
			{
				//判断是否网络超时,接口请求没有发出去
				int code = atoi(keyValues["code"].c_str());
				std::string safeMsg = GetSafeMsg(keyValues["msg"].c_str(), code);
				TRACEI("GunionCheckPwdLogin response -> code: %d, safeMsg: %s", code, safeMsg.c_str());
				m_CreateCheckSpecialPwdLoginCallback(
					code,
					safeMsg.c_str(),
					keyValues["userid"].c_str(),
					keyValues["ticket"].c_str(),
					keyValues["autokey"].c_str(),
					atoi(keyValues["has_realInfo"].c_str()),
					atoi(keyValues["isNewUser"].c_str()),
					keyValues["smsSendGuid"].c_str(),
					keyValues["lastSafePhone"].c_str(),
					atoi(keyValues["isPt"].c_str()),
					realMessage,
					atoi(keyValues["activation"].c_str())
					, atoi(keyValues["isAdult"].c_str()),
					atoi(keyValues["age"].c_str())
					, atoi(keyValues["needTutor"].c_str()),
					strTutorNotification.c_str(),
					m_gunionToken.c_str(),
					m_gunionRandKey.c_str());
			}
		}
	}
}

void LoginClient::ProcessCreateGunionCheckSpecialPicPwdLoginResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_CreateCheckSpecialPicPwdLoginCallback)
	{
		const char* realMessage = "";
		std::string realTextGbk;
		std::map<std::string, std::string>::iterator it = keyValues.find("realInfoNotification");
		if (it != keyValues.end())
		{
			realTextGbk = Utf8ToGbk(it->second); // 转换成 GBK
			realMessage = realTextGbk.c_str();
		}

		if (atoi(keyValues["code"].c_str()) == 0)
		{
			ghome_check_special_check_code_guid = keyValues["checkCodeGuid"];
		}
		//安全手机短信验证码上下文
		ghome_check_special_sms_send_guid = keyValues["smsSendGuid"];

		//极验参数
		std::string success;
		std::string new_captcha;
		std::string challenge; //极验流水号
		std::string gt;        //极验id
		std::string gt_width;
		std::string gt_height;

		std::string ghome_back_url;

		const char* back_url = (keyValues["checkCodeUrl"]).c_str();
		ghome_back_url = back_url;

		//这个接口存在nextAction从服务端返回永远为0
		int needCheckCode = keyValues["nextAction"] == "8" ? 1 : 0;
		int width = 0;
		int height = 0;

		if (keyValues["nextAction"] == "8")
		{
			struct json_object* root = json_tokener_parse(keyValues["captchaParams"].c_str());

			// 解析JSON字符串
			if (!is_error(root))
			{
				if (keyValues["imagecodeType"] == "4")
				{
					//极验
					json_object* gtData = json_object_object_get(root, "gtData");

					if (gtData)
					{
						json_object* json_success = json_object_object_get(gtData, "success");
						json_object* json_new_captcha = json_object_object_get(gtData, "new_captcha");
						json_object* json_challenge = json_object_object_get(gtData, "challenge");
						json_object* json_gt = json_object_object_get(gtData, "gt");
						json_object* json_gt_url = json_object_object_get(gtData, "gt_url");

						if (json_success)
						{
							success = json_object_get_string(json_success);
						}
						if (json_new_captcha)
						{
							new_captcha = json_object_get_string(json_new_captcha);
						}
						if (json_challenge)
						{
							challenge = json_object_get_string(json_challenge);
						}
						if (json_gt)
						{
							gt = json_object_get_string(json_gt);
						}
						if (json_gt_url)
						{
							back_url = json_object_get_string(json_gt_url);
							ghome_back_url = back_url;
						}
						//极验
						needCheckCode = 4;
					}
				}
				else if (keyValues["imagecodeType"] == "1")
				{
					//普通图验
					json_object* json_picUrl = json_object_object_get(root, "picUrl");
					if (json_picUrl)
					{
						back_url = json_object_get_string(json_picUrl);
						ghome_back_url = back_url;
					}

				}
			}

			// 释放 JSON 对象
			json_object_put(root);
		}

		std::string strTutorNotification = Utf8ToGbk(keyValues["tutorNotification"]);

		if (m_CreateCheckSpecialPicPwdLoginCallback != NULL)
		{
			//判断是否网络超时,接口请求没有发出去
			int code = atoi(keyValues["code"].c_str());
			std::string safeMsg = GetSafeMsg(keyValues["msg"].c_str(), code);
			TRACEI("GunionCheckPicPwdLogin response -> code: %d, safeMsg: %s", code, safeMsg.c_str());
			m_CreateCheckSpecialPicPwdLoginCallback(
				code,
				safeMsg.c_str(),
				keyValues["userid"].c_str(),
				keyValues["ticket"].c_str(),
				keyValues["autokey"].c_str(),
				atoi(keyValues["has_realInfo"].c_str()),
				atoi(keyValues["isNewUser"].c_str()),
				ghome_back_url.c_str(),
				needCheckCode,
				140,
				37,
				keyValues["smsSendGuid"].c_str(),
				keyValues["lastSafePhone"].c_str(),
				atoi(keyValues["isPt"].c_str()),
				realMessage,
				atoi(keyValues["activation"].c_str())
				, atoi(keyValues["isAdult"].c_str()),
				atoi(keyValues["age"].c_str())
				, atoi(keyValues["needTutor"].c_str()),
				strTutorNotification.c_str(),
				m_gunionToken.c_str(),
				m_gunionRandKey.c_str());
		}
	}
}

void LoginClient::ProcessCreateGunionCheckSetLoginAreaLoginResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	{
		if (m_CreateCheckSetLoginAreaLoginCallback != NULL)
		{
			int code = atoi(keyValues["code"].c_str());
			std::string safeMsg = GetSafeMsg(keyValues["msg"].c_str(), code);
			TRACEI("GunionCheckSetLoginArea response -> code: %d, safeMsg: %s", code, safeMsg.c_str());
			m_CreateCheckSetLoginAreaLoginCallback(
				code,
				safeMsg.c_str(),
				keyValues["userid"].c_str());
		}
	}
}

void LoginClient::ProcessCreateGunionPayEntranceResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	{
		if (m_CreateDoPayEntranceCallback != NULL)
		{
			int code = atoi(keyValues["code"].c_str());
			std::string safeMsg = GetSafeMsg(keyValues["msg"].c_str(), code);
			TRACEI("GunionPayEntrance response -> code: %d, safeMsg: %s", code, safeMsg.c_str());
			m_CreateDoPayEntranceCallback(
				code,
				safeMsg.c_str(),
				keyValues["orderid"].c_str(),
				keyValues["payorderid"].c_str(),
				keyValues["paymenturl"].c_str());
		}
	}
}

void LoginClient::ProcessCreateGunionCheckPayOrderStatusResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	{
		if (m_CreateDoCheckPayOrderStatuseCallback != NULL)
		{
			int code = atoi(keyValues["code"].c_str());
			std::string safeMsg = GetSafeMsg(keyValues["msg"].c_str(), code);
			TRACEI("GunionCheckPayOrderStatus response -> code: %d, safeMsg: %s", code, safeMsg.c_str());
			m_CreateDoCheckPayOrderStatuseCallback(
				code,
				safeMsg.c_str(),
				keyValues["status"].c_str());
		}
	}
}

void LoginClient::ProcessCreateGunionCheckActivationResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	{
		if (m_CreateDoCheckActivationCallback != NULL)
		{
			int code = atoi(keyValues["code"].c_str());
			std::string safeMsg = GetSafeMsg(keyValues["msg"].c_str(), code);
			m_CreateDoCheckActivationCallback(
				code,
				safeMsg.c_str());
		}
	}
}

void LoginClient::ProcessCreateGunionCheckGetLoginAreaLoginResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	{
		if (m_CreateCheckGetLoginAreaLoginCallback != NULL)
		{
			//判断是否网络超时,接口请求没有发出去
			int code = atoi(keyValues["code"].c_str());
			std::string safeMsg = GetSafeMsg(keyValues["msg"].c_str(), code);
			TRACEI("GunionCheckGetLoginArea response -> code: %d, safeMsg: %s", code, safeMsg.c_str());
			m_CreateCheckGetLoginAreaLoginCallback(
				code,
				safeMsg.c_str(),
				(Utf8ToGbk(keyValues["message"]).c_str()));
		}
	}
}

void LoginClient::ProcessCreateGunionCheckSetTutorInfoResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	{
		if (m_CreateCheckSetTutorInfoCallback != NULL)
		{
			int code = atoi(keyValues["code"].c_str());
			std::string safeMsg = GetSafeMsg(keyValues["msg"].c_str(), code);
			TRACEI("GunionCheckSetTutorInfo response -> code: %d, safeMsg: %s", code, safeMsg.c_str());
			std::string strBlockNotification = Utf8ToGbk(keyValues["blockNotification"]);
			std::string strConfirmKey = (keyValues["confirmKey"]);
			std::string strNeedConfirmNotification = Utf8ToGbk(keyValues["needConfirmNotification"]);
			m_CreateCheckSetTutorInfoCallback(
				code,
				safeMsg.c_str(),
				atoi(keyValues["blockLogin"].c_str()),
				strBlockNotification.c_str(),
				atoi(keyValues["needConfirm"].c_str()),
				strConfirmKey.c_str(),
				strNeedConfirmNotification.c_str());
		}
	}
}

void LoginClient::ProcessCreateGunionCheckGunionWegameLoginResponse(int reqeustCode, map<string, string>& keyValues)
{
	TRACET();
	if (m_CreateCheckGunionWegameLoginCallback)
	{
		const char* realMessage = "";
		std::string realTextGbk;
		std::map<std::string, std::string>::iterator it = keyValues.find("realInfoNotification");
		if (it != keyValues.end())
		{
			realTextGbk = Utf8ToGbk(it->second); // 转换成 GBK
			realMessage = realTextGbk.c_str();
		}

		//判断是否网络超时,接口请求没有发出去
		int code = atoi(keyValues["code"].c_str());
		std::string safeMsg = GetSafeMsg(keyValues["msg"].c_str(), code);
		TRACEI("GunionCheckGunionWegameLogin response -> code: %d, safeMsg: %s", code, safeMsg.c_str());
		{
			std::string strTutorNotification = Utf8ToGbk(keyValues["tutorNotification"]);
			if (m_CreateCheckGunionWegameLoginCallback != NULL)
			{
				m_CreateCheckGunionWegameLoginCallback(
					code,
					safeMsg.c_str(),
					keyValues["userid"].c_str(),
					keyValues["ticket"].c_str(),
					keyValues["autokey"].c_str(),
					atoi(keyValues["has_realInfo"].c_str()),
					atoi(keyValues["isNewUser"].c_str()),
					realMessage,
					atoi(keyValues["activation"].c_str()));
			}
		}
	}
}

// ---------------------- Gunion登录相关响应处理 ----------------------

void LoginClient::ProcessCreateGunionLoginGetPublicKeyResponse(
	int requestCode,
	map<string, string>& keyValues)
{
	TRACET();

	// 1. 取 code
	int code = ERROR_RESPONSE;
	auto itCode = keyValues.find("code");
	if (itCode != keyValues.end())
	{
		code = atoi(itCode->second.c_str());
	}

	// 2. 生成 safeMsg
	std::string safeMsg;
	auto itMsg = keyValues.find("msg");
	if (itMsg != keyValues.end())
	{
		safeMsg = GetSafeMsg(itMsg->second.c_str(), code);
	}
	else
	{
		safeMsg = GetSafeMsg("", code);
	}
	TRACEI("GunionGetPublicKey response -> code: %d, safeMsg: %s", code, safeMsg.c_str());

	// 2. 取 RSA 公钥
	auto itKey = keyValues.find("key");

	// 3. 构造完整 PEM 公钥
	m_gunionPublicKey.clear();
	m_gunionPublicKey.reserve(itKey->second.size() + 64);

	m_gunionPublicKey += RSA_PUBLIC_KEY_PREFIX;
	m_gunionPublicKey += itKey->second;
	m_gunionPublicKey += RSA_PUBLIC_KEY_SUFFIX;

	// 4. PublicKey 已就绪
	if (m_CreateCheckGunionGetPublicKeyCallback != NULL)
	{
		m_CreateCheckGunionGetPublicKeyCallback(
			code,
			safeMsg.c_str(),
			m_gunionPublicKey.c_str());
	}
}

void LoginClient::ProcessCreateGunionLoginHandshakeResponse(
	int requestCode,
	map<string, string>& keyValues)
{
	TRACET();

	// 1. 取 code
	int code = ERROR_RESPONSE;
	auto itCode = keyValues.find("code");
	if (itCode != keyValues.end())
	{
		code = atoi(itCode->second.c_str());
	}
	// 2. 生成 safeMsg
	std::string safeMsg;
	auto itMsg = keyValues.find("msg");
	if (itMsg != keyValues.end())
	{
		safeMsg = GetSafeMsg(itMsg->second.c_str(), code);
	}
	else
	{
		safeMsg = GetSafeMsg("", code);
	}
	TRACEI("GunionHandshake response -> code: %d, safeMsg: %s", code, safeMsg.c_str());

	// 3. 取 token
	auto itToken = keyValues.find("token");

	// 4. 保存会话 token
	m_gunionToken = itToken->second;

	// 5. token 已就绪
	if (m_CreateCheckGunionHandShakeCallback != NULL)
	{
		m_CreateCheckGunionHandShakeCallback(
			code,
			safeMsg.c_str(),
			m_gunionToken.c_str());
	}
}

// ---------------------- Gunion- 登录相关响应处理 ----------------------

int LoginClient::ProxyEnable(int enable)
{
	m_httpThread->ProxyEnable(enable);
	return 0;
}

int LoginClient::SetValue(const char* keyVal, const char* val)
{
	if (keyVal == nullptr || val == nullptr)
	{
		return ERROR_PARAM_INVALIDATE;
	}

	m_mapValList[keyVal] = val;

	if (strcmp(keyVal, HOSTNAME3) == 0 && nullptr != m_httpThread)
	{
		string hostname3;
		int hostport3;
		ParseHttpAddr(val, hostname3, hostport3);

		m_httpThread->SetHost3(hostname3, hostport3);

	}

	if (strcmp(keyVal, HOSTNAME4) == 0 && nullptr != m_httpThread)
	{
		string hostname4;
		int hostport4;
		ParseHttpAddr(val, hostname4, hostport4);
		m_httpThread->SetHost4(hostname4, hostport4);

	}

	if (strcmp(keyVal, HOSTNAME5) == 0 && nullptr != m_httpThread)
	{
		//新版域名关于下行短信
		string hostname5;
		int hostport5;
		ParseHttpAddr(val, hostname5, hostport5);
		m_httpThread->SetHost5(hostname5, hostport5);
	}

	if (strcmp(keyVal, HOSTNAME6) == 0 && nullptr != m_httpThread)
	{
		//新版域名关于下行短信
		string hostname6;
		int hostport6;
		ParseHttpAddr(val, hostname6, hostport6);
		m_httpThread->SetHost6(hostname6, hostport6);
	}

	if (strcmp(keyVal, HOSTNAME7) == 0 && nullptr != m_httpThread)
	{
		//新版域名关于Gunion
		string hostname7;
		int hostport7;
		ParseHttpAddr(val, hostname7, hostport7);
		m_httpThread->SetHost7(hostname7, hostport7);
	}

	if (strcmp(keyVal, HOSTNAME8) == 0 && nullptr != m_httpThread)
	{
		//新版域名关于Gunion
		string hostname8;
		int hostport8;
		ParseHttpAddr(val, hostname8, hostport8);
		m_httpThread->SetHost8(hostname8, hostport8);
	}

	return 0;
}

int LoginClient::GetValue(const char* keyVal, char* val)
{
	if (keyVal == nullptr || val == nullptr)
	{
		return ERROR_PARAM_INVALIDATE;
	}

	memcpy(val, m_mapValList[keyVal].c_str(), m_mapValList[keyVal].size());

	return 0;
}

int LoginClient::SetCallBack(const char* funcName, const void* func)
{
	if (funcName == nullptr || func == nullptr)
	{
		return ERROR_PARAM_INVALIDATE;
	}

	m_mapCallback[funcName] = (void*)func;
	return 0;
}

int LoginClient::WaitResponse(int timeout)
{
	if (m_waitEvent && m_httpThread->GetState() == HttpThread::STATE_BUSY)
	{
		ResetEvent(m_waitEvent);

		return WaitForSingleObject(m_waitEvent, timeout);
	}

	return 0;
}

int LoginClient::Cancel()
{
	TRACET();
	if (m_httpThread->GetState() == HttpThread::STATE_BUSY)
	{
		if (m_waitEvent)
		{
			ResetEvent(m_waitEvent);
		}

		if (m_httpThread)
		{
			m_httpThread->Cancel();
		}
	}
	return 0;
}

void LoginClient::MapToMap(map<string, string>& dest, map<string, string> src)
{
	for (map<string, string>::const_iterator i = src.begin(); i != src.end(); i++)
	{
		dest[i->first] = i->second;
	}
}

HttpRequest* LoginClient::GetPublicKeyRequest()
{
	return m_requestProcess.GetPublicKeyRequest();
}

int LoginClient::GetPublicKey()
{
	HttpRequest* request = GetPublicKeyRequest();
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::SetUserData(const void* userData)
{
	if (userData == nullptr)
	{
		return ERROR_PARAM_INVALIDATE;
	}

	if (m_httpThread)
		m_httpThread->SetUserData(userData);

	return 0;
}

void* LoginClient::GetUserData()
{
	if (m_httpThread)
	{
		return m_httpThread->GetUserData();
	}

	return nullptr;
}

int LoginClient::SetParam(const char* keyVal, const char* val)
{
	if (keyVal == nullptr || val == nullptr)
	{
		return ERROR_PARAM_INVALIDATE;
	}

	m_mapReqParams[keyVal] = val;
	return 0;
}

int LoginClient::GetParam(const char* keyVal, char* val)
{
	if (keyVal == nullptr || val == nullptr)
	{
		return ERROR_PARAM_INVALIDATE;
	}

	memcpy(val, m_mapRespParams[keyVal].c_str(), m_mapRespParams[keyVal].size());
	return 0;
}


bool LoginClient::ParsePromotionUrl(const string url, string& promotionId)
{
	TRACET();
	const string strPId = "promotionId=";

	int n_off = 0;
	n_off = url.find(strPId);
	if (n_off > 0)
	{
		n_off += strPId.length();
		int id = atoi(url.substr(n_off).c_str());
		promotionId = IntToStr(id);

		return true;
	}

	return false;
}

void LoginClient::ClearPushMessageVerifyCheckCodeStatus()
{
	TRACET();
	m_isPushMessageCheckCode = false;
	m_pushMsgSessionKey = "";
}

int LoginClient::GetAccountGroup(const char* tgt)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetAccountGroupListRequest(tgt);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::AccountGroupLogin(const char* tgt, const char* sndaId, int autoLoginFlag, int autoLoginKeepTime)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetAccountGroupLoginRequest(tgt, sndaId, autoLoginFlag, autoLoginKeepTime);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::ThirdPartyPollingLogin(const char* companyId, int autoLoginFlag, int autoLoginKeepTime)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetThirdPartyPollingLoginRequest(companyId, autoLoginFlag, autoLoginKeepTime);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::ThirdPartyLogin(const char* companyId, const char* token, int autoLoginFlag, int autoLoginKeepTime, const char* scene, const char* phone, const char* smsCode)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetThirdPartyLoginRequest(companyId, token, autoLoginFlag, autoLoginKeepTime, scene, phone, smsCode);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::CloudGameLogin(const char* tgt, const char* scene/*=nullptr*/)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetCloudGameLoginRequest(tgt, scene);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::SendMiGuSms(const char* phone)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetSendMiGuSmsRequest(phone);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::SendSms(const char* smsSessionKey, const char* phone, int smsType, const char* scene)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetSendSmsRequest(smsSessionKey, phone, smsType, scene);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::CheckCodeToSendSms(const char* smsSessionKey, const char* checkCode, const char* captchaInfo)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetCheckCodeToSendSmsRequest(smsSessionKey, checkCode, captchaInfo);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::SmsLogin(const char* smsSessionKey, const char* smsCode, int autoLoginFlag, int autoLoginKeepTime, int keepLoginFlag)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetSmsLoginRequest(smsSessionKey, smsCode, autoLoginFlag, autoLoginKeepTime, keepLoginFlag);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::UserPrivacyConfig(const char* scene, int privacypolicyversion, int serviceAgreementVersion)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetUserPrivacyConfig(scene, privacypolicyversion, serviceAgreementVersion);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::NewVersionUserPrivacyConfig(const char* scene, int privacypolicyversion)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetNewVersionUserPrivacyConfig(scene, privacypolicyversion);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::FaceVerifyInit(const char* scene)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetFaceVerifyInit(scene);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}

	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::GetFaceCodeResult(const char* contextId)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetFaceCodeResult(contextId);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}

	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::SendAction(const char* contextId, int action)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.SendAction(contextId, action);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}

	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::UeInitClient(const char* hash)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.UeInitClient(hash);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}

	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::UeReport(const char* szExtendData)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.UeReport(szExtendData);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}

	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;

}

int LoginClient::SendSteamPayResult(const char* szSteamId, const char* szOrderId)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.SendSteamPayResult(szSteamId, szOrderId);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}

	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::SendSteamChannelPayResult(const char* szChannel, const char* szTicket, const char* szOrderId)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.SendSteamChannelPayResult(szChannel, szTicket, szOrderId);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}

	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

//获取登录器功能相关配置
int LoginClient::GetClientConfig()
{
	TRACET();

	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetClientConfig();
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::GoDownConfigInit(const char* account, const char* isVoice, const char* m_flowId)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	if (strlen(m_flowId) > 0)
	{
		HttpRequest* request = m_requestProcess.GetGoDownConfigInit(account, isVoice, m_flowId);
		if (request == nullptr)
		{
			return ERROR_FORAT_URL;
		}
		if (m_httpThread->ProcessRequest(request) != 0)
		{
			delete request;
			return ERROR_PROCESSING;
		}
		return 0;
	}
	else
	{
		HttpRequest* request = m_requestProcess.GetGoDownConfigInit(account, isVoice, sms_flowId.c_str());
		if (request == nullptr)
		{
			return ERROR_FORAT_URL;
		}
		if (m_httpThread->ProcessRequest(request) != 0)
		{
			delete request;
			return ERROR_PROCESSING;
		}
		return 0;
	}
}

int LoginClient::GoDownSendSmsCheckCode(const char* captchaInfo)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetGoDownSendSmsCheckCode(captchaInfo, sms_flowId.c_str());
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::GoDownconfirmSendSms(int isVoice)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetGoDownconfirmSendSms(sms_flowId.c_str(), isVoice);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::GoDownconfirmLogin(const char* account, const char* verifyCode, int keepLoginFlag)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetGoDownconfirmLogin(account, sms_flowId.c_str(), verifyCode, keepLoginFlag);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::FastLogin(const char* autoLoginKey)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	auto_login_fast_key = autoLoginKey;

	HttpRequest* request = m_requestProcess.GetFastLogin(autoLoginKey);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::GetTicketNew()
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetTicketNew();
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::FastLoginActiveDeleteAccount(const char* keepLoginKey, const char* inputUserId)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.FastLoginActiveDeleteAccount(keepLoginKey, inputUserId);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::CheckAccountLoginType(const char* tgt)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.CheckAccountLoginType(tgt);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::CheckCalculate(const char* star_sign, const char* date)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.CheckCalculate(star_sign, date);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::CheckInitAdv()
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.CheckInitAdv();
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::SetTraceId(const char* traceId)
{
	TRACET();
	m_requestProcess.adTraceId = traceId;
	return 0;
}

int LoginClient::ClearCodeKey()
{
	TRACET();
	m_requestProcess.m_codeKey = "";
	return 0;
}

int LoginClient::GuardDianSendSms(const char* flowId, const char* phone, int isVoice)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetGuardDianSendSms(phone, isVoice, flowId);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;

}

int LoginClient::GuardDianSendSmsCheckCode(const char* flowId, const char* captchaInfo, int isVoice)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetGuardDianSendSmsCheckCode(captchaInfo, isVoice, flowId);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::GuardDianConfrimSendSmsResult(int ageCheckFlag, const char* flowId, const char* verifyCode, const char* tutorName,
	const char* tutorIdCard, const char* phone)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetGuardDianConfrimSendSmsResult(ageCheckFlag, flowId, verifyCode, tutorName, tutorIdCard, phone);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::SetSdoVersion(const char* sdoVersion)
{
	TRACET();
	if (strlen(sdoVersion) > 0)
	{
		std::string encodeSdoVersion = UrlEncoder::encode_go_down(sdoVersion);
		m_requestProcess.sdoVersion = encodeSdoVersion;
	}
	return 0;
}

int LoginClient::RltLoginKeepLogin(const char* vkey, int keepLoginFlag)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetRltLoginKeepLoginRequest(vkey, keepLoginFlag);
	if (request == nullptr)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::SetRunTimerId(const char* runTimeId)
{
	TRACET();
	if (strlen(runTimeId) > 0)
	{
		std::string encodeRunTimeId = UrlEncoder::encode_go_down(runTimeId);
		m_requestProcess.sdoRunTimerId = encodeRunTimeId;
	}
	return 0;
}

int LoginClient::SetFaceRecognitionUrl(const char* confType, const char* confKey)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetFaceRecognitionUrl(confType, confKey);
	if (request == NULL)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::SetQueryFaceVerifyTicketStatus(const char* appId, const char* ticket)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetQueryFaceVerifyTicketStatus(appId, ticket);
	if (request == NULL)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::SetFaceHolderRegistrationUrl(const char* confType, const char* confKey)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetFaceHolderRegistrationUrl(confType, confKey);
	if (request == NULL)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::GetActiveCodeLogin(const char* activeCode)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetActiveCodeLoginResult(activeCode);
	if (request == NULL)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

/**
 * @brief Gunion-WeGame 初始化（会话初始化 / 业务环境绑定）
 *
 * 业务说明：
 *   - 本接口用于在 Gunion 握手完成后，执行 WeGame 业务侧的初始化流程
 *   - 初始化流程依赖当前已建立的 Gunion 会话（randKey / token）
 *   - 通过初始化接口向服务端绑定当前应用与客户端运行环境信息
 *
 * 协议行为：
 *   - 使用当前 Gunion 会话密钥（randKey）对初始化参数进行 3DES 加密
 *   - 按初始化接口协议规则计算请求签名（signature）
 *   - 向 /v1/account/initialize 接口发起 HTTP 请求
 *
 * 流程说明：
 *   1. 校验当前 Gunion 会话是否就绪（EnsureGunionReady）
 *   2. 构建初始化接口专用的加密参数（3DES + Base64）
 *   3. 计算初始化接口专用的请求签名（signature）
 *   4. 构造初始化 HttpRequest（RequestProcess 仅负责拼装请求）
 *   5. 投递请求至 HttpThread 执行网络通信
 *
 * 调用前置条件：
 *   - 已成功完成 GetPublicKey 流程
 *   - 已成功完成 Gunion Handshake（会话密钥 randKey 已建立）
 *   - 已获取并保存 Gunion token
 *
 * @param appId
 *   - 当前应用的 AppId，用于构建初始化请求的通用参数
 *
 * @return
 *   - 0     初始化请求已成功构造并投递
 *   - 非 0  会话未就绪、参数构造失败、签名失败或请求投递失败
 */
int LoginClient::CheckGunionWegameInit(const char* appId)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();
	// 校验 Gunion 会话状态（token + randKey 必须已就绪）
	//MessageBoxA(NULL, "CheckGunionWegameInit", "CheckGunionWegameInit", 0);
	//if (!EnsureGunionReady())
	//	return ERROR_PROCESSING;

	// ------------------------------------------------------------
	// 协议计算：构建 3DES 加密后的初始化参数（初始化接口专属规则）
	// ------------------------------------------------------------
	std::string encryptedParams;
	if (!BuildGunionWegameInitEncryptedParams(
		/* deviceIdServer */ new_device_id_server,
		/* mac            */ GetMacAddress(),     // 可能为空，允许
		/* windowsDeviceId*/ BuildClientEnvironmentDeviceId(),
		/* randKey        */ m_gunionRandKey,
		encryptedParams))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 协议计算：生成初始化请求签名（初始化接口专属规则）
	// ------------------------------------------------------------
	std::string signature;
	if (!CalcGunionWegameInitSignature(
		/* deviceIdServer */ new_device_id_server,
		/* mac            */ GetMacAddress(),     // 必须和加密使用同一套参数
		/* windowsDeviceId*/ BuildClientEnvironmentDeviceId(),
		/* randKey        */ m_gunionRandKey,
		signature))
	{
		return ERROR_PROCESSING;
	}

	// 构造初始化 HttpRequest（RequestProcess 只负责拼装请求）
	HttpRequest* request = m_requestProcess.GetCheckGunionWegameInitRequest(appId, m_gunionToken, encryptedParams, signature);

	if (request == nullptr)
		return ERROR_FORAT_URL;

	// 请求至 HttpThread 执行网络通信
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}

	return 0;
}

int LoginClient::GetCheckGunionWegameLogin(
	const char* rail_id,
	const char* rail_session_ticket,
	bool is_limited,
	int company_id)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	// ------------------------------------------------------------
	// 校验 Gunion 会话状态（token + randKey 必须已就绪）
	// ------------------------------------------------------------
	//if (!EnsureGunionReady())
	//	return ERROR_PROCESSING;

	// ------------------------------------------------------------
	// 协议计算：构建 3DES 加密后的登录参数（Wegame Login 专属规则）
	// ------------------------------------------------------------
	std::string encryptedParams;
	if (!BuildGunionWegameLoginEncryptedParams(
		/* windowsDeviceId*/ new_device_id_server,
		/* deviceIdServer  */ new_device_id_server,
		/* thirdToken      */ rail_session_ticket,
		/* thirdUserId     */ rail_id,
		/* isLimited       */ is_limited ? "1" : "0",
		/* companyId       */ std::to_string(company_id),
		/* randKey         */ m_gunionRandKey,
		encryptedParams))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 协议计算：生成登录请求签名（Wegame Login 专属规则）
	// ------------------------------------------------------------
	std::string signature;
	if (!CalcGunionWegameLoginSignature(
		/* windowsDeviceId */ new_device_id_server,
		/* deviceIdServer  */ new_device_id_server,
		/* thirdToken      */ rail_session_ticket,
		/* thirdUserId     */ rail_id,
		/* isLimited       */ is_limited ? "1" : "0",
		/* companyId       */ std::to_string(company_id),
		/* randKey         */ m_gunionRandKey,
		signature))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 构造登录 HttpRequest（RequestProcess 只负责拼装请求）
	// ------------------------------------------------------------
	HttpRequest* request = m_requestProcess.GetCheckGunionWegameLoginRequest(m_gunionToken, encryptedParams, signature);

	if (request == nullptr)
		return ERROR_FORAT_URL;

	// ------------------------------------------------------------
	// 请求至 HttpThread 执行网络通信
	// ------------------------------------------------------------
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}

	return 0;
}

int LoginClient::CheckGunionWegameSmsSend(
	const char* phone,
	const char* voice_msg,
	const char* str_new_deviceid_server)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	// ------------------------------------------------------------
	// 保存 phone，用于后续 PicSmsSend
	// ------------------------------------------------------------
	gunion_phone_str = phone ? phone : "";

	// ------------------------------------------------------------
	// 校验 Gunion 会话状态（token + randKey 必须已就绪）
	// ------------------------------------------------------------
	//if (!EnsureGunionReady())
	//	return ERROR_PROCESSING;

	// ------------------------------------------------------------
	// 协议计算：构建 3DES 加密后的短信参数（SmsSend 专属规则）
	// ------------------------------------------------------------
	std::string encryptedParams;
	if (!BuildGunionWegameSmsSendEncryptedParams(
		/* phone            */ phone,
		/* type             */ "5",
		/* supportPic       */ "1",
		/* voiceMsg         */ voice_msg,
		/* sms_new          */ "1",
		/* deviceIdServer  */ str_new_deviceid_server,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		encryptedParams))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 协议计算：生成短信请求签名（SmsSend 专属规则）
	// ------------------------------------------------------------
	std::string signature;
	if (!CalcGunionWegameSmsSendSignature(
		/* phone            */ phone,
		/* type             */ "5",
		/* supportPic       */ "1",
		/* voiceMsg         */ voice_msg,
		/* sms_new          */ "1",
		/* deviceIdServer  */ str_new_deviceid_server,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		signature))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 构造短信发送 HttpRequest（RequestProcess 只负责拼装）
	// ------------------------------------------------------------
	HttpRequest* request = m_requestProcess.GetCheckGunionWegameSmsSendRequest(m_gunionToken, encryptedParams, signature);

	if (request == nullptr)
		return ERROR_FORAT_URL;

	// ------------------------------------------------------------
	// 请求至 HttpThread 执行网络通信
	// ------------------------------------------------------------
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}

	return 0;
}

int LoginClient::CheckGunionWegamePicSmsSend(
	const char* imagecodeType,
	const char* voice_msg,
	const char* captchaInfo,
	const char* str_new_deviceid_server)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	// ------------------------------------------------------------
	// 确保 Gunion 会话状态（token + randKey 已就绪）
	// ------------------------------------------------------------
	//if (!EnsureGunionReady())
	//	return ERROR_PROCESSING;

	// ------------------------------------------------------------
	// 根据 imagecodeType 决定 captchaInfo 填充规则（协议规则）
	//   imagecodeType:
	//     "1" -> 普通图验
	//     "4" -> 极验
	// ------------------------------------------------------------
	std::string captchaInfo1;
	std::string captchaInfo4;

	if (imagecodeType && strcmp(imagecodeType, "1") == 0)
	{
		captchaInfo1 = captchaInfo ? captchaInfo : "";
	}
	else if (imagecodeType && strcmp(imagecodeType, "4") == 0)
	{
		captchaInfo4 = captchaInfo ? captchaInfo : "";
	}

	// ------------------------------------------------------------
	// 协议计算：3DES 加密参数（PicSmsSend 专属规则）
	// ------------------------------------------------------------
	std::string encryptedParams;
	if (!BuildGunionWegamePicSmsSendEncryptedParams(
		/* phone            */ gunion_phone_str,
		/* type             */ "5",
		/* checkCodeGuid    */ gunion_wegame_check_sms_send_CodeGuid,
		/* checkCode        */ captchaInfo1,
		/* outInfo          */ captchaInfo4,
		/* voiceMsg         */ voice_msg,
		/* smsNew           */ "1",
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		encryptedParams))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 协议计算：MD5 签名（PicSmsSend 专属规则）
	// ------------------------------------------------------------
	std::string signature;
	if (!CalcGunionWegamePicSmsSendSignature(
		/* phone            */ gunion_phone_str,
		/* type             */ "5",
		/* checkCodeGuid    */ gunion_wegame_check_sms_send_CodeGuid,
		/* checkCode        */ captchaInfo1,
		/* outInfo          */ captchaInfo4,
		/* voiceMsg         */ voice_msg,
		/* smsNew           */ "1",
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		signature))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 构造 HTTP 请求（RequestProcess 只负责拼装）
	// ------------------------------------------------------------
	HttpRequest* request = m_requestProcess.GetCheckGunionWegamePicSmsSendRequest(m_gunionToken, encryptedParams, signature);

	if (request == nullptr)
		return ERROR_FORAT_URL;

	// ------------------------------------------------------------
	// 请求至 HttpThread 执行网络通信
	// ------------------------------------------------------------
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}

	return 0;
}

int LoginClient::CheckGunionWegameThirdAccountBindPhone(
	const char* phone,
	const char* sms_code,
	const char* str_new_deviceid_server)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	// ------------------------------------------------------------
	// 确保 Gunion 会话状态（token + randKey 已就绪）
	// ------------------------------------------------------------
	//if (!EnsureGunionReady())
	//	return ERROR_PROCESSING;

	// ------------------------------------------------------------
	// 协议计算：3DES 加密参数（ThirdAccountBindPhone 专属）
	// ------------------------------------------------------------
	std::string encryptedParams;
	if (!BuildGunionWegameThirdAccountBindPhoneEncryptedParams(
		/* windowsDeviceId */ BuildClientEnvironmentDeviceId(),
		/* phone           */ phone,
		/* smscode         */ sms_code,
		/* type            */ "5",
		/* scene           */ "gunionWegameLogin",
		/* deviceIdServer  */ str_new_deviceid_server,
		/* randKey         */ m_gunionRandKey,
		encryptedParams))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 协议计算：MD5 签名
	// ------------------------------------------------------------
	std::string signature;
	if (!CalcGunionWegameThirdAccountBindPhoneSignature(
		/* windowsDeviceId */ BuildClientEnvironmentDeviceId(),
		/* phone           */ phone,
		/* smscode         */ sms_code,
		/* type            */ "5",
		/* scene           */ "gunionWegameLogin",
		/* deviceIdServer  */ str_new_deviceid_server,
		/* randKey         */ m_gunionRandKey,
		signature))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 构造 HTTP 请求（RequestProcess 只负责拼装）
	// ------------------------------------------------------------
	HttpRequest* request = m_requestProcess.GetCheckGunionWegameThirdAccountBindPhoneRequest(m_gunionToken, encryptedParams, signature);

	if (request == nullptr)
		return ERROR_FORAT_URL;

	// ------------------------------------------------------------
	// 请求至 HttpThread 执行网络通信
	// ------------------------------------------------------------
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}

	return 0;
}

int LoginClient::CheckGunionWegameFillRealinfo(
	const char* idcard,
	const char* name,
	const char* realInfoFlowId,
	const char* str_new_deviceid_server)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	// ------------------------------------------------------------
	// 确保 Gunion 会话状态（token + randKey 已就绪）
	// ------------------------------------------------------------
	//if (!EnsureGunionReady())
	//	return ERROR_PROCESSING;

	// ------------------------------------------------------------
	// 协议要求：姓名必须使用 UTF-8
	// （旧实现中是 GbkToUtf8）
	// ------------------------------------------------------------
	std::string nameUtf8 = GbkToUtf8(name ? name : "");

	// ------------------------------------------------------------
	// 协议计算：3DES 加密参数（FillRealinfo 专属）
	// ------------------------------------------------------------
	std::string encryptedParams;
	if (!BuildGunionWegameFillRealinfoEncryptedParams(
		/* idcard           */ idcard,
		/* nameUtf8         */ nameUtf8,
		/* realInfoFlowId   */ realInfoFlowId,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		encryptedParams))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 协议计算：MD5 签名
	// ------------------------------------------------------------
	std::string signature;
	if (!CalcGunionWegameFillRealinfoSignature(
		/* idcard           */ idcard,
		/* nameUtf8         */ nameUtf8,
		/* realInfoFlowId   */ realInfoFlowId,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		signature))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 构造 HTTP 请求（RequestProcess 只负责拼装）
	// ------------------------------------------------------------
	HttpRequest* request = m_requestProcess.GetCheckGunionWegameFillRealinfoRequest(m_gunionToken, encryptedParams, signature);

	if (request == nullptr)
		return ERROR_FORAT_URL;

	// ------------------------------------------------------------
	// 请求至 HttpThread 执行网络通信
	// ------------------------------------------------------------
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}

	return 0;
}

int LoginClient::CheckGunionWegameGetTicket(
	int time_s,
	const char* str_new_deviceid_server)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	// ------------------------------------------------------------
	// 确保 Gunion 会话状态（token + randKey 已就绪）
	// ------------------------------------------------------------
	//if (!EnsureGunionReady())
	//	return ERROR_PROCESSING;

	// ------------------------------------------------------------
	// 协议计算：3DES 加密参数
	// ------------------------------------------------------------
	std::string encryptedParams;
	if (!BuildGunionWegameGetTicketEncryptedParams(
		/* time_s        */ time_s,
		/* deviceIdSrv   */ str_new_deviceid_server,
		/* randKey       */ m_gunionRandKey,
		encryptedParams))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 协议计算：MD5 签名
	// ------------------------------------------------------------
	std::string signature;
	if (!CalcGunionWegameGetTicketSignature(
		/* time_s        */ time_s,
		/* deviceIdSrv   */ str_new_deviceid_server,
		/* randKey       */ m_gunionRandKey,
		signature))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 构造 HTTP 请求（RequestProcess 只拼装）
	// ------------------------------------------------------------
	HttpRequest* request = m_requestProcess.GetCheckGunionWegameGetTicketRequest(m_gunionToken, encryptedParams, signature);

	if (request == nullptr)
		return ERROR_FORAT_URL;

	// ------------------------------------------------------------
	// 请求至 HttpThread 执行网络通信
	// ------------------------------------------------------------
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}

	return 0;
}

int LoginClient::CheckGunionWegameGetPayTicket(
	int time_s,
	const char* str_new_deviceid_server)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	// ------------------------------------------------------------
	// 确保 Gunion 会话状态（token + randKey 已就绪）
	// ------------------------------------------------------------
	//if (!EnsureGunionReady())
	//	return ERROR_PROCESSING;

	// ------------------------------------------------------------
	// 协议计算：3DES 加密参数（与 GetTicket 完全一致）
	// ------------------------------------------------------------
	std::string encryptedParams;
	if (!BuildGunionWegameGetTicketEncryptedParams(
		/* time_s        */ time_s,
		/* deviceIdSrv   */ str_new_deviceid_server,
		/* randKey       */ m_gunionRandKey,
		encryptedParams))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 协议计算：MD5 签名（与 GetTicket 完全一致）
	// ------------------------------------------------------------
	std::string signature;
	if (!CalcGunionWegameGetTicketSignature(
		/* time_s        */ time_s,
		/* deviceIdSrv   */ str_new_deviceid_server,
		/* randKey       */ m_gunionRandKey,
		signature))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 构造 HTTP 请求（仅 requestCode 不同）
	// ------------------------------------------------------------
	HttpRequest* request = m_requestProcess.GetCheckGunionWegameGetPayTicketRequest(m_gunionToken, encryptedParams, signature);

	if (request == nullptr)
		return ERROR_FORAT_URL;

	// ------------------------------------------------------------
	// 请求至 HttpThread 执行网络通信
	// ------------------------------------------------------------
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}

	return 0;
}

int LoginClient::CheckDoPrivateRequest(const char* appId, const char* scene)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetCheckDoPrivateRequest(appId, scene);
	if (request == NULL)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::CheckDoSmsSend(
	const char* phone,
	const char* voiceMsg,
	const char* smsType,
	const char* str_new_deviceid_server)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	// ------------------------------------------------------------
	// 保存 phone，用于后续 PicSmsSend
	// ------------------------------------------------------------
	gunion_phone_str = phone ? phone : "";

	// ------------------------------------------------------------
	// 校验 Gunion 会话状态（token + randKey 必须已就绪）
	// ------------------------------------------------------------
	//if (!EnsureGunionReady())
	//	return ERROR_PROCESSING;

	// ------------------------------------------------------------
	// 协议计算：构建 3DES 加密后的短信参数（SmsSend 专属规则）
	// ------------------------------------------------------------
	std::string encryptedParams;
	if (!BuildGunionSmsSendEncryptedParams(
		/* phone            */ phone,
		/* type             */ smsType,
		/* supportPic       */ "1",
		/* voiceMsg         */ voiceMsg,
		/* sms_new          */ "1",
		/* deviceIdServer  */ str_new_deviceid_server,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		encryptedParams))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 协议计算：生成短信请求签名（SmsSend 专属规则）
	// ------------------------------------------------------------
	std::string signature;
	if (!CalcGunionSmsSendSignature(
		/* phone            */ phone,
		/* type             */ smsType,
		/* supportPic       */ "1",
		/* voiceMsg         */ voiceMsg,
		/* sms_new          */ "1",
		/* deviceIdServer  */ str_new_deviceid_server,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		signature))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 构造短信发送 HttpRequest（RequestProcess 只负责拼装）
	// ------------------------------------------------------------
	HttpRequest* request = m_requestProcess.GetCheckGunionSmsSendRequest(m_gunionToken, encryptedParams, signature);

	if (request == nullptr)
		return ERROR_FORAT_URL;

	// ------------------------------------------------------------
	// 请求至 HttpThread 执行网络通信
	// ------------------------------------------------------------
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}

	return 0;
}

int LoginClient::CheckCheckPicSmsSend(
	const char* imagecodeType,
	const char* voiceMsg,
	const char* captchaInfo,
	const char* smsType,
	const char* str_new_deviceid_server)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	// ------------------------------------------------------------
	// 确保 Gunion 会话状态（token + randKey 已就绪）
	// ------------------------------------------------------------
	//if (!EnsureGunionReady())
	//	return ERROR_PROCESSING;

	// ------------------------------------------------------------
	// 根据 imagecodeType 决定 captchaInfo 填充规则（协议规则）
	//   imagecodeType:
	//     "1" -> 普通图验
	//     "4" -> 极验
	// ------------------------------------------------------------
	std::string captchaInfo1;
	std::string captchaInfo4;

	if (imagecodeType && strcmp(imagecodeType, "1") == 0)
	{
		captchaInfo1 = captchaInfo ? captchaInfo : "";
	}
	else if (imagecodeType && strcmp(imagecodeType, "4") == 0)
	{
		captchaInfo4 = captchaInfo ? captchaInfo : "";
	}

	// ------------------------------------------------------------
	// 协议计算：3DES 加密参数（PicSmsSend 专属规则）
	// ------------------------------------------------------------
	std::string encryptedParams;
	if (!BuildGunionPicSmsSendEncryptedParams(
		/* phone            */ gunion_phone_str,
		/* type             */ smsType,
		/* checkCodeGuid    */ gunion_wegame_check_sms_send_CodeGuid,
		/* checkCode        */ captchaInfo1,
		/* outInfo          */ captchaInfo4,
		/* voiceMsg         */ voiceMsg,
		/* smsNew           */ "1",
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		encryptedParams))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 协议计算：MD5 签名（PicSmsSend 专属规则）
	// ------------------------------------------------------------
	std::string signature;
	if (!CalcGunionPicSmsSendSignature(
		/* phone            */ gunion_phone_str,
		/* type             */ smsType,
		/* checkCodeGuid    */ gunion_wegame_check_sms_send_CodeGuid,
		/* checkCode        */ captchaInfo1,
		/* outInfo          */ captchaInfo4,
		/* voiceMsg         */ voiceMsg,
		/* smsNew           */ "1",
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		signature))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 构造 HTTP 请求（RequestProcess 只负责拼装）
	// ------------------------------------------------------------
	HttpRequest* request = m_requestProcess.GetCheckGunionPicSmsSendRequest(m_gunionToken, encryptedParams, signature);

	if (request == nullptr)
		return ERROR_FORAT_URL;

	// ------------------------------------------------------------
	// 请求至 HttpThread 执行网络通信
	// ------------------------------------------------------------
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}

	return 0;
}

int LoginClient::CheckCheckSmsLogin(
	const char* phone,
	const char* sms_code,
	const char* str_new_deviceid_server)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	// ------------------------------------------------------------
	// 校验 Gunion 会话状态（token + randKey 必须已就绪）
	// ------------------------------------------------------------
	//if (!EnsureGunionReady())
	//	return ERROR_PROCESSING;

	// ------------------------------------------------------------
	// 协议计算：构建 3DES 加密后的短信登录参数
	// ------------------------------------------------------------
	std::string encryptedParams;
	if (!BuildGunionSmsLoginEncryptedParams(
		/* sms_new          */ "1",
		/* sms_type         */ "4",
		/* phone            */ phone,
		/* smsCode          */ sms_code,
		/* deviceIdServer  */ str_new_deviceid_server,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		encryptedParams))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 协议计算：生成短信登录签名
	// ------------------------------------------------------------
	std::string signature;
	if (!CalcGunionSmsLoginSignature(
		/* sms_new          */ "1",
		/* sms_type         */ "4",
		/* phone            */ phone,
		/* smsCode          */ sms_code,
		/* deviceIdServer  */ str_new_deviceid_server,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		signature))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 构造短信登录 HttpRequest
	// ------------------------------------------------------------
	HttpRequest* request =
		m_requestProcess.GetCheckGunionSmsLoginRequest(
			m_gunionToken,
			encryptedParams,
			signature);

	if (request == nullptr)
		return ERROR_FORAT_URL;

	// ------------------------------------------------------------
	// 请求至 HttpThread 执行网络通信
	// ------------------------------------------------------------
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}

	return 0;
}

int LoginClient::CheckCheckRealName(
	const char* idcard,
	const char* name,
	const char* str_new_deviceid_server)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	// ------------------------------------------------------------
	// 校验 Gunion 会话状态（token + randKey 必须已就绪）
	// ------------------------------------------------------------
	//if (!EnsureGunionReady())
	//	return ERROR_PROCESSING;

	// ------------------------------------------------------------
	// 协议计算：构建 3DES 加密后的实名认证参数
	// ------------------------------------------------------------
	std::string encryptedParams;
	std::string utfName = GbkToUtf8(name);
	if (!BuildGunionRealNameEncryptedParams(
		/* idcard           */ idcard,
		/* name             */ utfName,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		encryptedParams))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 协议计算：生成实名认证签名
	// ------------------------------------------------------------
	std::string signature;
	if (!CalcGunionRealNameSignature(
		/* idcard           */ idcard,
		/* name             */ utfName,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		signature))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 构造实名认证 HttpRequest
	// ------------------------------------------------------------
	HttpRequest* request =
		m_requestProcess.GetCheckGunionRealNameRequest(
			m_gunionToken,
			encryptedParams,
			signature);

	if (request == nullptr)
		return ERROR_FORAT_URL;

	// ------------------------------------------------------------
	// 请求至 HttpThread 执行网络通信
	// ------------------------------------------------------------
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}

	return 0;
}

int LoginClient::CheckCheckAutoLogin(
	const char* autokey,
	const char* str_new_deviceid_server)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	// ------------------------------------------------------------
	// 校验 Gunion 会话状态（token + randKey 必须已就绪）
	// ------------------------------------------------------------
	//if (!EnsureGunionReady())
	//	return ERROR_PROCESSING;

	// ------------------------------------------------------------
	// 协议计算：构建 3DES 加密后的自动登录参数
	// ------------------------------------------------------------
	std::string encryptedParams;
	if (!BuildGunionAutoLoginEncryptedParams(
		/* autokey          */ autokey,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		encryptedParams))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 协议计算：生成自动登录签名
	// ------------------------------------------------------------
	std::string signature;
	if (!CalcGunionAutoLoginSignature(
		/* autokey          */ autokey,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		signature))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 构造自动登录 HttpRequest
	// ------------------------------------------------------------
	HttpRequest* request =
		m_requestProcess.GetCheckGunionAutoLoginRequest(
			m_gunionToken,
			encryptedParams,
			signature);

	if (request == nullptr)
		return ERROR_FORAT_URL;

	// ------------------------------------------------------------
	// 请求至 HttpThread 执行网络通信
	// ------------------------------------------------------------
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}

	return 0;
}

int LoginClient::CheckCheckLogout(
	const char* autokey,
	const char* str_new_deviceid_server)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	// ------------------------------------------------------------
	// 校验 Gunion 会话状态（token + randKey 必须已就绪）
	// ------------------------------------------------------------
	//if (!EnsureGunionReady())
	//	return ERROR_PROCESSING;

	// ------------------------------------------------------------
	// 协议计算：构建 3DES 加密后的登出参数
	// ------------------------------------------------------------
	std::string encryptedParams;
	if (!BuildGunionLogoutEncryptedParams(
		/* autokey          */ "auto_key_flag", /* 不真正请求autokey只是把token和randkey去除  */
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		encryptedParams))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 协议计算：生成登出签名
	// ------------------------------------------------------------
	std::string signature;
	if (!CalcGunionLogoutSignature(
		/* autokey          */ "auto_key_flag", /* 不真正请求autokey只是把token和randkey去除  */
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		signature))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 构造登出 HttpRequest
	// ------------------------------------------------------------
	HttpRequest* request =
		m_requestProcess.GetCheckGunionLogoutRequest(
			m_gunionToken,
			encryptedParams,
			signature);

	if (request == nullptr)
		return ERROR_FORAT_URL;

	// ------------------------------------------------------------
	// 请求至 HttpThread 执行网络通信
	// ------------------------------------------------------------
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}

	return 0;
}

int LoginClient::CheckDoPwdLogin(
	const char* phone,
	const char* password,
	const char* str_new_deviceid_server)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	// ------------------------------------------------------------
	// 校验 Gunion 会话状态（token + randKey 必须已就绪）
	// ------------------------------------------------------------
	//if (!EnsureGunionReady())
	//	return ERROR_PROCESSING;

	// ------------------------------------------------------------
	// 协议计算：构建 3DES 加密后的密码登录参数
	// ------------------------------------------------------------
	std::string encryptedParams;
	if (!BuildGunionPwdLoginEncryptedParams(
		/* phone            */ phone,
		/* password         */ password,
		/* supportPic       */ "1",
		/* deviceIdServer  */ str_new_deviceid_server,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		encryptedParams))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 协议计算：生成密码登录签名
	// ------------------------------------------------------------
	std::string signature;
	if (!CalcGunionPwdLoginSignature(
		/* phone            */ phone,
		/* password         */ password,
		/* supportPic       */ "1",
		/* deviceIdServer  */ str_new_deviceid_server,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		signature))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 构造密码登录 HttpRequest
	// ------------------------------------------------------------
	HttpRequest* request =
		m_requestProcess.GetCheckGunionPwdLoginRequest(
			m_gunionToken,
			encryptedParams,
			signature);

	if (request == nullptr)
		return ERROR_FORAT_URL;

	// ------------------------------------------------------------
	// 请求至 HttpThread 执行网络通信
	// ------------------------------------------------------------
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}

	return 0;
}

int LoginClient::CheckDoPicPwdLogin(
	const char* imageType,
	const char* captchaInfo,
	const char* str_new_deviceid_server)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	// ------------------------------------------------------------
	// 校验 Gunion 会话状态（token + randKey 必须已就绪）
	// ------------------------------------------------------------
	//if (!EnsureGunionReady())
	//	return ERROR_PROCESSING;

	// ------------------------------------------------------------
	// 根据 imagecodeType 决定 captchaInfo 填充规则（协议规则）
	//   imagecodeType:
	//     "1" -> 普通图验
	//     "4" -> 极验
	// ------------------------------------------------------------
	std::string captchaInfo1;
	std::string captchaInfo4;

	if (imageType && strcmp(imageType, "1") == 0)
	{
		captchaInfo1 = captchaInfo ? captchaInfo : "";
	}
	else if (imageType && strcmp(imageType, "4") == 0)
	{
		captchaInfo4 = captchaInfo ? captchaInfo : "";
	}

	// ------------------------------------------------------------
	// 协议计算：构建 3DES 加密后的图验登录参数
	// ------------------------------------------------------------
	std::string encryptedParams;
	if (!BuildGunionPicPwdLoginEncryptedParams(
		/* checkCodeGuid    */ ghome_check_static_login_CodeGuid.c_str(),
		/* password         */ captchaInfo1,
		/* outInfo          */ captchaInfo4,
		/* deviceIdServer  */ str_new_deviceid_server,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		encryptedParams))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 协议计算：生成图验登录签名
	// ------------------------------------------------------------
	std::string signature;
	if (!CalcGunionPicPwdLoginSignature(
		/* checkCodeGuid    */ ghome_check_static_login_CodeGuid.c_str(),
		/* password         */ captchaInfo1,
		/* outInfo          */ captchaInfo4,
		/* deviceIdServer  */ str_new_deviceid_server,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		signature))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 构造图验密码登录 HttpRequest
	// ------------------------------------------------------------
	HttpRequest* request =
		m_requestProcess.GetCheckGunionPicPwdLoginRequest(
			m_gunionToken,
			encryptedParams,
			signature);

	if (request == nullptr)
		return ERROR_FORAT_URL;

	// ------------------------------------------------------------
	// 请求至 HttpThread 执行网络通信
	// ------------------------------------------------------------
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}

	return 0;
}

int LoginClient::CheckDoThirdLogin(
	const char* company_id,
	const char* third_ticket,
	const char* code_verify,
	const char* openid,
	const char* uid,
	const char* str_new_deviceid_server)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	// ------------------------------------------------------------
	// 校验 Gunion 会话状态（token + randKey 必须已就绪）
	// ------------------------------------------------------------
	//if (!EnsureGunionReady())
	//	return ERROR_PROCESSING;

	// ------------------------------------------------------------
	// 协议计算：构建 3DES 加密后的第三方登录参数
	// ------------------------------------------------------------
	std::string encryptedParams;
	if (!BuildGunionThirdLoginEncryptedParams(
		/* companyId        */ company_id,
		/* thirdTicket      */ third_ticket,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		encryptedParams))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 协议计算：生成第三方登录签名
	// ------------------------------------------------------------
	std::string signature;
	if (!CalcGunionThirdLoginSignature(
		/* companyId        */ company_id,
		/* thirdTicket      */ third_ticket,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		signature))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 构造第三方登录 HttpRequest
	// ------------------------------------------------------------
	HttpRequest* request =
		m_requestProcess.GetCheckGunionThirdLoginRequest(
			m_gunionToken,
			encryptedParams,
			signature);

	if (request == nullptr)
		return ERROR_FORAT_URL;

	// ------------------------------------------------------------
	// 请求至 HttpThread 执行网络通信
	// ------------------------------------------------------------
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}

	return 0;
}

int LoginClient::CheckDoSpecialPwdLogin(
	const char* account,
	const char* password,
	const char* str_new_deviceid_server)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	// ------------------------------------------------------------
	// 校验 Gunion 会话状态（token + randKey 必须已就绪）
	// ------------------------------------------------------------
	//if (!EnsureGunionReady())
	//	return ERROR_PROCESSING;

	// ------------------------------------------------------------
	// 协议计算：构建 3DES 加密后的个性账号密码登录参数
	// ------------------------------------------------------------
	std::string encryptedParams;
	std::string utfAccount = GbkToUtf8(account);
	if (!BuildGunionSpecialPwdLoginEncryptedParams(
		/* account          */ utfAccount,
		/* password         */ password,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		encryptedParams))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 协议计算：生成个性账号密码登录签名
	// ------------------------------------------------------------
	std::string signature;
	if (!CalcGunionSpecialPwdLoginSignature(
		/* account          */ utfAccount,
		/* password         */ password,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		signature))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 构造个性账号密码登录 HttpRequest
	// ------------------------------------------------------------
	HttpRequest* request =
		m_requestProcess.GetCheckGunionSpecialPwdLoginRequest(
			m_gunionToken,
			encryptedParams,
			signature);

	if (request == nullptr)
		return ERROR_FORAT_URL;

	// ------------------------------------------------------------
	// 请求至 HttpThread 执行网络通信
	// ------------------------------------------------------------
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}

	return 0;
}

int LoginClient::CheckDoSpecialPicPwdLogin(
	const char* imageType,
	const char* captchaInfo,
	const char* str_new_deviceid_server)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	// ------------------------------------------------------------
	// 校验 Gunion 会话状态（token + randKey 必须已就绪）
	// ------------------------------------------------------------
	//if (!EnsureGunionReady())
	//	return ERROR_PROCESSING;

	// ------------------------------------------------------------
	// 根据 imagecodeType 决定 captchaInfo 填充规则（协议规则）
	//   imagecodeType:
	//     "1" -> 普通图验
	//     "4" -> 极验
	// ------------------------------------------------------------
	std::string captchaInfo1;
	std::string captchaInfo4;

	if (imageType && strcmp(imageType, "1") == 0)
	{
		captchaInfo1 = captchaInfo ? captchaInfo : "";
	}
	else if (imageType && strcmp(imageType, "4") == 0)
	{
		captchaInfo1 = captchaInfo ? captchaInfo : "";
	}

	// ------------------------------------------------------------
	// 协议计算：构建 3DES 加密后的个性账号图验登录参数
	// ------------------------------------------------------------
	std::string encryptedParams;
	if (!BuildGunionSpecialPicPwdLoginEncryptedParams(
		/* checkCodeGuid    */ ghome_check_special_check_code_guid.c_str(),
		/* checkCode        */ captchaInfo1,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		encryptedParams))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 协议计算：生成个性账号图验登录签名
	// ------------------------------------------------------------
	std::string signature;
	if (!CalcGunionSpecialPicPwdLoginSignature(
		/* checkCodeGuid    */ ghome_check_special_check_code_guid.c_str(),
		/* checkCode        */ captchaInfo1,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		signature))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 构造个性账号图验密码登录 HttpRequest
	// ------------------------------------------------------------
	HttpRequest* request =
		m_requestProcess.GetCheckGunionSpecialPicPwdLoginRequest(
			m_gunionToken,
			encryptedParams,
			signature);

	if (request == nullptr)
		return ERROR_FORAT_URL;

	// ------------------------------------------------------------
	// 请求至 HttpThread 执行网络通信
	// ------------------------------------------------------------
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}

	return 0;
}

int LoginClient::CheckDoSpecialSmsSend(
	const char* account,
	const char* voice_msg,
	const char* str_new_deviceid_server)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	// ------------------------------------------------------------
	// 保存 account / voice_msg（如果后续接口需要复用，可在这里缓存）
	// ------------------------------------------------------------
	// special_account_str = account ? account : "";

	// ------------------------------------------------------------
	// 校验 Gunion 会话状态（token + randKey 必须已就绪）
	// ------------------------------------------------------------
	//if (!EnsureGunionReady())
	//	return ERROR_PROCESSING;

	// ------------------------------------------------------------
	// 协议计算：构建 3DES 加密后的短信参数（个性账号 SmsSend 专属规则）
	// ------------------------------------------------------------
	std::string encryptedParams;
	std::string utfAccount = GbkToUtf8(account);
	if (!BuildGunionSpecialSmsSendEncryptedParams(
		/* account          */ utfAccount,
		/* voiceMsg         */ voice_msg,
		/* sms_new          */ "4",
		/* deviceIdServer   */ str_new_deviceid_server,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		encryptedParams))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 协议计算：生成短信请求签名（个性账号 SmsSend 专属规则）
	// ------------------------------------------------------------
	std::string signature;
	if (!CalcGunionSpecialSmsSendSignature(
		/* account          */ utfAccount,
		/* voiceMsg         */ voice_msg,
		/* sms_new          */ "4",
		/* deviceIdServer   */ str_new_deviceid_server,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		signature))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 构造短信发送 HttpRequest（RequestProcess 只负责拼装）
	// ------------------------------------------------------------
	HttpRequest* request =
		m_requestProcess.GetCheckGunionSpecialSmsSendRequest(
			m_gunionToken,
			encryptedParams,
			signature);

	if (request == nullptr)
		return ERROR_FORAT_URL;

	// ------------------------------------------------------------
	// 请求至 HttpThread 执行网络通信
	// ------------------------------------------------------------
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}

	return 0;
}

int LoginClient::CheckSpecialCheckPicSmsSend(
	const char* imagecodeType,
	const char* captchaInfo,
	const char* str_new_deviceid_server)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	// ------------------------------------------------------------
	// 校验 Gunion 会话状态（token + randKey 必须已就绪）
	// ------------------------------------------------------------
	//if (!EnsureGunionReady())
	//	return ERROR_PROCESSING;

	std::string captchaInfo1;
	std::string captchaInfo4;

	if (imagecodeType && strcmp(imagecodeType, "1") == 0)
	{
		captchaInfo1 = captchaInfo ? captchaInfo : "";
	}
	else if (imagecodeType && strcmp(imagecodeType, "4") == 0)
	{
		captchaInfo4 = captchaInfo ? captchaInfo : "";
	}

	// ------------------------------------------------------------
	// 协议计算：构建 3DES 加密后的图验短信参数（个性账号 PicSmsSend 专属规则）
	// ------------------------------------------------------------
	std::string encryptedParams;
	if (!BuildGunionSpecialPicSmsSendEncryptedParams(
		/* captchaInfo      */ captchaInfo,
		/* smsSendGuid      */ ghome_check_special_sms_send_guid,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		encryptedParams))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 协议计算：生成请求签名（个性账号 PicSmsSend 专属规则）
	// ------------------------------------------------------------
	std::string signature;
	if (!CalcGunionSpecialPicSmsSendSignature(
		/* captchaInfo      */ captchaInfo,
		/* smsSendGuid      */ ghome_check_special_sms_send_guid,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		signature))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 构造 HttpRequest（RequestProcess 只负责拼装）
	// ------------------------------------------------------------
	HttpRequest* request =
		m_requestProcess.GetCheckGunionSpecialPicSmsSendRequest(
			m_gunionToken,
			encryptedParams,
			signature);

	if (request == nullptr)
		return ERROR_FORAT_URL;

	// ------------------------------------------------------------
	// 请求至 HttpThread 执行网络通信
	// ------------------------------------------------------------
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}

	return 0;
}

int LoginClient::CheckDoSpecialConfirmSmsSend(
	const char* str_new_deviceid_server)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	// ------------------------------------------------------------
	// 校验 Gunion 会话状态（token + randKey 必须已就绪）
	// ------------------------------------------------------------
	//if (!EnsureGunionReady())
	//	return ERROR_PROCESSING;

	// ------------------------------------------------------------
	// 协议计算：构建 3DES 加密后的确认发送参数（ConfirmSmsSend 专属规则）
	// ------------------------------------------------------------
	std::string encryptedParams;
	if (!BuildGunionSpecialConfirmSmsSendEncryptedParams(
		/* smsSendGuid      */ ghome_check_special_sms_send_guid,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		encryptedParams))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 协议计算：生成请求签名（ConfirmSmsSend 专属规则）
	// ------------------------------------------------------------
	std::string signature;
	if (!CalcGunionSpecialConfirmSmsSendSignature(
		/* smsSendGuid      */ ghome_check_special_sms_send_guid,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		signature))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 构造 HttpRequest（RequestProcess 只负责拼装）
	// ------------------------------------------------------------
	HttpRequest* request =
		m_requestProcess.GetCheckGunionSpecialConfirmSmsSendRequest(
			m_gunionToken,
			encryptedParams,
			signature);

	if (request == nullptr)
		return ERROR_FORAT_URL;

	// ------------------------------------------------------------
	// 请求至 HttpThread 执行网络通信
	// ------------------------------------------------------------
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}

	return 0;
}

int LoginClient::CheckDoSpecialCheckSmsLogin(
	const char* account,
	const char* sms_code,
	const char* str_new_deviceid_server)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	// ------------------------------------------------------------
	// 校验 Gunion 会话状态（token + randKey 必须已就绪）
	// ------------------------------------------------------------
	//if (!EnsureGunionReady())
	//	return ERROR_PROCESSING;

	// ------------------------------------------------------------
	// 协议计算：构建 3DES 加密后的短信登录参数（SpecialCheckSmsLogin 专属规则）
	// ------------------------------------------------------------
	std::string encryptedParams;
	if (!BuildGunionSpecialCheckSmsLoginEncryptedParams(
		/* smsSendGuid      */ ghome_check_special_sms_send_guid,
		/* smsCode          */ sms_code,
		/* account          */ account,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		encryptedParams))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 协议计算：生成请求签名（SpecialCheckSmsLogin 专属规则）
	// ------------------------------------------------------------
	std::string signature;
	if (!CalcGunionSpecialCheckSmsLoginSignature(
		/* smsSendGuid      */ ghome_check_special_sms_send_guid,
		/* smsCode          */ sms_code,
		/* account          */ account,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		signature))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 构造 HttpRequest（RequestProcess 只负责拼装）
	// ------------------------------------------------------------
	HttpRequest* request =
		m_requestProcess.GetCheckGunionSpecialCheckSmsLoginRequest(
			m_gunionToken,
			encryptedParams,
			signature);

	if (request == nullptr)
		return ERROR_FORAT_URL;

	// ------------------------------------------------------------
	// 请求至 HttpThread 执行网络通信
	// ------------------------------------------------------------
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}

	return 0;
}

int LoginClient::CheckDoLoginArea(
	const char* userid,
	const char* area,
	const char* group,
	const char* str_new_deviceid_server)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	// ------------------------------------------------------------
	// 校验 Gunion 会话状态（token + randKey 必须已就绪）
	// ------------------------------------------------------------
	//if (!EnsureGunionReady())
	//	return ERROR_PROCESSING;

	// ------------------------------------------------------------
	// 协议计算：构建 3DES 加密后的区服记录参数
	// ------------------------------------------------------------
	std::string encryptedParams;
	if (!BuildGunionLoginAreaEncryptedParams(
		/* userid           */ userid,
		/* area             */ area,
		/* group            */ group,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		encryptedParams))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 协议计算：生成区服记录请求签名
	// ------------------------------------------------------------
	std::string signature;
	if (!CalcGunionLoginAreaSignature(
		/* userid           */ userid,
		/* area             */ area,
		/* group            */ group,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		signature))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 构造写入区服记录 HttpRequest（RequestProcess 只负责拼装）
	// ------------------------------------------------------------
	HttpRequest* request =
		m_requestProcess.GetCheckGunionLoginAreaRequest(
			m_gunionToken,
			encryptedParams,
			signature);

	if (request == nullptr)
		return ERROR_FORAT_URL;

	// ------------------------------------------------------------
	// 请求至 HttpThread 执行网络通信
	// ------------------------------------------------------------
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}

	return 0;
}

int LoginClient::CheckDoGetLoginArea(
	const char* str_new_deviceid_server)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	// ------------------------------------------------------------
	// 校验 Gunion 会话状态（token + randKey 必须已就绪）
	// ------------------------------------------------------------
	//if (!EnsureGunionReady())
	//	return ERROR_PROCESSING;

	// ------------------------------------------------------------
	// 协议计算：构建 3DES 加密后的 GetLoginArea 参数
	// ------------------------------------------------------------
	std::string encryptedParams;
	if (!BuildGunionGetLoginAreaEncryptedParams(
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		encryptedParams))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 协议计算：生成 GetLoginArea 请求签名
	// ------------------------------------------------------------
	std::string signature;
	if (!CalcGunionGetLoginAreaSignature(
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		signature))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 构造获取区服记录 HttpRequest（RequestProcess 只负责拼装）
	// ------------------------------------------------------------
	HttpRequest* request =
		m_requestProcess.GetCheckGunionGetLoginAreaRequest(
			m_gunionToken,
			encryptedParams,
			signature);

	if (request == nullptr)
		return ERROR_FORAT_URL;

	// ------------------------------------------------------------
	// 请求至 HttpThread 执行网络通信
	// ------------------------------------------------------------
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}

	return 0;
}

int LoginClient::CheckDoCreatPayEntrance(
	const char* szOrderId,
	const char* szProductId,
	const char* szGroupId,
	const char* szAreaId,
	const char* szExtend,
	int iSSandboxAccount,
	const char* str_new_deviceid_server)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	// ------------------------------------------------------------
	// 校验 Gunion 会话状态（token + randKey 必须已就绪）
	// ------------------------------------------------------------
	//if (!EnsureGunionReady())
	//	return ERROR_PROCESSING;

	// ------------------------------------------------------------
	// 协议计算：构建 3DES 加密后的支付下单参数（PayEntrance 专属规则）
	// ------------------------------------------------------------
	std::string encryptedParams;
	if (!BuildGunionPayEntranceEncryptedParams(
		/* orderId          */ szOrderId,
		/* productId        */ szProductId,
		/* groupId          */ szGroupId,
		/* areaId           */ szAreaId,
		/* extend           */ szExtend,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		encryptedParams))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 协议计算：生成请求签名（PayEntrance 专属规则）
	// ------------------------------------------------------------
	std::string signature;
	if (!CalcGunionPayEntranceSignature(
		/* orderId          */ szOrderId,
		/* productId        */ szProductId,
		/* groupId          */ szGroupId,
		/* areaId           */ szAreaId,
		/* extend           */ szExtend,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		signature))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 构造 HTTP 请求（RequestProcess 只负责拼装）
	// ------------------------------------------------------------
	HttpRequest* request =
		m_requestProcess.GetCheckGunionPayEntranceRequest(
			iSSandboxAccount,
			m_gunionToken,
			encryptedParams,
			signature);

	if (request == nullptr)
		return ERROR_FORAT_URL;

	// ------------------------------------------------------------
	// 请求至 HttpThread 执行网络通信
	// ------------------------------------------------------------
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}

	return 0;
}

int LoginClient::CheckDoCreateCheckPayOrderStatus(
	const char* szOrderId,
	int iSSandboxAccount,
	const char* str_new_deviceid_server)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	// ------------------------------------------------------------
	// 校验 Gunion 会话状态（token + randKey 必须已就绪）
	// ------------------------------------------------------------
	//if (!EnsureGunionReady())
	//	return ERROR_PROCESSING;

	// ------------------------------------------------------------
	// 协议计算：构建 3DES 加密后的订单查询参数（PayOrderStatus 专属规则）
	// ------------------------------------------------------------
	std::string encryptedParams;
	if (!BuildGunionCheckPayOrderStatusEncryptedParams(
		/* orderId          */ szOrderId,
		/* randKey          */ m_gunionRandKey,
		encryptedParams))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 协议计算：生成请求签名（PayOrderStatus 专属规则）
	// ------------------------------------------------------------
	std::string signature;
	if (!CalcGunionCheckPayOrderStatusSignature(
		/* orderId          */ szOrderId,
		/* randKey          */ m_gunionRandKey,
		signature))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 构造 HTTP 请求（RequestProcess 只负责拼装）
	// ------------------------------------------------------------
	HttpRequest* request =
		m_requestProcess.GetCheckGunionCheckPayOrderStatusRequest(
			iSSandboxAccount,
			m_gunionToken,
			encryptedParams,
			signature);

	if (request == nullptr)
		return ERROR_FORAT_URL;

	// ------------------------------------------------------------
	// 请求至 HttpThread 执行网络通信
	// ------------------------------------------------------------
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}

	return 0;
}

int LoginClient::CheckDoCreateCheckActivation(
	const char* activation,
	const char* str_new_deviceid_server)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	// ------------------------------------------------------------
	// 校验 Gunion 会话状态（token + randKey 必须已就绪）
	// ------------------------------------------------------------
	//if (!EnsureGunionReady())
	//	return ERROR_PROCESSING;

	// ------------------------------------------------------------
	// 协议计算：构建 3DES 加密后的激活码参数（Activation 专属规则）
	// ------------------------------------------------------------
	std::string encryptedParams;
	if (!BuildGunionCheckActivationEncryptedParams(
		/* activation       */ activation,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		encryptedParams))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 协议计算：生成请求签名（Activation 专属规则）
	// ------------------------------------------------------------
	std::string signature;
	if (!CalcGunionCheckActivationSignature(
		/* activation       */ activation,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		signature))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 构造 HTTP 请求（RequestProcess 只负责拼装）
	// ------------------------------------------------------------
	HttpRequest* request =
		m_requestProcess.GetCheckGunionCheckActivationRequest(
			m_gunionToken,
			encryptedParams,
			signature);

	if (request == nullptr)
		return ERROR_FORAT_URL;

	// ------------------------------------------------------------
	// 请求至 HttpThread 执行网络通信
	// ------------------------------------------------------------
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}

	return 0;
}

int LoginClient::CheckGameAgreementUrlContent(const char* appId, const char* scene)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.GetCheckGameAgreementUrlContent(appId, scene);
	if (request == NULL)
	{
		return ERROR_FORAT_URL;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}
	return 0;
}

int LoginClient::CheckSetTutorInfo(
	const char* idcard,
	const char* name,
	const char* phone,
	const char* smscode,
	const char* confirmKey,
	const char* str_new_deviceid_server)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	// ------------------------------------------------------------
	// 协议计算：构建 3DES 加密参数
	// ------------------------------------------------------------
	std::string encryptedParams;
	std::string utfName = GbkToUtf8(name);
	if (!BuildGunionSetTutorInfoEncryptedParams(
		/* idcard           */ idcard,
		/* name             */ utfName,
		/* phone            */ phone,
		/* smscode          */ smscode,
		/* confirmKey       */ confirmKey,
		/* deviceIdServer   */ str_new_deviceid_server,
		/* randKey          */ m_gunionRandKey,
		encryptedParams))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 协议计算：生成签名
	// ------------------------------------------------------------
	std::string signature;
	if (!CalcGunionSetTutorInfoSignature(
		idcard,
		utfName,
		phone,
		smscode,
		confirmKey,
		str_new_deviceid_server,
		m_gunionRandKey,
		signature))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 构造 HttpRequest
	// ------------------------------------------------------------
	HttpRequest* request =
		m_requestProcess.GetCheckGunionSetTutorInfoRequest(
			m_gunionToken,
			encryptedParams,
			signature);

	if (!request)
		return ERROR_FORAT_URL;

	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}

	return 0;
}

int LoginClient::CheckDoGunionWegameLogin(
	const char* selectLoginContext,
	const char* companyId,
	const char* str_new_deviceid_server)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	// ------------------------------------------------------------
	// 协议计算：构建 3DES 加密参数
	// ------------------------------------------------------------
	std::string encryptedParams;
	if (!BuildGunionWegameChannelLoginEncryptedParams(
		/* selectLoginContext */ selectLoginContext,
		/* companyId          */ companyId,
		/* deviceIdServer     */ str_new_deviceid_server,
		/* randKey            */ m_gunionRandKey,
		encryptedParams))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 协议计算：生成签名
	// ------------------------------------------------------------
	std::string signature;
	if (!CalcGunionWegameChannelLoginSignature(
		selectLoginContext,
		companyId,
		str_new_deviceid_server,
		m_gunionRandKey,
		signature))
	{
		return ERROR_PROCESSING;
	}

	// ------------------------------------------------------------
	// 构造 HttpRequest
	// ------------------------------------------------------------
	HttpRequest* request =
		m_requestProcess.GetCheckGunionWegameChannelLoginRequest(
			m_gunionToken,
			encryptedParams,
			signature);

	if (!request)
		return ERROR_FORAT_URL;

	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}

	return 0;
}

int LoginClient::CheckDoGunionGetPublicKey()
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request =
		m_requestProcess.BuildGunionLoginGetPublicKeyRequest();
	if (!request)
		return ERROR_FORAT_URL;

	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}

	return 0;
}

int LoginClient::CheckDoGunionHandShake(const char* publicKey)
{
	TRACET();
	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	// ① 生成并保存会话 randKey（唯一入口）
	GenerateGunionRandKey();

	// ② 先拼接参数名
	std::string params("randkey=");
	params += UrlEncoder::Encode(m_gunionRandKey);

	// ③ 使用 RSA 公钥加密 randKey（协议能力在 LoginClient）
	std::string encryptedRandKey;
	if (!EncryptGunionRsaPub(publicKey, params, encryptedRandKey))
	{
		return ERROR_PROCESSING;
	}

	// ④ 构建握手请求（RequestProcess 只负责拼请求）
	HttpRequest* request =
		m_requestProcess.BuildGunionLoginHandshakeRequest(encryptedRandKey);
	if (!request)
		return ERROR_FORAT_URL;

	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return ERROR_PROCESSING;
	}

	return 0;
}

int LoginClient::CheckGunionWegameSetToken(const char* token)
{
	TRACET();
	m_gunionToken = token;
	return 0;
}

int LoginClient::CheckGunionWegameSetRandKey(const char* randKey)
{
	TRACET();
	m_gunionRandKey = randKey;
	return 0;
}

bool LoginClient::EnsureGunionReady()
{
	TRACET();

	// 必须先有 Handshake 对象
	if (!m_gunionHandshake)
	{
		TRACEI("[GUNION] Handshake object is null, create new instance.");
		m_gunionHandshake = std::make_unique<GunionHandshake>(this);
	}
	else
	{
		TRACEI("[GUNION] Handshake object already exists.");
	}

	// 如果已经结束
	if (m_gunionHandshake->IsFinished())
	{
		TRACEI("[GUNION] Handshake already finished.");

		// 成功 → 直接返回
		if (m_gunionHandshake->IsSuccess())
		{
			TRACEI("[GUNION] Handshake success, session is ready.");
			return true;
		}

		// 失败 → 重置并重试
		TRACEW("[GUNION] Handshake failed before, reset and retry.");
		m_gunionHandshake->Reset();
	}
	else
	{
		TRACEI("[GUNION] Handshake not finished yet.");
	}

	// 尚未开始 or 正在进行 → 启动/继续流程
	// 注意：
	//   - Start() 仅表示“已触发握手流程”
	//   - 并不代表握手已经成功
	//   - 若为首次调用，此处通常返回 false

	bool started = m_gunionHandshake->Start();

	TRACEI("[GUNION] Start() invoked, return=%d", started ? 1 : 0);

	return false;   // 明确表示：当前尚未完成握手
}

bool LoginClient::StartGunionGetPublicKey()
{
	TRACET();

	m_requestProcess.SetReqParams(&m_mapReqParams);
	m_mapReqParams.clear();

	HttpRequest* request = m_requestProcess.BuildGunionGetPublicKeyRequest();
	if (request == nullptr)
	{
		return false;
	}
	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return false;
	}
	return true;
}

bool LoginClient::StartGunionHandshake(const std::string& publicKey)
{
	TRACET();

	if (publicKey.empty())
		return false;

	// ① 生成并保存会话 randKey（唯一入口）
	GenerateGunionRandKey();

	// ② 先拼接参数名
	std::string params("randkey=");
	params += UrlEncoder::Encode(m_gunionRandKey);

	// ③ 使用 RSA 公钥加密 randKey（协议能力在 LoginClient）
	std::string encryptedRandKey;
	if (!EncryptGunionRsaPub(publicKey, params, encryptedRandKey))
	{
		return false;
	}

	// ④ 构建握手请求（RequestProcess 只负责拼请求）
	HttpRequest* request = m_requestProcess.BuildGunionHandshakeRequest(encryptedRandKey);

	if (request == nullptr)
	{
		return false;
	}

	if (m_httpThread->ProcessRequest(request) != 0)
	{
		delete request;
		return false;
	}

	return true;
}

void LoginClient::GenerateGunionRandKey()
{
	TRACET();

	static const char CHAR_SET[] =
		"1234567890"
		"QWERTYUIOPASDFGHJKLZXCVBNM"
		"qwertyuiopasdfghjklzxcvbnm"
		"!@#$%^&*()_+-={}[]:\";'<>?,./\\";

	static const size_t CHAR_SET_LEN = strlen(CHAR_SET);

	srand((unsigned)time(nullptr));

	m_gunionRandKey.clear();
	for (int i = 0; i < RAND_KEY_LEN; ++i)
	{
		m_gunionRandKey.append(1, CHAR_SET[rand() % CHAR_SET_LEN]);
	}
}

bool LoginClient::EncryptGunionRsaPub(
	const std::string& publicKey,
	const std::string& plainText,
	std::string& encryptText)
{
	TRACET();
	BIO* keyBio = BIO_new_mem_buf(
		(unsigned char*)publicKey.c_str(), -1);
	if (!keyBio)
		return false;

	RSA* rsa = RSA_new();
	if (!rsa)
	{
		BIO_free(keyBio);
		return false;
	}

	rsa = PEM_read_bio_RSA_PUBKEY(keyBio, &rsa, nullptr, nullptr);
	if (!rsa)
	{
		BIO_free(keyBio);
		return false;
	}

	int keyLen = RSA_size(rsa);
	int blockLen = keyLen - 11;

	char* subText = new char[keyLen + 1];
	memset(subText, 0, keyLen + 1);

	size_t pos = 0;
	int ret = 0;
	encryptText.clear();

	while (pos < plainText.length())
	{
		std::string subStr = plainText.substr(pos, blockLen);
		memset(subText, 0, keyLen + 1);

		ret = RSA_public_encrypt(
			(int)subStr.length(),
			(const unsigned char*)subStr.c_str(),
			(unsigned char*)subText,
			rsa,
			RSA_PKCS1_PADDING);

		if (ret < 0)
		{
			delete[] subText;
			BIO_free(keyBio);
			RSA_free(rsa);
			return false;
		}

		encryptText.append(subText, ret);
		pos += blockLen;
	}

	delete[] subText;
	BIO_free(keyBio);
	RSA_free(rsa);

	encryptText = CSafeBase64::Encode(encryptText);
	return true;
}

std::string LoginClient::CalcGunionSignature(
	const std::map<std::string, std::string>& params)
{
	TRACET();
	std::set<std::string> sortParams;

	for (auto& it : params)
	{
		sortParams.insert(it.first + "=" + it.second);
	}

	std::string raw;
	bool first = true;

	for (auto& it : sortParams)
	{
		if (!first)
			raw += "&";
		raw += it;
		first = false;
	}

	raw += m_gunionRandKey;

	transform(raw.begin(), raw.end(), raw.begin(), ::tolower);

	unsigned char md5Bytes[16] = { 0 };
	MD5((const unsigned char*)raw.c_str(), raw.size(), md5Bytes);

	char signature[33] = { 0 };
	for (int i = 0; i < 16; ++i)
	{
		_snprintf_s(signature + i * 2, 3, _TRUNCATE, "%02X", md5Bytes[i]);
	}

	return signature;
}

// ucs2->utf8->unicode->ansi(国内一般为gbk)
std::string LoginClient::ConvertUcs2ToAnsi(const std::string& ucs2)
{
	TRACET();
	if (ucs2.empty())
	{
		return "";
	}

	// 将含ucs3编码字符串内容转为unicode编码
	std::string utf8;
	for (size_t i = 0; i < ucs2.size();)
	{
		char cur = ucs2[i];
		if (cur == '\\')
		{
			if (i + 1 < ucs2.size())
			{
				char next = ucs2[i + 1];
				if (next == 'u' || next == 'U')
				{
					if (i + 5 < ucs2.size())
					{
						// 将\uffff编码形式的字符串转为utf8二进制内容
						std::string ucs2CharCode = ucs2.substr(i + 2, 4);
						transform(ucs2CharCode.begin(), ucs2CharCode.end(), ucs2CharCode.begin(), ::tolower);
						unsigned short code = (unsigned short)strtoul(ucs2CharCode.c_str(), nullptr, 16);

						if (code > 0)
						{
							if (0x0080 > code)
							{
								/* 1 byte UTF-8 Character.*/
								utf8.append(1, (char)code);
							}
							else if (0x0800 > code)
							{
								/*2 bytes UTF-8 Character.*/
								utf8.append(1, 0xc0 | ((char)(code >> 6)));
								utf8.append(1, 0x80 | ((char)(code & 0x3F)));
							}
							else
							{
								/* 3 bytes UTF-8 Character .*/
								utf8.append(1, 0xE0 | ((char)(code >> 12)));
								utf8.append(1, 0x80 | ((char)((code >> 6) & 0x3F)));
								utf8.append(1, 0x80 | ((char)(code & 0x3F)));
							}

							i += 6;
							continue;
						}
					}
				}
			}
		}
		utf8.append(1, cur);
		++i;
	}

	std::string ansi;

	// 将utf8编码转为unicode编码
	int unicodeBytesLen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), utf8.size(), nullptr, 0);
	WCHAR* unicodeBytes = new WCHAR[unicodeBytesLen];
	if (unicodeBytesLen == MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), utf8.size(), unicodeBytes, unicodeBytesLen))
	{
		// 将unicode编码转为ansi编码
		int ansiBytesLen = WideCharToMultiByte(CP_ACP, 0, unicodeBytes, unicodeBytesLen, nullptr, 0, nullptr, nullptr);
		char* ansiBytes = new char[ansiBytesLen];
		if (ansiBytesLen == WideCharToMultiByte(CP_ACP, 0, unicodeBytes, unicodeBytesLen, ansiBytes, ansiBytesLen, nullptr, nullptr))
		{
			ansi.assign(ansiBytes, ansiBytesLen);
		}
		delete[] ansiBytes;
	}
	delete[] unicodeBytes;

	return ansi;
}

bool LoginClient::DecryptGunion3Des(
	const std::string& base64Cipher,
	std::string& outPlain) const
{
	TRACET();
	if (base64Cipher.empty())
		return false;

	return GunionCrypto::Decrypt3DesBase64(
		base64Cipher,
		m_gunionRandKey,
		outPlain);
}

bool LoginClient::BuildGunionWegameInitEncryptedParams(
	const std::string& deviceIdServer,
	const std::string& mac,
	const std::string& windowsDeviceId,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	TRACET();

	if (randKey.empty())
		return false;

	// 说明：
	//  - Init 协议下，仅使用以下字段
	return GunionCrypto::EncryptWegameInitParams3DesBase64(
		deviceIdServer,
		mac,
		windowsDeviceId,
		randKey,
		outEncryptedParams);
}

bool LoginClient::CalcGunionWegameInitSignature(
	const std::string& deviceIdServer,
	const std::string& mac,
	const std::string& windowsDeviceId,
	const std::string& randKey,
	std::string& outSignature)
{
	TRACET();

	if (randKey.empty())
		return false;

	return GunionCrypto::CalcWegameInitSignatureMd5(
		deviceIdServer,
		mac,
		windowsDeviceId,
		randKey,
		outSignature);
}

bool LoginClient::BuildGunionWegameLoginEncryptedParams(
	const std::string& deviceId,
	const std::string& deviceIdServer,
	const std::string& thirdToken,
	const std::string& thirdUserId,
	const std::string& isLimited,
	const std::string& companyId,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（3DES + Base64）
	// ------------------------------------------------------------
	return GunionCrypto::EncryptWegameLoginParams3DesBase64(
		deviceId,
		deviceIdServer,
		thirdToken,
		thirdUserId,
		isLimited,
		companyId,
		randKey,
		outEncryptedParams);
}

bool LoginClient::CalcGunionWegameLoginSignature(
	const std::string& deviceId,
	const std::string& deviceIdServer,
	const std::string& thirdToken,
	const std::string& thirdUserId,
	const std::string& isLimited,
	const std::string& companyId,
	const std::string& randKey,
	std::string& outSignature)
{
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（MD5 签名）
	// ------------------------------------------------------------
	return GunionCrypto::CalcWegameLoginSignatureMd5(
		deviceId,
		deviceIdServer,
		thirdToken,
		thirdUserId,
		isLimited,
		companyId,
		randKey,
		outSignature
	);
}

bool LoginClient::BuildGunionWegameSmsSendEncryptedParams(
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
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（3DES + Base64）
	// ------------------------------------------------------------
	return GunionCrypto::EncryptWegameSmsSendParams3DesBase64(
		phone,
		type,
		supportPic,
		voiceMsg,
		smsNew,
		deviceId,
		deviceIdServer,
		randKey,
		outEncryptedParams);
}

bool LoginClient::CalcGunionWegameSmsSendSignature(
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
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（MD5 签名）
	// ------------------------------------------------------------
	return GunionCrypto::CalcWegameSmsSendSignatureMd5(
		phone,
		type,
		supportPic,
		voiceMsg,
		smsNew,
		deviceId,
		deviceIdServer,
		randKey,
		outSignature);
}

bool LoginClient::BuildGunionWegamePicSmsSendEncryptedParams(
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
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（3DES + Base64）
	// ------------------------------------------------------------
	return GunionCrypto::EncryptWegamePicSmsSendParams3DesBase64(
		phone,
		type,
		checkCodeGuid,
		checkCode,
		outInfo,
		voiceMsg,
		smsNew,
		deviceIdServer,
		randKey,
		outEncryptedParams);
}

bool LoginClient::CalcGunionWegamePicSmsSendSignature(
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
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（MD5 签名）
	// ------------------------------------------------------------
	return GunionCrypto::CalcWegamePicSmsSendSignatureMd5(
		phone,
		type,
		checkCodeGuid,
		checkCode,
		outInfo,
		voiceMsg,
		smsNew,
		deviceIdServer,
		randKey,
		outSignature);
}

bool LoginClient::BuildGunionWegameThirdAccountBindPhoneEncryptedParams(
	const std::string& deviceId,
	const std::string& phone,
	const std::string& smscode,
	const std::string& type,
	const std::string& scene,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（3DES + Base64）
	// ------------------------------------------------------------
	return GunionCrypto::EncryptWegameThirdAccountBindPhoneParams3DesBase64(
		deviceId,
		phone,
		smscode,
		type,
		scene,
		deviceIdServer,
		randKey,
		outEncryptedParams);
}

bool LoginClient::CalcGunionWegameThirdAccountBindPhoneSignature(
	const std::string& deviceId,
	const std::string& phone,
	const std::string& smscode,
	const std::string& type,
	const std::string& scene,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（MD5 签名）
	// ------------------------------------------------------------
	return GunionCrypto::CalcWegameThirdAccountBindPhoneSignatureMd5(
		deviceId,
		phone,
		smscode,
		type,
		scene,
		deviceIdServer,
		randKey,
		outSignature);
}

bool LoginClient::BuildGunionWegameFillRealinfoEncryptedParams(
	const std::string& idcard,
	const std::string& nameUtf8,
	const std::string& realInfoFlowId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（3DES + Base64）
	// ------------------------------------------------------------
	return GunionCrypto::EncryptWegameFillRealinfoParams3DesBase64(
		idcard,
		nameUtf8,
		realInfoFlowId,
		deviceIdServer,
		randKey,
		outEncryptedParams);
}

bool LoginClient::CalcGunionWegameFillRealinfoSignature(
	const std::string& idcard,
	const std::string& nameUtf8,
	const std::string& realInfoFlowId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（MD5 签名）
	// ------------------------------------------------------------
	return GunionCrypto::CalcWegameFillRealinfoSignatureMd5(
		idcard,
		nameUtf8,
		realInfoFlowId,
		deviceIdServer,
		randKey,
		outSignature);
}

bool LoginClient::BuildGunionWegameGetTicketEncryptedParams(
	int time_s,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（3DES + Base64）
	// ------------------------------------------------------------
	return GunionCrypto::EncryptWegameGetTicketParams3DesBase64(
		time_s,
		deviceIdServer,
		randKey,
		outEncryptedParams);
}

bool LoginClient::CalcGunionWegameGetTicketSignature(
	int time_s,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（MD5 签名）
	// ------------------------------------------------------------
	return GunionCrypto::CalcWegameGetTicketSignatureMd5(
		time_s,
		deviceIdServer,
		randKey,
		outSignature);
}

bool LoginClient::BuildGunionSmsSendEncryptedParams(
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
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（3DES + Base64）
	// ------------------------------------------------------------
	return GunionCrypto::EncryptWegameSmsSendParams3DesBase64(
		phone,
		type,
		supportPic,
		voiceMsg,
		smsNew,
		deviceId,
		deviceIdServer,
		randKey,
		outEncryptedParams);
}

bool LoginClient::CalcGunionSmsSendSignature(
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
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（MD5 签名）
	// ------------------------------------------------------------
	return GunionCrypto::CalcWegameSmsSendSignatureMd5(
		phone,
		type,
		supportPic,
		voiceMsg,
		smsNew,
		deviceId,
		deviceIdServer,
		randKey,
		outSignature);
}

bool LoginClient::BuildGunionPicSmsSendEncryptedParams(
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
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（3DES + Base64）
	// ------------------------------------------------------------
	return GunionCrypto::EncryptWegamePicSmsSendParams3DesBase64(
		phone,
		type,
		checkCodeGuid,
		checkCode,
		outInfo,
		voiceMsg,
		smsNew,
		deviceIdServer,
		randKey,
		outEncryptedParams);
}

bool LoginClient::CalcGunionPicSmsSendSignature(
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
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（MD5 签名）
	// ------------------------------------------------------------
	return GunionCrypto::CalcWegamePicSmsSendSignatureMd5(
		phone,
		type,
		checkCodeGuid,
		checkCode,
		outInfo,
		voiceMsg,
		smsNew,
		deviceIdServer,
		randKey,
		outSignature);
}

bool LoginClient::BuildGunionSmsLoginEncryptedParams(
	const std::string& sms_new,
	const std::string& sms_type,
	const std::string& phone,
	const std::string& smsCode,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（3DES + Base64）
	// ------------------------------------------------------------
	return GunionCrypto::EncryptGunionSmsLoginParams3DesBase64(
		sms_new,
		sms_type,
		phone,
		smsCode,
		windowsDeviceId,
		deviceIdServer,
		randKey,
		outEncryptedParams);
}

bool LoginClient::CalcGunionSmsLoginSignature(
	const std::string& sms_new,
	const std::string& sms_type,
	const std::string& phone,
	const std::string& smsCode,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（MD5 签名）
	// ------------------------------------------------------------
	return GunionCrypto::CalcGunionSmsLoginSignatureMd5(
		sms_new,
		sms_type,
		phone,
		smsCode,
		windowsDeviceId,
		deviceIdServer,
		randKey,
		outSignature);
}

bool LoginClient::BuildGunionRealNameEncryptedParams(
	const std::string& idcard,
	const std::string& name,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（3DES + Base64）
	// ------------------------------------------------------------
	return GunionCrypto::EncryptGunionRealNameParams3DesBase64(
		idcard,
		name,
		deviceIdServer,
		randKey,
		outEncryptedParams);
}

bool LoginClient::CalcGunionRealNameSignature(
	const std::string& idcard,
	const std::string& name,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（MD5 签名）
	// ------------------------------------------------------------
	return GunionCrypto::CalcGunionRealNameSignatureMd5(
		idcard,
		name,
		deviceIdServer,
		randKey,
		outSignature);
}

bool LoginClient::BuildGunionAutoLoginEncryptedParams(
	const std::string& autokey,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（3DES + Base64）
	// ------------------------------------------------------------
	return GunionCrypto::EncryptGunionAutoLoginParams3DesBase64(
		autokey,
		windowsDeviceId,
		deviceIdServer,
		randKey,
		outEncryptedParams);
}

bool LoginClient::CalcGunionAutoLoginSignature(
	const std::string& autokey,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（MD5 签名）
	// ------------------------------------------------------------
	return GunionCrypto::CalcGunionAutoLoginSignatureMd5(
		autokey,
		windowsDeviceId,
		deviceIdServer,
		randKey,
		outSignature);
}

bool LoginClient::BuildGunionLogoutEncryptedParams(
	const std::string& autokey,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（3DES + Base64）
	// ------------------------------------------------------------
	return GunionCrypto::EncryptGunionLogoutParams3DesBase64(
		autokey,
		deviceIdServer,
		randKey,
		outEncryptedParams);
}

bool LoginClient::CalcGunionLogoutSignature(
	const std::string& autokey,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（MD5 签名）
	// ------------------------------------------------------------
	return GunionCrypto::CalcGunionLogoutSignatureMd5(
		autokey,
		deviceIdServer,
		randKey,
		outSignature);
}

bool LoginClient::BuildGunionPwdLoginEncryptedParams(
	const std::string& phone,
	const std::string& password,
	const std::string& supportPic,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（3DES + Base64）
	// ------------------------------------------------------------
	return GunionCrypto::EncryptGunionPwdLoginParams3DesBase64(
		phone,
		password,
		supportPic,
		windowsDeviceId,
		deviceIdServer,
		randKey,
		outEncryptedParams);
}

bool LoginClient::CalcGunionPwdLoginSignature(
	const std::string& phone,
	const std::string& password,
	const std::string& supportPic,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（MD5 签名）
	// ------------------------------------------------------------
	return GunionCrypto::CalcGunionPwdLoginSignatureMd5(
		phone,
		password,
		supportPic,
		windowsDeviceId,
		deviceIdServer,
		randKey,
		outSignature);
}

bool LoginClient::BuildGunionPicPwdLoginEncryptedParams(
	const std::string& checkCodeGuid,
	const std::string& password,
	const std::string& outInfo,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（3DES + Base64）
	// ------------------------------------------------------------
	return GunionCrypto::EncryptGunionPicPwdLoginParams3DesBase64(
		checkCodeGuid,
		password,
		outInfo,
		windowsDeviceId,
		deviceIdServer,
		randKey,
		outEncryptedParams);
}

bool LoginClient::CalcGunionPicPwdLoginSignature(
	const std::string& checkCodeGuid,
	const std::string& password,
	const std::string& outInfo,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（MD5 签名）
	// ------------------------------------------------------------
	return GunionCrypto::CalcGunionPicPwdLoginSignatureMd5(
		checkCodeGuid,
		password,
		outInfo,
		windowsDeviceId,
		deviceIdServer,
		randKey,
		outSignature);
}

bool LoginClient::BuildGunionThirdLoginEncryptedParams(
	const std::string& companyId,
	const std::string& thirdTicket,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（3DES + Base64）
	// ------------------------------------------------------------
	return GunionCrypto::EncryptGunionThirdLoginParams3DesBase64(
		companyId,
		thirdTicket,
		windowsDeviceId,
		deviceIdServer,
		randKey,
		outEncryptedParams);
}

bool LoginClient::CalcGunionThirdLoginSignature(
	const std::string& companyId,
	const std::string& thirdTicket,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（MD5 签名）
	// ------------------------------------------------------------
	return GunionCrypto::CalcGunionThirdLoginSignatureMd5(
		companyId,
		thirdTicket,
		windowsDeviceId,
		deviceIdServer,
		randKey,
		outSignature);
}

bool LoginClient::BuildGunionLoginAreaEncryptedParams(
	const std::string& userid,
	const std::string& area,
	const std::string& group,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（3DES + Base64）
	// ------------------------------------------------------------
	return GunionCrypto::EncryptGunionLoginAreaParams3DesBase64(
		userid,
		area,
		group,
		windowsDeviceId,
		deviceIdServer,
		randKey,
		outEncryptedParams);
}

bool LoginClient::CalcGunionLoginAreaSignature(
	const std::string& userid,
	const std::string& area,
	const std::string& group,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（MD5 签名）
	// ------------------------------------------------------------
	return GunionCrypto::CalcGunionLoginAreaSignatureMd5(
		userid,
		area,
		group,
		windowsDeviceId,
		deviceIdServer,
		randKey,
		outSignature);
}

bool LoginClient::BuildGunionGetLoginAreaEncryptedParams(
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（3DES + Base64）
	// ------------------------------------------------------------
	return GunionCrypto::EncryptGunionGetLoginAreaParams3DesBase64(
		deviceIdServer,
		randKey,
		outEncryptedParams);
}

bool LoginClient::CalcGunionGetLoginAreaSignature(
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（MD5 签名）
	// ------------------------------------------------------------
	return GunionCrypto::CalcGunionGetLoginAreaSignatureMd5(
		deviceIdServer,
		randKey,
		outSignature);
}

bool LoginClient::BuildGunionSpecialSmsSendEncryptedParams(
	const std::string& account,
	const std::string& voiceMsg,
	const std::string& smsNew,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	TRACET();

	// ------------------------------------------------------------
	// 调用协议算法层（3DES + Base64）
	// ------------------------------------------------------------
	return GunionCrypto::EncryptGunionSpecialSmsSendParams3DesBase64(
		account,
		voiceMsg,
		smsNew,
		windowsDeviceId,
		deviceIdServer,
		randKey,
		outEncryptedParams);
}

bool LoginClient::BuildGunionSpecialPwdLoginEncryptedParams(
	const std::string& account,
	const std::string& password,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	TRACET();

	// ------------------------------------------------------------
	// 调用协议算法层（3DES + Base64）
	// ------------------------------------------------------------
	return GunionCrypto::EncryptGunionSpecialPwdLoginParams3DesBase64(
		account,
		password,
		windowsDeviceId,
		deviceIdServer,
		randKey,
		outEncryptedParams);
}

bool LoginClient::CalcGunionSpecialPwdLoginSignature(
	const std::string& account,
	const std::string& password,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	TRACET();

	// ------------------------------------------------------------
	// 调用协议算法层（MD5 签名）
	// ------------------------------------------------------------
	return GunionCrypto::CalcGunionSpecialPwdLoginSignatureMd5(
		account,
		password,
		windowsDeviceId,
		deviceIdServer,
		randKey,
		outSignature);
}

bool LoginClient::BuildGunionSpecialPicPwdLoginEncryptedParams(
	const std::string& checkCodeGuid,
	const std::string& outInfo,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	TRACET();

	// ------------------------------------------------------------
	// 调用协议算法层（3DES + Base64）
	// ------------------------------------------------------------
	return GunionCrypto::EncryptGunionSpecialPicPwdLoginParams3DesBase64(
		checkCodeGuid,
		outInfo,
		windowsDeviceId,
		deviceIdServer,
		randKey,
		outEncryptedParams);
}

bool LoginClient::CalcGunionSpecialPicPwdLoginSignature(
	const std::string& checkCodeGuid,
	const std::string& outInfo,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	TRACET();

	// ------------------------------------------------------------
	// 调用协议算法层（MD5 签名）
	// ------------------------------------------------------------
	return GunionCrypto::CalcGunionSpecialPicPwdLoginSignatureMd5(
		checkCodeGuid,
		outInfo,
		windowsDeviceId,
		deviceIdServer,
		randKey,
		outSignature);
}

bool LoginClient::CalcGunionSpecialSmsSendSignature(
	const std::string& account,
	const std::string& voiceMsg,
	const std::string& smsNew,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	TRACET();

	// ------------------------------------------------------------
	// 调用协议算法层（MD5 签名）
	// ------------------------------------------------------------
	return GunionCrypto::CalcGunionSpecialSmsSendSignatureMd5(
		account,
		voiceMsg,
		smsNew,
		windowsDeviceId,
		deviceIdServer,
		randKey,
		outSignature);
}

bool LoginClient::BuildGunionSpecialPicSmsSendEncryptedParams(
	const std::string& captchaInfo,
	const std::string& smsSendGuid,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	TRACET();

	// ------------------------------------------------------------
	// 调用协议算法层（3DES + Base64）
	// ------------------------------------------------------------
	return GunionCrypto::EncryptGunionSpecialPicSmsSendParams3DesBase64(
		captchaInfo,
		smsSendGuid,
		deviceIdServer,
		randKey,
		outEncryptedParams);
}

bool LoginClient::CalcGunionSpecialPicSmsSendSignature(
	const std::string& captchaInfo,
	const std::string& smsSendGuid,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	TRACET();

	// ------------------------------------------------------------
	// 调用协议算法层（MD5 签名）
	// ------------------------------------------------------------
	return GunionCrypto::CalcGunionSpecialPicSmsSendSignatureMd5(
		captchaInfo,
		smsSendGuid,
		deviceIdServer,
		randKey,
		outSignature);
}

bool LoginClient::BuildGunionSpecialConfirmSmsSendEncryptedParams(
	const std::string& smsSendGuid,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	TRACET();

	// ------------------------------------------------------------
	// 调用协议算法层（3DES + Base64）
	// ------------------------------------------------------------
	return GunionCrypto::EncryptGunionSpecialConfirmSmsSendParams3DesBase64(
		smsSendGuid,
		deviceIdServer,
		randKey,
		outEncryptedParams);
}

bool LoginClient::CalcGunionSpecialConfirmSmsSendSignature(
	const std::string& smsSendGuid,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	TRACET();

	// ------------------------------------------------------------
	// 调用协议算法层（MD5 签名）
	// ------------------------------------------------------------
	return GunionCrypto::CalcGunionSpecialConfirmSmsSendSignatureMd5(
		smsSendGuid,
		deviceIdServer,
		randKey,
		outSignature);
}

bool LoginClient::BuildGunionSpecialCheckSmsLoginEncryptedParams(
	const std::string& smsSendGuid,
	const std::string& smsCode,
	const std::string& account,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	TRACET();

	// ------------------------------------------------------------
	// 调用协议算法层（3DES + Base64）
	// ------------------------------------------------------------
	return GunionCrypto::EncryptGunionSpecialCheckSmsLoginParams3DesBase64(
		smsSendGuid,
		smsCode,
		account,
		windowsDeviceId,
		deviceIdServer,
		randKey,
		outEncryptedParams);
}

bool LoginClient::CalcGunionSpecialCheckSmsLoginSignature(
	const std::string& smsSendGuid,
	const std::string& smsCode,
	const std::string& account,
	const std::string& windowsDeviceId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	TRACET();

	// ------------------------------------------------------------
	// 调用协议算法层（MD5 签名）
	// ------------------------------------------------------------
	return GunionCrypto::CalcGunionSpecialCheckSmsLoginSignatureMd5(
		smsSendGuid,
		smsCode,
		account,
		windowsDeviceId,
		deviceIdServer,
		randKey,
		outSignature);
}

bool LoginClient::BuildGunionPayEntranceEncryptedParams(
	const std::string& orderId,
	const std::string& productId,
	const std::string& groupId,
	const std::string& areaId,
	const std::string& extend,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（3DES + Base64）
	// ------------------------------------------------------------
	return GunionCrypto::EncryptGunionPayEntranceParams3DesBase64(
		orderId,
		productId,
		groupId,
		areaId,
		extend,
		deviceIdServer,
		randKey,
		outEncryptedParams);
}

bool LoginClient::CalcGunionPayEntranceSignature(
	const std::string& orderId,
	const std::string& productId,
	const std::string& groupId,
	const std::string& areaId,
	const std::string& extend,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（MD5 签名）
	// ------------------------------------------------------------
	return GunionCrypto::CalcGunionPayEntranceSignatureMd5(
		orderId,
		productId,
		groupId,
		areaId,
		extend,
		deviceIdServer,
		randKey,
		outSignature);
}

bool LoginClient::BuildGunionCheckPayOrderStatusEncryptedParams(
	const std::string& orderId,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（3DES + Base64）
	// ------------------------------------------------------------
	return GunionCrypto::EncryptGunionCheckPayOrderStatusParams3DesBase64(
		orderId,
		randKey,
		outEncryptedParams);
}

bool LoginClient::CalcGunionCheckPayOrderStatusSignature(
	const std::string& orderId,
	const std::string& randKey,
	std::string& outSignature)
{
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（MD5 签名）
	// ------------------------------------------------------------
	return GunionCrypto::CalcGunionCheckPayOrderStatusSignatureMd5(
		orderId,
		randKey,
		outSignature);
}

bool LoginClient::BuildGunionCheckActivationEncryptedParams(
	const std::string& activation,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（3DES + Base64）
	// ------------------------------------------------------------
	return GunionCrypto::EncryptGunionCheckActivationParams3DesBase64(
		activation,
		deviceIdServer,
		randKey,
		outEncryptedParams);
}

bool LoginClient::CalcGunionCheckActivationSignature(
	const std::string& activation,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（MD5 签名）
	// ------------------------------------------------------------
	return GunionCrypto::CalcGunionCheckActivationSignatureMd5(
		activation,
		deviceIdServer,
		randKey,
		outSignature);
}

bool LoginClient::BuildGunionSetTutorInfoEncryptedParams(
	const std::string& idcard,
	const std::string& name,
	const std::string& phone,
	const std::string& smscode,
	const std::string& confirmKey,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（3DES + Base64）
	// ------------------------------------------------------------
	return GunionCrypto::EncryptGunionSetTutorInfoParams3DesBase64(
		idcard,
		name,
		phone,
		smscode,
		confirmKey,
		deviceIdServer,
		randKey,
		outEncryptedParams);
}

bool LoginClient::CalcGunionSetTutorInfoSignature(
	const std::string& idcard,
	const std::string& name,
	const std::string& phone,
	const std::string& smscode,
	const std::string& confirmKey,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（MD5 签名）
	// ------------------------------------------------------------
	return GunionCrypto::CalcGunionSetTutorInfoSignatureMd5(
		idcard,
		name,
		phone,
		smscode,
		confirmKey,
		deviceIdServer,
		randKey,
		outSignature);
}

bool LoginClient::BuildGunionWegameChannelLoginEncryptedParams(
	const std::string& selectLoginContext,
	const std::string& companyId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outEncryptedParams)
{
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（3DES + Base64）
	// ------------------------------------------------------------
	return GunionCrypto::EncryptGunionWegameChannelLoginParams3DesBase64(
		selectLoginContext,
		companyId,
		deviceIdServer,
		randKey,
		outEncryptedParams);
}

bool LoginClient::CalcGunionWegameChannelLoginSignature(
	const std::string& selectLoginContext,
	const std::string& companyId,
	const std::string& deviceIdServer,
	const std::string& randKey,
	std::string& outSignature)
{
	TRACET();

	if (randKey.empty())
		return false;

	// ------------------------------------------------------------
	// 调用协议算法层（MD5 签名）
	// ------------------------------------------------------------
	return GunionCrypto::CalcGunionWegameChannelLoginSignatureMd5(
		selectLoginContext,
		companyId,
		deviceIdServer,
		randKey,
		outSignature);
}
//
//
//┌────────────────────────────┐
//│ LoginClient                │
//│ - 会话状态                │
//│ - randKey / token         │
//│ - 握手用加密              │  ← 现在迁移的
//│ - EnsureGunionReady       │
//└──────────▲─────────────────┘
//│
//┌──────────┴─────────────────┐
//│ RequestProcess / MsgProcess│
//│ - 拼参数                  │
//│ - 3DES 加密业务参数       │
//│ - 计算接口签名            │  ← 绝大多数接口
//│ - 构造 HttpRequest        │
//└────────────────────────────┘
