#pragma once
///////////////////////////////////////////////////////////////////////////////
// RequestCode —— 所有网络请求的类型标识
//
// 每一个 request code 对应一个 HTTP 请求。
// LoginClient / SdoBaseClient / RequestProcess 会根据 code 判断如何处理。
// 没有遗漏任何一个值。
///////////////////////////////////////////////////////////////////////////////
enum RequestCode
{
	REQ_GetDynamicKey,                 // 获取动态密钥
	REQ_StaticLogin,                   // 静态密码登录
	REQ_AutoLogin,                     // 自动登录
	REQ_CheckCodeLogin,                // 图片验证码登录
	REQ_FcmLogin,                      // 实名登录
	REQ_SsoLogin,                      // SSO 登录（网页态）
	REQ_PhoneCheckCodeLogin,           // 短信验证码登录
	REQ_CodeKeyLogin,                  // 扫码登录（CodeKey）
	REQ_DynamicLogin,                  // 动密登录
	REQ_DynamicLoginVoice,             // 语音动密登录
	REQ_Logout,                        // 登出
	REQ_CheckAccountType,              // 查询账号类型
	REQ_SendPhoneCheckCode,            // 发送短信验证码
	REQ_GetQrCode,                     // 获取二维码
	REQ_GetLoginStatus,                // 获取登录状态
	REQ_ExtendLoginState,              // 延长登录态
	REQ_SendPushMessage,               // 一键登录：发送 Push 消息
	REQ_PushMessageLogin,              // 一键登录：轮询登录
	REQ_RltLogin,                      // 注册即登录
	REQ_GetPushMessageStatus,          // 查询一键登录状态
	REQ_GetAccountInfo,                // 查询账号信息
	REQ_GetLoginHistory,               // 查询登录历史
	REQ_GetPublicKey,                  // 获取 RSA 公钥
	REQ_GetPromotionInfo,              // 获取推广信息
	REQ_PromotionInfoConfirm,          // 确认推广信息
	REQ_GetClientVKey,                 // 获取小票 vKey
	REQ_SendUserAccount,               // 发送账号（反木马）
	REQ_SendPushMessageVerifyCheckCode,// 一键登录验证码
	REQ_GetAccountGroup,               // 获取账号组
	REQ_AccountGroupLogin,             // 账号组登录
	REQ_ThirdPartyPollingLogin,        // 第三方轮询登录
	REQ_ThirdPartyLogin,               // 第三方直接登录
	REQ_FastLogin,                     // 快速登录
	REQ_CancelPushMessageLogin,        // 取消一键登录消息
	REQ_GetSessionIdStates,            // 查询 SessionId 状态
	REQ_KickoffVerify,                 // 踢人：发送短信验证码
	REQ_KickOffVerifyCheckCode,        // 踢人：验证验证码
	REQ_KickoffAccount,                // 踢人：执行踢人
	REQ_GetLoginUserInfo,              // 获取登录用户信息
	REQ_GetLoginAreaInfo,              // 获取区组信息
	REQ_SetLoginUserInfo,              // 设置用户昵称等

	// ------------------- 外部平台登录 -------------------
	REQ_WeGameLogin,                   // WeGame 登录
	REQ_LenovoLogin,                   // 联想登录
	REQ_CloudGameLogin,                // 云游戏登录
	REQ_SendMiGuSms,                   // 咪咕短信
	REQ_SendSms,                       // 发送短信验证码
	REQ_CheckCodeToSendSms,            // 图验 + 短信
	REQ_SmsLogin,                      // 短信登录
	REQ_UserPrivacyConfig,             // 用户隐私协议
	REQ_FaceVerifyInit,                // 人脸识别初始化
	REQ_FaceCodeResult,                // 获取人脸扫码结果
	REQ_FaceSendAction,                // 上报人脸行为
	REQ_GetTicket,                     // 获取 ticket
	REQ_CreateWeGameOrder,             // 创建 WeGame 支付订单
	REQ_WeGameStatus,                  // WeGame 状态（退出事件监听）
	REQ_CreateLxOrder,                 // 联想支付订单
	REQ_UeInitClient,                  // 卫士通初始化
	REQ_QQGameLogin,                   // QQGame 登录
	REQ_UeReport,                      // 卫士通上报数据
	REQ_CreateQQGameOrder,             // QQGame 支付订单
	REQ_QQGameIsLogin,                 // QQGame 票据延期
	REQ_SteamPayResult,                // Steam 端游支付通知
	REQ_SteamChannelPayResult,         // Steam 手游支付通知
	REQ_CreateSteamChannelOrder,       // Steam 手游支付订单
	REQ_CreateCQ4Order,                // 传奇4 支付订单
	REQ_CreateCQ4Query,                // 传奇4 查询订单
	REQ_ThirdLogin,                    // QQ/WX/WB 三方登录
	REQ_ThirdLoginSkin,                // 三方登录皮肤配置
	REQ_SsoLoginAuthCode,              // 盒子：带授权码 SSO
	REQ_CheckAuthCode,                 // 授权码验证
	REQ_CheckLoginTypeAccount,         // 检测账号类型（快速登录）
	REQ_CheckCalculate,                // 星座运势
	REQ_CheckInitAdv,                  // 广告系统初始化 traceId
	REQ_NewVersionUserPrivacyConfig,   // 新版隐私协议
	REQ_GetClientConfig,               // 新版登录器功能配置
	REQ_NewVersionGoDownConfigInit,    // 新版下行短信初始化
	REQ_NewVersionGoDownSendSmsCheckCode,// 新版下行短信图验
	REQ_NewVersionGoDownconfirmSendSms,// 新版下行短信确认发送
	REQ_NewVersionGoDownconfirmLogin,  // 新版下行短信确认登录
	REQ_NewVersionFastLogin,           // 新版快速登录
	REQ_NewVersionGetTicket,           // 新版 ticket
	REQ_FastLoginActiveDeleteAccount,  // 新版主动删除账号
	REQ_GuardDianSendSms,              // 监护人信息短信
	REQ_GuardDianSendSmsCheckCode,     // 监护人图验短信
	REQ_GuardDianConfrimSendSmsResult, // 监护人短信确认
	REQ_RltLoginKeepLogin,             // 注册即登录（带自动登录）
	REQ_GunionWegameInit,                 // Gunion-WeGame 初始化（获取三方登录配置、token、randKey 等）
	REQ_GunionWegameLogin,                // Gunion-WeGame 登录（使用 rail_id + rail_session_ticket）
	REQ_GunionWegameSmsSend,              // Gunion-WeGame 短信发送（普通短信 / 语音短信）
	REQ_GunionWegamePicSmsSend,           // Gunion-WeGame 图形验证码短信发送
	REQ_GunionWegameThirdAccountBindPhone,// Gunion-WeGame 三方账号绑定手机
	REQ_GunionWegameFillRealinfo,          // Gunion-WeGame 实名信息提交
	REQ_GunionWegameGetTicket,             // Gunion-WeGame 获取登录票据（Login Ticket）
	REQ_GunionWegameGetPayTicket,          // Gunion-WeGame 获取支付票据（Pay Ticket）
	REQ_GunionGetPublicKey,                // Gunion 获取公钥
	REQ_GunionHandshake,                   // Gunion 握手（建立会话 / 获取 token）
	REQ_GunionGetPrivateUrl,			//获取隐私展示信息url
	REQ_GunionGetAgreementContent,      // 获取游戏协议内容
	REQ_GunionSmsSend,                 // Gunion 短信发送（普通短信 / 语音短信）
	REQ_GunionPicSmsSend,              // Gunion 图形验证码短信发送
	REQ_GunionSmsLogin,                 // 短信验证码登录
	REQ_GunionSpecialSmsSend,           // 个性账号短信发送
	REQ_GunionSpecialPicSmsSend,        // 个性账号图验短信发送
	REQ_GunionSpecialConfirmSmsSend,    // 个性账号短信确认
	REQ_GunionSpecialSmsLogin,          // 个性账号短信登录
	REQ_GunionPwdLogin,                 // 密码登录
	REQ_GunionPicPwdLogin,              // 图验密码登录
	REQ_GunionSpecialPwdLogin,          // 个性账号密码登录
	REQ_GunionSpecialPicPwdLogin,       // 个性账号图验密码登录
	REQ_GunionThirdLogin,               // 第三方登录（QQ / WX / WB 等）
	REQ_GunionAutoLogin,                // 自动登录
	REQ_GunionLogout,                   // 登出
	REQ_GunionRealName,                 // 实名认证
	REQ_GunionGetLoginArea,             // 获取区服记录
	REQ_GunionSetLoginArea,             // 写入区服记录
	REQ_GunionPayEntrance,              // 创建支付订单
	REQ_GunionPayOrderStatus,           // 查询支付订单状态
	REQ_GunionActivation,               // 激活校验
	REQ_GunionSetTutorLogin,            // 监护人登录
	REQ_GunionWegameChannelLogin,       // Wegame账号绑定登录
	REQ_GunionLoginGetPublicKey,        // GunionLogin 获取公钥
	REQ_GunionLoginHandshake,           // GunionLogin 握手（建立会话 / 获取 token）
	REQ_GetFaceRecognitionUrl,          // 返回人脸验证url
	REQ_QueryFaceVerifyTicketStatus,    // 查询人脸识别票据状态
	REQ_GetFaceHolderRegistrationUrl,   // 返回信息收集url
	REQ_GetActiveCodeLoginResult,       // 激活码登录结果
};

///////////////////////////////////////////////////////////////////////////////
// HttpRequest —— HTTP 请求的完整描述结构体
//
// 注意：RequestProcess 创建 HttpRequest，LoginClient 发起请求。
// 每个字段解释用途，并与 CurlHttpClient 配置对应。
// // HttpRequest —— HTTP 请求的完整描述结构体
//
// 设计说明：
//   - RequestProcess / LoginClient 负责创建并填充 HttpRequest
//   - HttpThread 负责消费并通过 CurlHttpClient 发送请求
//   - 每一个 HttpRequest 对象描述“一次完整的 HTTP 请求行为”
//
// 设计目标：
//   - 支持主域 / 备用域自动切换
//   - 支持 GET / POST 等多种请求方式
//   - 支持自定义 HTTP Header（用于鉴权、签名、Token 等）
//   - 支持不同超时策略（主请求 / 备用请求）
//
// 典型使用场景：
//   - 登录 / 注册 / 短信 / 三方登录 / Gunion-WeGame
//   - 图片下载、验证码、票据获取
//   - 后续可扩展 OAuth / Bearer Token / 自定义签名头
///////////////////////////////////////////////////////////////////////////////
struct HttpRequest
{
public:
	HttpRequest()
	{
		userData = nullptr;
	}

public:
	/**
	 * @brief 请求类型标识（对应 RequestCode 枚举）
	 *
	 * 作用：
	 *   - HttpThread 用于区分接口类型
	 *   - LoginClient::ProcessResponse 根据该值分发解析逻辑
	 */
	int requestCode = 0;

	/**
	 * @brief 主域名（主服务器）
	 *
	 * 示例：
	 *   - login.sdo.com
	 *   - api.wegame.qq.com
	 */
	string hostName;

	/**
	 * @brief 主端口号
	 *
	 * 常见值：
	 *   - 443（HTTPS）
	 *   - 80（HTTP）
	 */
	int port = 0;

	/**
	 * @brief 备用域名（备用服务器）
	 *
	 * 用途：
	 *   - 主域请求失败时自动切换
	 *   - 支持多组域名容灾（host1/host2 → host3/host4）
	 */
	string hostName2;

	/**
	 * @brief 备用端口号
	 */
	int port2 = 0;

	/**
	 * @brief 主请求 URL Path
	 *
	 * 示例：
	 *   - "/sdo/login"
	 *   - "/gunion/wegame/init"
	 */
	string url;

	/**
	 * @brief 备用请求 URL Path
	 *
	 * 说明：
	 *   - 若为空，则默认使用 url
	 *   - 用于接口级 URL 切换（少数特殊接口）
	 */
	string url2;

	/**
	 * @brief HTTP 请求方法
	 *
	 * 常见值：
	 *   - "GET"
	 *   - "POST"
	 */
	string method;

	/**
	 * @brief POST 请求体内容
	 *
	 * 说明：
	 *   - 仅在 method == "POST" 时生效
	 *   - 可承载 JSON / x-www-form-urlencoded 等数据
	 */
	string postData;

	/**
	 * @brief 主请求超时时间（毫秒）
	 */
	int timeout = 0;

	/**
	 * @brief 备用请求超时时间（毫秒）
	 */
	int timeout2 = 0;

	/**
	 * @brief 用户自定义数据指针
	 *
	 * 用途：
	 *   - Request 发起时由 LoginClient 设置
	 *   - 回调时原样返回，用于关联上下文
	 */
	void* userData;

	/**
	 * @brief HTTP 请求头集合
	 *
	 * 设计目的：
	 *   - 支持 Authorization / Token / 签名 / TraceId 等 Header
	 *   - 为 POST / 安全接口提供扩展能力
	 *
	 * 示例：
	 *   - "Content-Type" : "application/json"
	 *   - "Authorization": "Bearer xxx"
	 *   - "X-Sign"       : "abcdef"
	 */
	std::map<std::string, std::string> headers;

	/**
	 * @brief 添加或覆盖一个 HTTP Header
	 *
	 * @param key   Header 名称
	 * @param value Header 值
	 */
	void AddHeader(string key, string value)
	{
		headers[key] = value;
	}

	/**
	 * @brief 获取指定 HTTP Header 的值
	 *
	 * @param key Header 名称
	 * @return Header 值，不存在则返回空字符串
	 */
	string GetHeader(string& key)
	{
		std::map<std::string, std::string>::iterator it = headers.find(key);
		if (it != headers.end())
		{
			return it->second;
		}
		return "";
	}
};

///////////////////////////////////////////////////////////////////////////////
// RequestProcess —— 所有网络请求构建器
//
// 该类负责生成 HttpRequest，设置 URL、参数、公参、签名等。
// LoginClient 调用 RequestProcess 返回 HttpRequest，并提交到 CurlHttpClient。
//
///////////////////////////////////////////////////////////////////////////////
class RequestProcess
{
public:
	// ---------------------- Login 获取动态秘钥系列请求 ----------------------
	HttpRequest* GetGetDynamicKeyRequest(const string& key);

	// 静密码登录（新版带 keepLoginFlag）
	HttpRequest* GetStaticLoginRequest(const char* inputUserId, const char* password,
		int accountDomain, int autoLoginFlag, int autoLoginKeepTime, int isSupportGeetest, int keepLoginFlag);

	HttpRequest* GetStaticLoginRequest2(const char* inputUserId, const char* password,
		int accountDomain, int autoLoginFlag, int autoLoginKeepTime, int isSupportGeetest, int keepLoginFlag);

	// 游戏账号登录
	HttpRequest* GetStaticLoginWithGameAccountRequest(const char* inputUserId, const char* password,
		int accountDomain, int autoLoginFlag, int autoLoginKeepTime, int isSupportGeetest,
		int inputUserType, int keepLoginFlag, const char* scene = nullptr);

	// ---------------------- WeGame / QQGame 顺网 等第三方登录 ----------------------
	HttpRequest* GetWeGameLoginRequest(const char* rail_id, const char* rail_session_ticket, bool is_limited);
	HttpRequest* GetWeGameLoginRequest(const char* rail_id, const char* rail_session_ticket, bool is_limited, int company_id);

	HttpRequest* GetQQGameLoginRequest(const char* openid, const char* openkey, bool is_limited, int company_id);
	HttpRequest* GetQQGameIsLoginRequest(const char* openid, const char* openkey, int company_id);

	HttpRequest* GetThirdLoginRequest(const char* third_token, const char* companyId,
		const char* scene = nullptr, const char* phone = nullptr, const char* smsCode = nullptr,
		const char* extend = nullptr, const char* szIsLimited = nullptr);

	HttpRequest* GetForThirdLoginRequest(const char* third_token, const char* companyId,
		const char* scene = nullptr, const char* phone = nullptr, const char* smsCode = nullptr,
		const char* extend = nullptr, const char* szIsLimited = nullptr);

	// ---------------------- 图验 / 动密 / 防沉迷 ----------------------
	HttpRequest* GetCheckCodeLoginRequest(const char* checkCode, const char* challenge,
		const char* validate, const char* seccode, const char* outinfo, int keepLoginFlag, const char* captchaInfo = nullptr);

	HttpRequest* GetDynamicLoginRequest(const char* password, int keepLoginFlag);
	HttpRequest* GetFcmLoginRequest(const char* realName, const char* idCard, const char* email, int keepLoginFlag);

	// ---------------------- 自动登录 / SSO / 授权码----------------------
	HttpRequest* GetAutoLoginRequest(const char* autoLoginSessionKey);
	HttpRequest* GetSsoLoginRequest(const char* tgt, const char* scene);
	HttpRequest* GetSsoLoginRequest(const char* tgt, const char* scene, int appId);
	HttpRequest* GetAuthCodeRequest(const char* tgt, const char* scene);
	HttpRequest* GetCheckAuthCodeRequest(const char* authCode);

	// ---------------------- 快速登录 / Ticket ----------------------
	HttpRequest* GetFastLoginRequest();
	HttpRequest* GetTicketRequest();

	// ---------------------- 一系列通用业务请求 ----------------------
	HttpRequest* GetPhoneCheckCodeLoginRequest(const char* checkCode);
	HttpRequest* GetLogoutRequest();
	HttpRequest* GetQrCodeLoginRequest(int autoLoginFlag, int autoLoginKeepTime, int keepLoginFlag);

	HttpRequest* GetCheckAccoutTypeRequest(const char* inputUserId);
	HttpRequest* GetSendPhoneCheckCodeRequest(const char* inputUserId, int type);

	HttpRequest* GetSessionIdStatesRequest(const char* sessioId);

	HttpRequest* GetGetQrCodeRequest();
	HttpRequest* GetGetLoginStateRequest();
	HttpRequest* GetGetLoginStateRequest2(const char* tgt);
	HttpRequest* GetExtendLoginStateRequest(const char* tgt);

	HttpRequest* GetPushMessageStatusRequest(const char* inputUserId);
	HttpRequest* GetPushMessageLoginRequest(int autoLoginFlag, int autoLoginKeepTime, int keepLoginFlag);
	HttpRequest* GetCancelPushMessageLoginRequest();
	HttpRequest* GetSendPushMessageRequest(const char* inputUserId, const char* scene = nullptr);

	HttpRequest* GetRltLoginRequest(const char* vkey);
	HttpRequest* GetRltLoginKeepLoginRequest(const char* vkey, int keepLoginFlag);

	HttpRequest* GetGetAccountInfoRequest(const char* tgt);
	HttpRequest* GetGetLoginHistoryRequest(const char* tgt, int queryNumber);

	HttpRequest* GetGetLoginUserInfoRequest(const char* tgt);
	HttpRequest* GetSetLoginUserInfoRequest(const char* tgt, const char* notename);

	HttpRequest* GetGetLoginAreaInfoRequest(int nAppId);

	// ---------------------- 踢人流程 ----------------------
	HttpRequest* GetKickoffAccountVerifyRequest(const char* tgt);
	HttpRequest* GetKickoffProtectCodeRequest(const char* tgt, const char* protectCode);
	HttpRequest* GetKickoffAccountRequest(const char* tgt, const char* checkCode, int nKickoffAppid, int nKickoffAreaid);


	// ---------------------- 公钥 / 推广 / vKey ----------------------
	HttpRequest* GetPublicKeyRequest();
	HttpRequest* GetPromotionInfoRequest();

	void SetFlowId(string flowId) { m_ueFlowId = flowId; }
	string GetFlowId() { return m_ueFlowId; }

	void SetReqParams(map<string, string>* mapReqParams);

	HttpRequest* GetPromotionInfoConfirmRequest(int days);

	HttpRequest* GetClientVKeyRequest(const char* tgt);
	HttpRequest* SendUserAccountRequest(const char* inputUserId);

	// ---------------------- 一键登录验证码 ----------------------
	HttpRequest* GetSendPushMessageVerifyCheckCodeRequest(
		const char* pushMsgSessionKey, const char* checkCode,
		int autoLoginFlag, const char* captchaInfo = nullptr);

	// ---------------------- 账号组 ----------------------
	HttpRequest* GetAccountGroupListRequest(const char* tgt);
	HttpRequest* GetAccountGroupLoginRequest(const char* tgt, const char* sndaId,
		int autoLoginFlag, int autoLoginKeepTime);

	// ---------------------- 第三方登录（轮询） ----------------------
	HttpRequest* GetThirdPartyPollingLoginRequest(const char* companyId,
		int autoLoginFlag, int autoLoginKeepTime);

	HttpRequest* GetThirdPartyLoginRequest(
		const char* companyId, const char* token,
		int autoLoginFlag, int autoLoginKeepTime,
		const char* scene = nullptr, const char* phone = nullptr,
		const char* smsCode = nullptr);

	// ---------------------- 云游戏 ----------------------
	HttpRequest* GetCloudGameLoginRequest(const char* tgt, const char* scene = nullptr);

	// ---------------------- 短信与图验 ----------------------
	HttpRequest* GetSendMiGuSmsRequest(const char* phone);
	HttpRequest* GetSendSmsRequest(const char* smsSessionKey, const char* phone, int smsType, const char* scene = nullptr);
	HttpRequest* GetCheckCodeToSendSmsRequest(const char* smsSessionKey, const char* checkCode, const char* captchaInfo = nullptr);

	HttpRequest* GetSmsLoginRequest(
		const char* smsSessionKey, const char* smsCode,
		int autoLoginFlag, int autoLoginKeepTime, int keepLoginFlag);

	// ---------------------- 隐私协议 ----------------------
	HttpRequest* GetUserPrivacyConfig(const char* scene, int privacypolicyversion, int serviceAgreementVersion);
	HttpRequest* GetNewVersionUserPrivacyConfig(const char* scene, int privacypolicyversion);

	// ---------------------- 人脸识别 ----------------------
	HttpRequest* GetFaceVerifyInit(const char* scene);
	HttpRequest* GetFaceCodeResult(const char* contextId);
	HttpRequest* SendAction(const char* contextId, int action);

	// ---------------------- Wegame支付相关 ----------------------
	HttpRequest* CreateWeGameOrder(const char* ticket, const char* szOrderId,
		const char* szProductId, const char* szGroupId, const char* szAreaId,
		const char* szExtend, const char* szSign, const char* channel);

	HttpRequest* WeGameStatus();

	// ---------------------- 联想支付相关 ----------------------
	HttpRequest* CreateLxOrder(const char* ticket, const char* szOrderId,
		const char* szProductId, const char* szGroupId, const char* szAreaId,
		const char* szExtend, const char* szSign, const char* channel,
		const char* szLenovoTgt, const char* szRole);

	// ---------------------- 三方界面皮肤 ----------------------
	HttpRequest* GetThirdLoginSkin();

	// ---------------------- 传奇4订单支付 / 传奇4订单查询 ----------------------
	HttpRequest* CreateCQ4Order(const char* ticket, const char* szOrderId,
		const char* szProductId, const char* szGroupId, const char* szAreaId,
		const char* szExtend, const char* szSign, const char* channel, int iSSandboxAccount);
	HttpRequest* CreateCQ4Query(const char* ticket, const char* szOrderId,
		const char* szSign, const char* channel, int iSSandboxAccount);

	// ----------------------QQ游戏订单 ----------------------
	HttpRequest* CreateQQGameOrder(const char* ticket, const char* szOrderId,
		const char* szProductId, const char* szGroupId, const char* szAreaId,
		const char* szExtend, const char* szOpenId, const char* szOpenKey,
		const char* szPfKey, const char* szSign, const char* channel);

	// ---------------------- 卫士通 ----------------------
	HttpRequest* UeInitClient(const char* hash);
	HttpRequest* UeReport(const char* szExtendData);

	// ---------------------- Steam端游模式支付通知 ----------------------
	HttpRequest* SendSteamPayResult(const char* szSteamId, const char* szOrderId);

	// ---------------------- Steam手游模式订单 / Steam 手游模式支付通知 ----------------------
	HttpRequest* CreateSteamChannelOrder(const char* ticket, const char* szOrderId,
		const char* szProductId, const char* szGroupId, const char* szAreaId,
		const char* szExtend, const char* szSign, const char* channel);
	HttpRequest* SendSteamChannelPayResult(const char* szChannel, const char* szTicket, const char* szOrderId);

	// ---------------------- 登录器功能配置 ----------------------
	HttpRequest* GetClientConfig();

	// ---------------------- 新版下行短信 ----------------------
	HttpRequest* GetGoDownConfigInit(const char* account, const char* isVoice, const char* flowid);
	HttpRequest* GetGoDownSendSmsCheckCode(const char* captchaInfo, const char* flowid);
	HttpRequest* GetGoDownconfirmSendSms(const char* flowid, int isVoice);
	HttpRequest* GetGoDownconfirmLogin(const char* account, const char* flowid, const char* verifyCode, int autoLoginFlag);

	// ---------------------- 新版快速登录 ----------------------
	HttpRequest* GetFastLogin(const char* autoLoginKey);

	// ---------------------- 新版获取票据 ----------------------
	HttpRequest* GetTicketNew();

	// ---------------------- 新版快速登录删除账号 ----------------------
	HttpRequest* FastLoginActiveDeleteAccount(const char* keepLoginKey, const char* inputUserId);

	// ---------------------- 新版检测账号类型 ----------------------
	HttpRequest* CheckAccountLoginType(const char* tgt);

	// 星座运势
	HttpRequest* CheckCalculate(const char* star_sign, const char* date);

	// ---------------------- 广告初始化系统 ----------------------
	HttpRequest* CheckInitAdv();

	// ---------------------- 监护人信息补填系列请求 ----------------------
	HttpRequest* GetGuardDianSendSms(const char* phone, int isVoice, const char* flowid);
	HttpRequest* GetGuardDianSendSmsCheckCode(const char* captchaInfo, int isVoice, const char* flowid);

	// ---------------------- 监护人信息补填短信确认 ----------------------
	HttpRequest* GetGuardDianConfrimSendSmsResult(
		int ageCheckFlag, const char* flowId, const char* verifyCode,
		const char* tutorName, const char* tutorIdCard, const char* phone);

	// ---------------------- 人身核验/验证票据/收集信息相关功能 ----------------------
	/**
	 * @brief 人脸打开Url获取
	 * 参数：
	 *   - confType      配置类型（当前场景固定：移动端<FACIAL_RECOGNITION_URL>、PC<FACIAL_RECOGNITION_PC_URL>）
	 *   - confKey       配置键（当前场景固定：-1）
	 */
	HttpRequest* GetFaceRecognitionUrl(const char* confType, const char* confKey);

	/**
	 * @brief 查询人脸识别票据状态,验证成功或者失败
	 * 参数：
	 *   - appId         应用ID
	 *   - ticket        人脸核验的票据
	 */
	HttpRequest* GetQueryFaceVerifyTicketStatus(const char* appId, const char* ticket);

	/**
	 * @brief 获取人脸识别页面URL/账号持有人登记页面URL
	 * 参数：
	 *   - confType      配置类型（当前场景固定：移动端<FACIAL_RECOGNITION_URL>、PC<FACIAL_RECOGNITION_PC_URL>）
	 *   - confKey       配置键（当前场景固定：-1）
	 */
	HttpRequest* GetFaceHolderRegistrationUrl(const char* confType, const char* confKey);
	// ---------------------- 人身核验/验证票据/收集信息相关功能 ----------------------
	
	// ---------------------- 登录器激活码相关功能 ----------------------
	/**
	 * @brief 获取人脸识别页面URL/账号持有人登记页面URL
	 * 参数：
	 *   - activeCode    激活码
	 */
	HttpRequest* GetActiveCodeLoginResult(const char* activeCode);
	// ---------------------- 登录器激活码相关功能 ----------------------
	
	// 
	// ---------------------- Gunion-Wegame 登录与初始化 ----------------------
	/**
	 * @brief Gunion-WeGame 初始化请求
	 *
	 * 用于获取三方登录配置、品牌信息、登录文案等初始化数据
	 *
	 * 说明：
	 *   - 加密参数与签名由 LoginClient 计算完成
	 *   - RequestProcess 仅负责拼装 HTTP 请求
	 *
	 * @param appId            应用 AppId
	 * @param gunionToken      Gunion 握手阶段获得的 token
	 * @param encryptedParams  3DES(Base64) 加密后的初始化参数
	 * @param signature        初始化接口请求签名（MD5）
	 *
	 * @return HttpRequest* 网络请求对象
	 */
	HttpRequest* GetCheckGunionWegameInitRequest(
		const char* appId,
		const std::string& gunionToken,
		const std::string& encryptedParams,
		const std::string& signature);

	/**
	 * @brief Gunion-WeGame 登录请求
	 *
	 * 说明：
	 *   - 登录协议参数的加密与签名已在 LoginClient 中完成
	 *   - RequestProcess 仅负责 HTTP 请求拼装
	 *
	 * @param gunionToken      Gunion 握手阶段获得的 token
	 * @param encryptedParams  3DES(Base64) 加密后的登录参数
	 * @param signature        登录接口请求签名（MD5）
	 *
	 * @return HttpRequest* 网络请求对象
	 */
	HttpRequest* GetCheckGunionWegameLoginRequest(
		const std::string& gunionToken,
		const std::string& encryptedParams,
		const std::string& signature);

	// ---------------------- Gunion-Wegame 短信发送 ----------------------

	/**
	 * @brief Gunion-Wegame 短信发送请求
	 *
	 * 说明：
	 *   - 短信发送协议参数的加密与签名已在 LoginClient 中完成
	 *   - RequestProcess 仅负责 HTTP 请求拼装
	 *
	 * @param gunionToken      Gunion 握手阶段获得的 token
	 * @param encryptedParams  3DES(Base64) 加密后的短信参数
	 * @param signature        短信接口请求签名（MD5）
	 *
	 * @return HttpRequest* 网络请求对象
	 */
	HttpRequest* GetCheckGunionWegameSmsSendRequest(
		const std::string& gunionToken,
		const std::string& encryptedParams,
		const std::string& signature);

	// ---------------------- Gunion-Wegame 带图验短信发送 ----------------------
	/**
	 * @brief Gunion-WeGame 带图形验证码短信发送请求
	 *
	 * 说明：
	 *   - 协议参数的加密与签名已在 LoginClient 中完成
	 *   - RequestProcess 仅负责 HTTP 请求拼装
	 *
	 * @param gunionToken      Gunion 握手阶段获得的 token
	 * @param encryptedParams  3DES(Base64) 加密后的请求参数
	 * @param signature        请求签名（MD5）
	 *
	 * @return HttpRequest* 网络请求对象
	 */
	HttpRequest* GetCheckGunionWegamePicSmsSendRequest(
		const std::string& gunionToken,
		const std::string& encryptedParams,
		const std::string& signature);


	// ---------------------- Gunion-Wegame 三方账号绑定与实名 ----------------------

	/**
	 * @brief Gunion-WeGame 三方账号绑定手机号请求
	 *
	 * 说明：
	 *   - 加密参数与签名已由 LoginClient 计算完成
	 *   - RequestProcess 仅负责拼装 HTTP 请求
	 *
	 * @param gunionToken      Gunion 握手阶段获得的 token
	 * @param encryptedParams  3DES(Base64) 加密后的请求参数
	 * @param signature        请求签名（MD5）
	 *
	 * @return HttpRequest* 网络请求对象
	 */
	HttpRequest* GetCheckGunionWegameThirdAccountBindPhoneRequest(
		const std::string& gunionToken,
		const std::string& encryptedParams,
		const std::string& signature);

	/**
	 * @brief Gunion-WeGame 实名信息补填请求
	 *
	 * 说明：
	 *   - 加密参数与签名已由 LoginClient 计算完成
	 *   - RequestProcess 仅负责拼装 HTTP 请求
	 *
	 * @param gunionToken      Gunion 握手阶段获得的 token
	 * @param encryptedParams  3DES(Base64) 加密后的请求参数
	 * @param signature        请求签名（MD5）
	 *
	 * @return HttpRequest* 网络请求对象
	 */
	HttpRequest* GetCheckGunionWegameFillRealinfoRequest(
		const std::string& gunionToken,
		const std::string& encryptedParams,
		const std::string& signature);

	// ---------------------- Gunion-Wegame 票据相关 ----------------------

	/**
	 * @brief Gunion-Wegame 获取登录票据请求
	 *
	 * 说明：
	 *   - 加密参数与签名由 LoginClient 计算完成
	 *   - RequestProcess 仅负责拼装 HTTP 请求
	 */
	HttpRequest* GetCheckGunionWegameGetTicketRequest(
		const std::string& gunionToken,
		const std::string& encryptedParams,
		const std::string& signature);

	/**
	 * @brief Gunion-Wegame 获取支付票据请求
	 *
	 * 说明：
	 *   - 协议与 GetTicket 完全一致，仅 requestCode 不同
	 */
	HttpRequest* GetCheckGunionWegameGetPayTicketRequest(
		const std::string& gunionToken,
		const std::string& encryptedParams,
		const std::string& signature);

public:
	/**
		==========================================================
			GHOME_PC / Gunion HTTP 请求构建层
		==========================================================
	 */
	 /**
	  * @brief 构建隐私协议跳转请求
	  *
	  * 说明：
	  *   - GHOME_PC 隐私协议接口
	  *   - 根据应用ID和业务场景获取对应的隐私协议地址
	  *   - 该接口不走 Gunion 加密逻辑
	  *
	  * @param appId  应用ID（游戏在 GHOME 平台的唯一标识）
	  *               用于区分不同游戏项目
	  *
	  * @param scene  业务场景标识
	  *               例如：
	  *               - "ghome_install"    首次安装
	  *               - "ghome_logout"     注销
	  *               - "ghome_reopen"     登录
	  *               用于区分不同展示策略
	  *
	  * @return 返回构造完成的 HttpRequest 对象
	  *         若返回 nullptr 表示构建失败
	  */
	HttpRequest* GetCheckDoPrivateRequest(
		const char* appId,
		const char* scene);


	// ---------------------- Gunion短信发送 ----------------------

	/**
	 * @brief Gunion 短信发送请求
	 *
	 * 说明：
	 *   - 短信发送协议参数的加密与签名已在 LoginClient 中完成
	 *   - RequestProcess 仅负责 HTTP 请求拼装
	 *
	 * @param gunionToken      Gunion 握手阶段获得的 token
	 * @param encryptedParams  3DES(Base64) 加密后的短信参数
	 * @param signature        短信接口请求签名（MD5）
	 *
	 * @return HttpRequest* 网络请求对象
	 */
	HttpRequest* GetCheckGunionSmsSendRequest(
		const std::string& gunionToken,
		const std::string& encryptedParams,
		const std::string& signature);

	// ---------------------- Gunion 带图验短信发送 ----------------------
	/**
	 * @brief Gunion 带图形验证码短信发送请求
	 *
	 * 说明：
	 *   - 协议参数的加密与签名已在 LoginClient 中完成
	 *   - RequestProcess 仅负责 HTTP 请求拼装
	 *
	 * @param gunionToken      Gunion 握手阶段获得的 token
	 * @param encryptedParams  3DES(Base64) 加密后的请求参数
	 * @param signature        请求签名（MD5）
	 *
	 * @return HttpRequest* 网络请求对象
	 */
	HttpRequest* GetCheckGunionPicSmsSendRequest(
		const std::string& gunionToken,
		const std::string& encryptedParams,
		const std::string& signature);

	/**
	 * @brief 构建短信登录请求
	 *
	 * 说明：
	 *   - gunionToken     握手得到的token
	 *   - encryptedParams 为业务参数加密结果
	 *   - signature       为签名结果
	 */
	HttpRequest* GetCheckGunionSmsLoginRequest(
		const std::string& gunionToken,
		const std::string& encryptedParams,
		const std::string& signature);

	/**
	 * @brief 构建实名认证请求
	 *
	 * 说明：
	 *   - gunionToken     握手得到的token
	 *   - encryptedParams 为业务参数加密结果
	 *   - signature       为签名结果
	 */
	HttpRequest* GetCheckGunionRealNameRequest(
		const std::string& gunionToken,
		const std::string& encryptedParams,
		const std::string& signature);

	/**
	 * @brief 构建自动登录请求
	 *
	 * 说明：
	 *   - gunionToken     握手得到的token
	 *   - encryptedParams 为业务参数加密结果
	 *   - signature       为签名结果
	 */
	HttpRequest* GetCheckGunionAutoLoginRequest(
		const std::string& gunionToken,
		const std::string& encryptedParams,
		const std::string& signature);

	/**
	 * @brief 构建登出请求
	 *
	 * 说明：
	 *   - gunionToken     握手得到的token
	 *   - encryptedParams 为业务参数加密结果
	 *   - signature       为签名结果
	 */
	HttpRequest* GetCheckGunionLogoutRequest(
		const std::string& gunionToken,
		const std::string& encryptedParams,
		const std::string& signature);

	/**
	 * @brief 构建静密登录请求
	 *
	 * 说明：
	 *   - gunionToken     握手得到的token
	 *   - encryptedParams 为业务参数加密结果
	 *   - signature       为签名结果
	 */
	HttpRequest* GetCheckGunionPwdLoginRequest(
		const std::string& gunionToken,
		const std::string& encryptedParams,
		const std::string& signature);

	/**
	 * @brief 构建图验静密登录请求
	 *
	 * 说明：
	 *   - gunionToken     握手得到的token
	 *   - encryptedParams 为业务参数加密结果
	 *   - signature       为签名结果
	 */
	HttpRequest* GetCheckGunionPicPwdLoginRequest(
		const std::string& gunionToken,
		const std::string& encryptedParams,
		const std::string& signature);

	/**
	 * @brief 构建第三方登录请求
	 *
	 * 说明：
	 *   - QQ / WX / WB / Wegame
	 *   - gunionToken     握手得到的token
	 *   - encryptedParams 为业务参数加密结果
	 *   - signature       为签名结果
	 */
	HttpRequest* GetCheckGunionThirdLoginRequest(
		const std::string& gunionToken,
		const std::string& encryptedParams,
		const std::string& signature);

	/**
	 * @brief 构建 Gunion-WeGame 个性账号密码登录请求
	 *
	 * 说明：
	 *   - 用于个性账号 + 密码登录流程
	 *   - gunionToken     为握手阶段获取的会话 token
	 *   - encryptedParams 为业务参数 3DES 加密后的 Base64 字符串
	 *   - signature       为当前接口专属签名结果（MD5 32 位大写 HEX）
	 *
	 * 请求特点：
	 *   - 需携带 gunionToken 作为身份凭证
	 *   - encryptedParams 与 signature 必须匹配
	 *   - 若 token 失效需重新握手
	 *
	 * @param gunionToken     握手阶段获取的 token
	 * @param encryptedParams 业务参数加密结果
	 * @param signature       接口签名结果
	 *
	 * @return
	 *   - 构造完成的 HttpRequest 对象
	 */
	HttpRequest* GetCheckGunionSpecialPwdLoginRequest(
		const std::string& gunionToken,
		const std::string& encryptedParams,
		const std::string& signature);

	/**
	 * @brief 构建 Gunion-WeGame 个性账号图验密码登录请求
	 *
	 * 说明：
	 *   - 用于个性账号 + 图形验证码 + 密码登录流程
	 *   - gunionToken     为握手阶段获取的会话 token
	 *   - encryptedParams 为业务参数 3DES 加密后的 Base64 字符串
	 *   - signature       为当前接口专属签名结果（MD5 32 位大写 HEX）
	 *
	 * 请求特点：
	 *   - 需携带 gunionToken 作为身份凭证
	 *   - encryptedParams 与 signature 必须匹配
	 *   - 若图验失败需重新获取 CodeGuid
	 *   - 若 token 失效需重新握手
	 *
	 * @param gunionToken     握手阶段获取的 token
	 * @param encryptedParams 业务参数加密结果
	 * @param signature       接口签名结果
	 *
	 * @return
	 *   - 构造完成的 HttpRequest 对象
	 */
	HttpRequest* GetCheckGunionSpecialPicPwdLoginRequest(
		const std::string& gunionToken,
		const std::string& encryptedParams,
		const std::string& signature);

	/**
	 * @brief 构建 Gunion-WeGame 写入区服记录请求
	 *
	 * 说明：
	 *   - 用于上报/写入用户登录区服记录
	 *   - gunionToken     为握手阶段获取的会话 token
	 *   - encryptedParams 为业务参数 3DES 加密后的 Base64 字符串
	 *   - signature       为当前接口专属签名结果（MD5 32 位大写 HEX）
	 *
	 * 请求特点：
	 *   - 需携带 gunionToken 作为身份凭证
	 *   - encryptedParams 与 signature 必须匹配
	 *   - 用于记录用户最近登录区服
	 *
	 * @param gunionToken     握手阶段获取的 token
	 * @param encryptedParams 业务参数加密结果
	 * @param signature       接口签名结果
	 *
	 * @return
	 *   - 构造完成的 HttpRequest 对象
	 */
	HttpRequest* GetCheckGunionLoginAreaRequest(
		const std::string& gunionToken,
		const std::string& encryptedParams,
		const std::string& signature);

	/**
	 * @brief 构建 Gunion-WeGame 获取区服记录请求
	 *
	 * 说明：
	 *   - 用于获取用户历史登录区服记录
	 *   - gunionToken     为握手阶段获取的会话 token
	 *   - encryptedParams 为业务参数 3DES 加密后的 Base64 字符串
	 *   - signature       为当前接口专属签名结果（MD5 32 位大写 HEX）
	 *
	 * 请求特点：
	 *   - 需携带 gunionToken 作为身份凭证
	 *   - encryptedParams 与 signature 必须匹配
	 *   - 与 LoginArea 写入接口配套使用
	 *
	 * @param gunionToken     握手阶段获取的 token
	 * @param encryptedParams 业务参数加密结果
	 * @param signature       接口签名结果
	 *
	 * @return
	 *   - 构造完成的 HttpRequest 对象
	 */
	HttpRequest* GetCheckGunionGetLoginAreaRequest(
		const std::string& gunionToken,
		const std::string& encryptedParams,
		const std::string& signature);

	/**
	 * @brief 构建 Gunion-WeGame 个性账号短信发送请求
	 *
	 * 说明：
	 *   - 用于个性账号短信发送流程
	 *   - gunionToken     为握手阶段获取的会话 token
	 *   - encryptedParams 为业务参数 3DES 加密后的 Base64 字符串
	 *   - signature       为当前接口专属签名结果（MD5 32 位大写 HEX）
	 *
	 * 请求特点：
	 *   - 需携带 gunionToken 作为身份凭证
	 *   - encryptedParams 与 signature 必须匹配
	 *   - 若触发图形验证码流程，需配合 SpecialPicSmsSend
	 *
	 * @param gunionToken     握手阶段获取的 token
	 * @param encryptedParams 业务参数加密结果
	 * @param signature       接口签名结果
	 *
	 * @return
	 *   - 构造完成的 HttpRequest 对象
	 */
	HttpRequest* GetCheckGunionSpecialSmsSendRequest(
		const std::string& gunionToken,
		const std::string& encryptedParams,
		const std::string& signature);

	/**
	 * @brief 构建 Gunion-WeGame 个性账号图验短信发送请求
	 *
	 * 说明：
	 *   - 用于个性账号短信发送流程中触发图形验证码后的短信发送接口
	 *   - gunionToken     为握手阶段获取的会话 token
	 *   - encryptedParams 为业务参数 3DES 加密后的 Base64 字符串
	 *   - signature       为当前接口专属签名结果（MD5 32 位大写 HEX）
	 *
	 * 请求特点：
	 *   - 需携带 gunionToken 作为身份凭证
	 *   - encryptedParams 与 signature 必须匹配
	 *   - 需配合 imagecodeType / captchaInfo / smsSendGuid
	 *   - 属于个性账号短信流程的一部分
	 *
	 * @param gunionToken     握手阶段获取的 token
	 * @param encryptedParams 业务参数加密结果
	 * @param signature       接口签名结果
	 *
	 * @return
	 *   - 构造完成的 HttpRequest 对象
	 */
	HttpRequest* GetCheckGunionSpecialPicSmsSendRequest(
		const std::string& gunionToken,
		const std::string& encryptedParams,
		const std::string& signature);

	/**
	 * @brief 构建 Gunion-WeGame 个性账号短信发送确认请求
	 *
	 * 说明：
	 *   - 用于个性账号短信发送流程中的确认步骤（ConfirmSmsSend）
	 *   - gunionToken     为握手阶段获取的会话 token
	 *   - encryptedParams 为业务参数 3DES 加密后的 Base64 字符串
	 *   - signature       为当前接口专属签名结果（MD5 32 位大写 HEX）
	 *
	 * 请求特点：
	 *   - 需携带 gunionToken 作为身份凭证
	 *   - encryptedParams 与 signature 必须匹配
	 *   - 需配合 smsSendGuid 使用
	 *   - 属于个性账号短信登录事务流程的一部分
	 *
	 * @param gunionToken     握手阶段获取的 token
	 * @param encryptedParams 业务参数加密结果
	 * @param signature       接口签名结果
	 *
	 * @return
	 *   - 构造完成的 HttpRequest 对象
	 */
	HttpRequest* GetCheckGunionSpecialConfirmSmsSendRequest(
		const std::string& gunionToken,
		const std::string& encryptedParams,
		const std::string& signature);

	/**
	 * @brief 构建 Gunion-WeGame 个性账号短信验证码登录校验请求
	 *
	 * 说明：
	 *   - 用于个性账号短信登录流程中的验证码校验步骤
	 *   - gunionToken     为握手阶段获取的会话 token
	 *   - encryptedParams 为业务参数 3DES 加密后的 Base64 字符串
	 *   - signature       为当前接口专属签名结果（MD5 32 位大写 HEX）
	 *
	 * 请求特点：
	 *   - 需携带 gunionToken 作为身份凭证
	 *   - encryptedParams 与 signature 必须匹配
	 *   - 需配合 smsSendGuid / smsCode / account 使用
	 *   - 属于个性账号短信登录事务流程的最终校验步骤
	 *
	 * @param gunionToken     握手阶段获取的 token
	 * @param encryptedParams 业务参数加密结果
	 * @param signature       接口签名结果
	 *
	 * @return
	 *   - 构造完成的 HttpRequest 对象
	 */
	HttpRequest* GetCheckGunionSpecialCheckSmsLoginRequest(
		const std::string& gunionToken,
		const std::string& encryptedParams,
		const std::string& signature);

	/**
	 * @brief 构建 Gunion 支付入口请求（PayEntrance）
	 *
	 * 说明：
	 *   - 用于发起支付入口流程（创建/进入支付）
	 *   - iSSandboxAccount     是否走沙盒域名
	 *   - gunionToken     为握手阶段获取的会话 token
	 *   - encryptedParams 为业务参数 3DES 加密后的 Base64 字符串
	 *   - signature       为当前接口专属签名结果（MD5 32 位大写 HEX）
	 *
	 * 请求特点：
	 *   - 需携带 gunionToken 作为身份凭证
	 *   - encryptedParams 与 signature 必须匹配
	 *   - 支付类接口，对签名校验要求严格
	 *   - 需与 CheckPayOrderStatus 配套使用
	 *
	 * @param iSSandboxAccount     是否走沙盒域名
	 * @param gunionToken     握手阶段获取的 token
	 * @param encryptedParams 业务参数加密结果
	 * @param signature       接口签名结果
	 *
	 * @return
	 *   - 构造完成的 HttpRequest 对象
	 */
	HttpRequest* GetCheckGunionPayEntranceRequest(
		int iSSandboxAccount,
		const std::string& gunionToken,
		const std::string& encryptedParams,
		const std::string& signature);

	/**
	 * @brief 构建 Gunion 查询支付订单状态请求（CheckPayOrderStatus）
	 *
	 * 说明：
	 *   - 用于查询支付订单当前状态
	 *   - iSSandboxAccount     是否走沙盒域名
	 *   - gunionToken     为握手阶段获取的会话 token
	 *   - encryptedParams 为业务参数 3DES 加密后的 Base64 字符串
	 *   - signature       为当前接口专属签名结果（MD5 32 位大写 HEX）
	 *
	 * 请求特点：
	 *   - 需携带 gunionToken 作为身份凭证
	 *   - encryptedParams 与 signature 必须匹配
	 *   - 支付类接口，对签名校验要求严格
	 *   - 通常与 PayEntrance 接口配套使用
	 *
	 * @param iSSandboxAccount     是否走沙盒域名
	 * @param gunionToken     握手阶段获取的 token
	 * @param encryptedParams 业务参数加密结果
	 * @param signature       接口签名结果
	 *
	 * @return
	 *   - 构造完成的 HttpRequest 对象
	 */
	HttpRequest* GetCheckGunionCheckPayOrderStatusRequest(
		int iSSandboxAccount,
		const std::string& gunionToken,
		const std::string& encryptedParams,
		const std::string& signature);

	/**
	 * @brief 构建 Gunion 激活校验请求（CheckActivation）
	 *
	 * 说明：
	 *   - 用于校验激活码/激活标识是否合法
	 *   - gunionToken     为握手阶段获取的会话 token
	 *   - encryptedParams 为业务参数 3DES 加密后的 Base64 字符串
	 *   - signature       为当前接口专属签名结果（MD5 32 位大写 HEX）
	 *
	 * 请求特点：
	 *   - 需携带 gunionToken 作为身份凭证
	 *   - encryptedParams 与 signature 必须匹配
	 *   - 属于安全校验类接口
	 *   - 若 token 失效需重新握手
	 *
	 * @param gunionToken     握手阶段获取的 token
	 * @param encryptedParams 业务参数加密结果
	 * @param signature       接口签名结果
	 *
	 * @return
	 *   - 构造完成的 HttpRequest 对象
	 */
	HttpRequest* GetCheckGunionCheckActivationRequest(
		const std::string& gunionToken,
		const std::string& encryptedParams,
		const std::string& signature);

	/**
	 * @brief 构建获取游戏协议内容请求
	 *
	 * 说明：
	 *   - 用于获取指定游戏在指定场景下的协议内容
	 *   - 属于内容展示类接口
	 *   - 不涉及账号安全
	 *   - 不需要 Gunion token
	 *   - 不需要 3DES 加密
	 *   - 不需要签名
	 *
	 * 请求参数说明：
	 *   - appId  游戏应用标识
	 *   - scene  协议展示场景（如：ghome_install / ghome_logout / ghome_reopen 等）
	 *
	 * 使用场景：
	 *   - 登录前展示用户协议
	 *   - 注册时展示隐私政策
	 *   - 支付前展示支付协议
	 *
	 * @param appId 游戏应用标识
	 * @param scene 协议展示场景标识
	 *
	 * @return
	 *   - 构造完成的 HttpRequest 对象
	 */
	HttpRequest* GetCheckGameAgreementUrlContent(
		const std::string& appId,
		const std::string& scene);

	/**
	 * @brief 构建 Gunion 监护人信息提交请求（SetTutorInfo）
	 *
	 * 说明：
	 *   - 用于未成年人实名认证后的监护人信息提交
	 *   - gunionToken     为握手阶段获取的会话 token
	 *   - encryptedParams 为业务参数 3DES 加密后的 Base64 字符串
	 *   - signature       为当前接口专属签名结果（MD5 32 位大写 HEX）
	 *
	 * 请求特点：
	 *   - 属于账号安全类接口
	 *   - 需携带 gunionToken 作为身份凭证
	 *   - encryptedParams 与 signature 必须匹配
	 *   - 若 token 失效需重新握手
	 *
	 * @param gunionToken     握手阶段获取的 token
	 * @param encryptedParams 业务参数加密结果
	 * @param signature       接口签名结果
	 *
	 * @return
	 *   - 构造完成的 HttpRequest 对象
	 */
	HttpRequest* GetCheckGunionSetTutorInfoRequest(
		const std::string& gunionToken,
		const std::string& encryptedParams,
		const std::string& signature);

	/**
	 * @brief 构建 Gunion-WeGame 渠道登录请求（ChannelLogin）
	 *
	 * 说明：
	 *   - 用于 Gunion-WeGame 渠道登录流程
	 *   - gunionToken     为握手阶段获取的会话 token
	 *   - encryptedParams 为业务参数 3DES 加密后的 Base64 字符串
	 *   - signature       为当前接口专属签名结果（MD5 32 位大写 HEX）
	 *
	 * 请求特点：
	 *   - 属于渠道登录类接口
	 *   - 需携带 gunionToken 作为身份凭证
	 *   - encryptedParams 与 signature 必须匹配
	 *   - 若 token 失效需重新握手
	 *
	 * @param gunionToken     握手阶段获取的 token
	 * @param encryptedParams 业务参数加密结果
	 * @param signature       接口签名结果
	 *
	 * @return
	 *   - 构造完成的 HttpRequest 对象
	 */
	HttpRequest* GetCheckGunionWegameChannelLoginRequest(
		const std::string& gunionToken,
		const std::string& encryptedParams,
		const std::string& signature);

	/**
	 * @brief Gunion 获取 RSA 公钥请求
	 *
	 * 业务说明：
	 *   - Gunion-WeGame 登录体系的第一步安全握手
	 *   - 客户端通过该接口获取服务端下发的 RSA 公钥
	 *   - 后续 NegotiateKey 接口将使用该公钥加密对称密钥
	 *
	 * 使用场景：
	 *   - Gunion-WeGame 登录
	 *   - Gunion-WeGame 短信 / 实名 / 票据等所有加密接口之前
	 *
	 * 调用关系：
	 *   GunionHandshake
	 *     → LoginClient::StartGunionGetPublicKey
	 *       → RequestProcess::BuildGunionGetPublicKeyRequest
	 *
	 * @return
	 *   - HttpRequest* 已构造的网络请求对象
	 *   - 由 LoginClient 投递到 HttpThread 执行
	 */
	HttpRequest* BuildGunionLoginGetPublicKeyRequest();

	/**
	 * @brief Gunion 握手请求（Handshake / 会话建立）
	 *
	 * 业务说明：
	 *   - Gunion 安全握手流程的第二步
	 *   - 使用 GetPublicKey 接口返回的 RSA 公钥
	 *   - 客户端生成随机对称密钥（randKey）
	 *   - 使用 RSA 公钥对 randKey 进行加密后提交给服务端
	 *   - 服务端校验成功后，双方建立会话并进入对称加密通信阶段
	 *
	 * 使用场景：
	 *   - 必须在 GetPublicKey 成功之后调用
	 *   - 握手成功后：
	 *       · 客户端获得会话 token
	 *       · 后续所有 Gunion 接口均使用 randKey 进行 3DES 加密
	 *
	 * 参数说明：
	 * @param publicKey
	 *   - 服务端返回的 RSA 公钥字符串
	 *   - PEM 格式（包含 BEGIN / END PUBLIC KEY）
	 *
	 * 调用关系：
	 *   GunionHandshake
	 *     → LoginClient::StartGunionHandshake
	 *       → RequestProcess::BuildGunionHandshakeRequest
	 *
	 * @return
	 *   - HttpRequest* 已构造的网络请求对象
	 *   - 由 LoginClient 投递至 HttpThread 执行
	 */
	HttpRequest* BuildGunionLoginHandshakeRequest(const std::string& encryptedRandKey);
public:

	//[LoginClient]
	//|
	//	| EnsureGunionReady()
	//	v
	//	[GunionHandshake]
	//|
	//	| Step 1
	//	v
	//	BuildGunionGetPublicKeyRequest
	//	|
	//	| Step 2
	//	v
	//	BuildGunionNegotiateKeyRequest
	//	|
	//	v
	//	[Gunion Secure Session Ready]
	//|
	//	v
	//	GetTicket / PayTicket / SMS / RealName / ...

	// ---------------------- Gunion-WeGame 安全握手相关请求 ----------------------

	/**
	 * @brief Gunion-WeGame 获取 RSA 公钥请求
	 *
	 * 业务说明：
	 *   - Gunion-WeGame 登录体系的第一步安全握手
	 *   - 客户端通过该接口获取服务端下发的 RSA 公钥
	 *   - 后续 NegotiateKey 接口将使用该公钥加密对称密钥
	 *
	 * 使用场景：
	 *   - Gunion-WeGame 登录
	 *   - Gunion-WeGame 短信 / 实名 / 票据等所有加密接口之前
	 *
	 * 调用关系：
	 *   GunionHandshake
	 *     → LoginClient::StartGunionGetPublicKey
	 *       → RequestProcess::BuildGunionGetPublicKeyRequest
	 *
	 * @return
	 *   - HttpRequest* 已构造的网络请求对象
	 *   - 由 LoginClient 投递到 HttpThread 执行
	 */
	HttpRequest* BuildGunionGetPublicKeyRequest();

	/**
	 * @brief Gunion-WeGame 握手请求（Handshake / 会话建立）
	 *
	 * 业务说明：
	 *   - Gunion-WeGame 安全握手流程的第二步
	 *   - 使用 GetPublicKey 接口返回的 RSA 公钥
	 *   - 客户端生成随机对称密钥（randKey）
	 *   - 使用 RSA 公钥对 randKey 进行加密后提交给服务端
	 *   - 服务端校验成功后，双方建立会话并进入对称加密通信阶段
	 *
	 * 使用场景：
	 *   - 必须在 GetPublicKey 成功之后调用
	 *   - 握手成功后：
	 *       · 客户端获得会话 token
	 *       · 后续所有 Gunion 接口均使用 randKey 进行 3DES 加密
	 *
	 * 参数说明：
	 * @param publicKey
	 *   - 服务端返回的 RSA 公钥字符串
	 *   - PEM 格式（包含 BEGIN / END PUBLIC KEY）
	 *
	 * 调用关系：
	 *   GunionHandshake
	 *     → LoginClient::StartGunionHandshake
	 *       → RequestProcess::BuildGunionHandshakeRequest
	 *
	 * @return
	 *   - HttpRequest* 已构造的网络请求对象
	 *   - 由 LoginClient 投递至 HttpThread 执行
	 */
	HttpRequest* BuildGunionHandshakeRequest(const std::string& encryptedRandKey);

private:
	///////////////////////////////////////////////////////////////////////////
	// 公共参数注入函数
	///////////////////////////////////////////////////////////////////////////
	void SetCommonParam(HttpRequest* request, bool isGet = true);
	void SetCommonParam(HttpRequest* request, int appId, bool isGet = true);
	void SetNewVersionCommonParam(HttpRequest* request, bool isGet = true);
	void SetCommonParam(HttpRequest* request, const char* scene, bool isGet = true);
	void SetParamToUrl(string& url, map<string, string>* mapReqParams);
	void SetGetFaceUrlCommonParam(HttpRequest* request, bool isGet = true);
	void SetCalculateCommonParam(HttpRequest* request, bool isGet = true);
	void SetAdvCommonParam(HttpRequest* request, bool isGet = true);
	void SetCommonParam(
		HttpRequest* request,
		const char* appId,
		const std::string& gunionToken,
		const std::string& gunionSignature,
		bool isGet = true); //Gunion-Wegame
	void SetCheckPayEntraceCommonParam(
		HttpRequest* request, 
		const char* appId, 
		int iSSandboxAccount,
		const std::string& gunionToken,
		const std::string& gunionSignature,
		bool isGet = true);

	std::string GenerateUniqueId();
	std::string GunionIntToString(int number);

	///////////////////////////////////////////////////////////////////////////
	// 私有基础字段：所有请求共有公参
	///////////////////////////////////////////////////////////////////////////
	string uniqueId;

	friend class LoginClient;

	//cas.sdo.com
	string m_hostName;
	int    m_port;
	string m_hostName2;
	int    m_port2;

	//mgame.sdo.com
	string m_hostName5;
	int    m_port5;
	string m_hostName6;
	int    m_port6;

	//gunion接口域名
	string m_hostName7;
	int    m_port7;
	string m_hostName8;
	int    m_port8;

	int m_timeout;
	int m_timeout2;

	// 初始化参数（来自 Initialize）
	int m_appid;
	int m_areaid;
	int m_groupId;
	int m_locale;
	int m_tag;
	int m_productId;
	int m_customSecurityLevel;

	string m_deviceId;
	string m_productVersion;

	// 新增：MAC 地址
	string m_macId;

	// 新增：IP 地址
	string m_IpId;

	// 中间变量（请求链路共用）
	string m_guid;
	string m_loginType;
	string m_tgt;
	string m_inputUserId;
	string m_codeKey;
	string m_autoLoginSessionKey;
	string m_autoLoginMaxAge;
	string m_checkCodeSessionKey;
	string m_pushMsgSessionKey;

	string m_promotionId;

	int thirdLoginExtern;

	string m_ueFlowId;

	map<string, string> m_mapReqParams;

	// 新版下行短信域名
	string m_hostName3;
	int    m_port3;
	string m_hostName4;
	int    m_port4;

	// 广告系统 traceId
	string adTraceId;

	// sdo 版本号
	string sdoVersion;

	// runtime id
	string sdoRunTimerId;
};

///////////////////////////////////////////////////////////////////////////////
// ResponseProcess —— 统一解析响应 JSON
//
// result：curl 返回值
// requestCode：请求类型
// response：服务器 JSON 字符串
// vecCookies：服务器返回的 Set-Cookie
// keyValues：解析后返回给 LoginClient 的 K-V
///////////////////////////////////////////////////////////////////////////////
class ResponseProcess
{
public:
	static int Process(LoginClient* loginClient, int result, int requestCode,
		const string& response, const vector<string>& vecCookies,
		map<string, string>& keyValues);

	static std::string ConvertUcs2ToAnsi(const std::string& ucs2);
};

