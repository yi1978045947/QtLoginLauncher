#ifndef SDO_BASE_CLIENT_H
#define SDO_BASE_CLIENT_H

#ifndef SDOAPI
#define SDOAPI _stdcall
#endif


#define ERROR_FORAT_URL		-10130001
#define ERROR_PROCESSING	-10130002
#define ERROR_REQUEST		-10130003
#define ERROR_RESPONSE		-10130004
#define  ERROR_HANDLE_EQ_NULL	-10130005
#define  ERROR_PARAM_INVALIDATE 	-10130006

#define  ERROR_WAIT_TIMEOUT 	-10130007
#define  ERROR_HTTP_AUTHEN 	-10130008

#define ERROR_REQUEST_FIRST	-10130100
#define ERROR_REQUEST_LAST	-10130200




#ifdef __cplusplus
extern "C" {
#endif

	typedef struct SdoBaseHandle SdoBaseHandle;

	int SDOAPI SdoBase_Initialize2(const char* serverAddr, const char* backupServerAddr,
		int appId, int areaId, int groupId, int locale, int tag, int productId, const char* productVersion,
		int customSecurityLevel, bool verifyCert, SdoBaseHandle** handle);

	int SDOAPI SdoBase_Release(SdoBaseHandle* handle);

	/**
	* 设置是否使用代理服务器（没有设置默认使用代理服务器）
	*/
	int SDOAPI SdoLsc_ProxyEnable(SdoBaseHandle* handle, int enable);

	/**
	* 设置超时时间（在调用任何接口之前都可以设置独立的超时时间）
	*/
	int SDOAPI SdoBase_SetTimeout(SdoBaseHandle* handle, int timeout, int secTimeout);

	/**
	* 当appid或者areaid发生变化是需要调用该接口传入变化之后的值
	*/
	int SDOAPI SdoBase_ModifyAppInfo(SdoBaseHandle* handle, int appId, int areaId, int groupId);

	/**
	* 获取当前登录的大票
	*/
	int SDOAPI SdoBase_GetSessionId(SdoBaseHandle* handle, char* sessioId);
	/**
	* 设置大票，可以用作登录状态恢复（比如重启之后恢复，或者传递大票到其他进程使用）
	*/
	int SDOAPI SdoBase_SetSessionId(SdoBaseHandle* handle, const char* sessioId);

	/**
	* 获取大票的有效性
	**/
	typedef void (SDOAPI* SdoBase_GetSessionIdStatesCallBack)(int resultCode, const char* failReason, const char* tgtArray, const char* loginStateArray, SdoBaseHandle* handle);
	int SDOAPI SdoBase_SetGetSessionIdStatesCallBack(SdoBaseHandle* handle, SdoBase_GetSessionIdStatesCallBack callback);
	int SDOAPI SdoBase_GetSessionIdStates(SdoBaseHandle* handle, const char* sessioId);

	/**
	* 获取动态密钥
	*/
	typedef void (SDOAPI* SdoBase_GetDynamicKeyCallback)(int resultCode, const char* failReason, const char* dynamicKey, const char* guid, SdoBaseHandle* handle);
	int SDOAPI SdoBase_SetGetDynamicKeyCallback(SdoBaseHandle* handle, SdoBase_GetDynamicKeyCallback callback);
	int SDOAPI SdoBase_GetDynamicKey(SdoBaseHandle* handle);

	/**
	* 登录过程中如果需要二次认证，比如需要做密宝，安全卡，验证那么会进入这个回调
	*/
	typedef void (SDOAPI* SdoBase_DynamicLoginCallback)(int resultCode, const char* failReason, int loginType, int deviceType,
		const char* deviceDisplayType, const char* challenge, const char* go_down_login_type, const char* dynamic_key,
		const char* inputUserId, SdoBaseHandle* handle);
	/**
	* 登录结果回调，如果不需要做二次认证，或者已经做了二次认证会进入这个回调
	*/
	//20241009增加自动登录票据参数keepLoginKey
	typedef void (SDOAPI* SdoBase_LoginResultCallback)(int resultCode, const char* failReason, const char* sndaId,
		const char* ticket, const char* accountUpgradeUrl, const char* mobile,
		const char* autoLoginSessionKey, int autoLoginMaxAge, int popWindowFlag,
		const char* redirectURL, const char* inputUserId, const char* bindMid,
		const char* noteName, const char* dispalyName, const char* companyId,
		bool isNew, const char* appMid, const char* tgt, const char* keepLoginKey,
		const char* flowid, const char* isScanned, SdoBaseHandle* handle);
	int SDOAPI SdoBase_SetDynamicLoginCallback(SdoBaseHandle* handle, SdoBase_DynamicLoginCallback callback);
	int SDOAPI SdoBase_SetLoginResultCallback(SdoBaseHandle* handle, SdoBase_LoginResultCallback callback);

	/*
	* 腾讯QQGame登陆
	* 回调SdoBase_LoginResultCallback
	*/
	int SDOAPI SdoBase_QQGameLogin(SdoBaseHandle* handle, const char* openid, const char* openkey, bool is_limited, int company_id);

	//qqGame票据延期
	typedef void (SDOAPI* SdoBase_QQGameIsLoginCallback)(int resultCode, const char* failReason);
	int SDOAPI SdoBase_SetQQGameIsLoginCallback(SdoBaseHandle* handle, SdoBase_QQGameIsLoginCallback callback);
	int SDOAPI SdoBase_QQGameIsLogin(SdoBaseHandle* handle, const char* openid, const char* openkey, int company_id);

	/*
	* 腾讯WeGame登陆
	* 回调SdoBase_LoginResultCallback
	*/
	int SDOAPI SdoBase_WeGameLogin(SdoBaseHandle* handle, const char* rail_id, const char* rail_session_ticket, bool is_limited);
	int SDOAPI SdoBase_WeGameLogin2(SdoBaseHandle* handle, const char* rail_id, const char* rail_session_ticket, bool is_limited, int company_id);


	/*
	* 三方登陆
	* 回调SdoBase_LoginResultCallback
	*/
	int SDOAPI SdoBase_ThirdLogin(SdoBaseHandle* handle, const char* third_token, const char* companyId, const char* scene = NULL, const char* phone = NULL,
		const char* smsCode = NULL, const char* extend = NULL, const char* szIsLimited = NULL);

	/*
	* 三方登陆，QQ、WX、WB
	* 回调SdoBase_LoginResultCallback
	*/
	int SDOAPI SdoBase_ForThirdLogin(SdoBaseHandle* handle, const char* third_token, const char* companyId, const char* scene = NULL, const char* phone = NULL,
		const char* smsCode = NULL, const char* extend = NULL, const char* szIsLimited = NULL);

	/*
	* 三方登录灵活获取皮肤展示设置
	* 回调SdoBase_CreateGetThirdLoginSkinCallback
	*/
	typedef void (SDOAPI* SdoBase_CreateThirdLoginSkinCallback)(int resultCode, const char* msg, const char* skin);
	int SDOAPI SdoBase_SetCreateThirdLoginSkinCallback(SdoBaseHandle* handle, SdoBase_CreateThirdLoginSkinCallback callback);
	int SDOAPI SdoBase_CreateThirdLoginSkin(SdoBaseHandle* handle);

	/**
	* 用户名密码登录
	* 如果需要二次认证则回调SdoBase_DynamicLoginCallback
	* 如果不需要二次认证则回调SdoBase_LoginResultCallback
	*/
	//2024/10/09 新版静密或者一键登录带图验登录接口中增加是否自动登录参数keepLoginFlag 1表示登录器本地自动登录，0表示未勾选自动登录
	int SDOAPI SdoBase_StaticLogin(SdoBaseHandle* handle, const char* inputUserId, const char* password,
		int accountDomain, int autoLoginFlag, int autoLoginKeepTime, int isSupportGeetest, int keepLoginFlag);
	/**
	* 用户名密码登录，这个接口使用明文的用户名密码
	* 如果需要二次认证则回调SdoBase_DynamicLoginCallback
	* 如果不需要二次认证则回调SdoBase_LoginResultCallback
	*/
	//2024/10/09 新版静密或者一键登录带图验登录接口中增加是否自动登录参数keepLoginFlag 1表示登录器本地自动登录，0表示未勾选自动登录
	int SDOAPI SdoBase_StaticLogin2(SdoBaseHandle* handle, const char* inputUserId, const char* password,
		int accountDomain, int autoLoginFlag, int autoLoginKeepTime, int isSupportGeetest, int keepLoginFlag);

	/**
	* 游戏账号登录
	* 当inputUserType，0=通行证（默认），1=游戏账号
	*/
	//2024/10/09 新版静密或者一键登录带图验登录接口中增加是否自动登录参数keepLoginFlag 1表示登录器本地自动登录，0表示未勾选自动登录
	int SDOAPI SdoBase_StaticLoginWithGameAccount(SdoBaseHandle* handle, const char* inputUserId, const char* password,
		int accountDomain, int autoLoginFlag, int autoLoginKeepTime, int isSupportGeetest, int inputUserType, int keepLoginFlag, const char* scene = NULL);

	/**
	* 图片验证码登陆
	*/
	//2024/10/09 新版静密或者一键登录带图验登录接口中增加是否自动登录参数keepLoginFlag 1表示登录器本地自动登录，0表示未勾选自动登录
	typedef void (SDOAPI* SdoBase_CheckCodeLoginCallback)(int resultCode, const char* failReason, const char* checkCodeUrl, int isGeetestCode, int width, int height, SdoBaseHandle* handle);
	int SDOAPI SdoBase_SetCheckCodeLoginCallback(SdoBaseHandle* handle, SdoBase_CheckCodeLoginCallback callback);
	int SDOAPI SdoBase_CheckCodeLogin(SdoBaseHandle* handle, const char* checkCode, const char* challenge, const char* validate, const char* seccode, const char* outinfo, int keepLoginFlag, const char* captchaInfo = NULL);

	/**
	* 密宝，安全卡登录
	* 回调函数SdoBase_LoginResultCallback
	*/
	//2024/10/09 新版动密登录接口中实名补填接口增加是否自动登录参数keepLoginFlag 1表示登录器本地自动登录，0表示未勾选自动登录
	int SDOAPI SdoBase_DynamicLogin(SdoBaseHandle* handle, const char* password, int keepLoginFlag);

	/**
	* 自动登录
	* 如果需要二次认证则回调SdoBase_DynamicLoginCallback
	* 如果不需要二次认证则回调SdoBase_LoginResultCallback
	*/
	int SDOAPI SdoBase_AutoLogin(SdoBaseHandle* handle, const char* autoLoginSessionKey);

	/**
	* 单点登录，用来获取网页的登录态
	* 回调函数SdoBase_LoginResultCallback
	*/
	int SDOAPI SdoBase_SsoLogin(SdoBaseHandle* handle);
	int SDOAPI SdoBase_SsoLogin2(SdoBaseHandle* handle, const char* tgt);
	int SDOAPI SdoBase_SsoLogin3(SdoBaseHandle* handle, const char* tgt, const char* scene);  // 为游戏盒子新增，增加scene，用于区分ssologin的来源
	int SDOAPI SdoBase_SsoLogin4(SdoBaseHandle* handle, const char* scene, int appId);         // 为游戏盒子新增，增加appId，用于区分商城与其他来源

	int SDOAPI SdoBase_GetAuthcode(SdoBaseHandle* handle, const char* tgt, const char* scene);  // 为游戏盒子新增，增加scene，用于区分ssologin的来源

	typedef void (SDOAPI* SdoBase_CreateAuthCodeTicketCallback)(int resultCode, const char* msg, const char* ticket, const char* tgt);
	int SDOAPI SdoBase_SetCreateAuthCodeTicketCallback(SdoBaseHandle* handle, SdoBase_CreateAuthCodeTicketCallback callback);
	int SDOAPI SdoBase_CheckAuthCode(SdoBaseHandle* handle, const char* authCode);                // 为游戏盒子新增，增加授权码验证


	//新增GetTicket接口用于获取最新的票据
	typedef void (SDOAPI* SdoBase_GetTicketCallback)(int resultCode, const char* failReason, const char* ticket);
	int SDOAPI SdoBase_SetGetTicketCallback(SdoBaseHandle* handle, SdoBase_GetTicketCallback callback);
	int SDOAPI SdoBase_GetTicket(SdoBaseHandle* handle);

	//WeGame用于用户进行支付初始化
	typedef void (SDOAPI* SdoBase_CreateWeGameOrderCallback)(int resultCode, const char* msg, const char* szMoney, const char* szItemName, const char* szPayOrderNo,
		const char* szPaymentUrl, const char* szOrderNo, const char* szWgOrderNo);
	int SDOAPI SdoBase_SetCreateWeGameOrderCallback(SdoBaseHandle* handle, SdoBase_CreateWeGameOrderCallback callback);
	int SDOAPI SdoBase_CreateWeGameOrder(SdoBaseHandle* handle, const char* ticket, const char* szOrderId, const char* szProductId, const char* szGroupId,
		const char* szAreaId, const char* szExtend, const char* szSign, const char* channel);

	/////////////////////////////////////////
	//传奇4用于用户进行支付初始化
	typedef void (SDOAPI* SdoBase_CreateCQ4OrderCallback)(int resultCode, const char* msg, const char* szOrderNo, const char* szMoney, const char* szItemName, const char* szPayOrderNo,
		const char* szPaymentUrl);
	int SDOAPI SdoBase_SetCreateCQ4OrderCallback(SdoBaseHandle* handle, SdoBase_CreateCQ4OrderCallback callback);
	int SDOAPI SdoBase_CreateCQ4Order(SdoBaseHandle* handle, const char* ticket, const char* szOrderId, const char* szProductId, const char* szGroupId,
		const char* szAreaId, const char* szExtend, const char* szSign, const char* channel, int iSSandboxAccount);

	//传奇4用于订单查询
	typedef void (SDOAPI* SdoBase_CreateCQ4QueryCallback)(int resultCode, const char* msg, const char* status);
	int SDOAPI SdoBase_SetCreateCQ4QueryCallback(SdoBaseHandle* handle, SdoBase_CreateCQ4QueryCallback callback);
	int SDOAPI SdoBase_CreateCQ4Query(SdoBaseHandle* handle, const char* ticket, const char* szOrderId, const char* szSign, const char* channel,
		int iSSandboxAccount);
	/////////////////////////////////////////

	//steam手游模式用于用户进行支付初始化
	typedef void (SDOAPI* SdoBase_CreateSteamChannelOrderCallback)(int resultCode, const char* msg, const char* szOrderNo);
	int SDOAPI SdoBase_SetCreateSteamChannelOrderCallback(SdoBaseHandle* handle, SdoBase_CreateSteamChannelOrderCallback callback);
	int SDOAPI SdoBase_CreateSteamChannelOrder(SdoBaseHandle* handle, const char* ticket, const char* szOrderId, const char* szProductId, const char* szGroupId,
		const char* szAreaId, const char* szExtend, const char* szSign, const char* channel);

	//QQGame用于用户进行支付初始化
	typedef void (SDOAPI* SdoBase_CreateQQGameOrderCallback)(int resultCode, const char* msg, const char* szPaymentUrl);
	int SDOAPI SdoBase_SetCreateQQGameOrderCallback(SdoBaseHandle* handle, SdoBase_CreateQQGameOrderCallback callback);
	int SDOAPI SdoBase_CreateQQGameOrder(SdoBaseHandle* handle, const char* ticket, const char* szOrderId, const char* szProductId, const char* szGroupId,
		const char* szAreaId, const char* szExtend, const char* szOpenId, const char* szOpenKey, const char* szPfKey, const char* szSign, const char* channel);

	//联想支付接口
	typedef void (SDOAPI* SdoBase_CreateLxOrderCallback)(int resultCode, const char* msg, const char* szMoney, const char* szItemName, const char* szPayOrderNo,
		const char* szPaymentUrl, const char* szOrderNo);
	int SDOAPI SdoBase_SetCreateLxOrderCallback(SdoBaseHandle* handle, SdoBase_CreateLxOrderCallback callback);
	int SDOAPI SdoBase_CreateLxOrder(SdoBaseHandle* handle, const char* ticket, const char* szOrderId, const char* szProductId, const char* szGroupId,
		const char* szAreaId, const char* szExtend, const char* szSign, const char* channel, const char* szLenovoTgt, const char* szRole);


	//控制WeGame退出事件开关
	typedef void (SDOAPI* SdoBase_WeGameStatusCallback)(int resultCode, const char* failReason, const char* szStatus);
	int SDOAPI SdoBase_SetWeGameStatusCallback(SdoBaseHandle* handle, SdoBase_WeGameStatusCallback callback);
	int SDOAPI SdoBase_WeGameStatus(SdoBaseHandle* handle);

	/**
	* 快速登录
	* 回调函数SdoBase_LoginResultCallback
	*/
	int SDOAPI SdoBase_FastLogin(SdoBaseHandle* handle);

	/**
	* 第三方账号信任登录（影子帐号）
	* 如果需要二次认证则回调SdoBase_DynamicLoginCallback
	* 如果不需要二次认证则回调SdoBase_LoginResultCallback
	*/
	int SDOAPI SdoLsc_RltLogin(SdoBaseHandle* handle, const char* vkey);

	/**
	* 账号检测，返回账号支持的登陆类型等信息
	*/
	typedef void (SDOAPI* SdoBase_CheckAccounTypeCallback)(int resultCode, const char* failReason, int type, int level, int existing,
		const char* mobileMask, int fromWoa, int hasPwdLoginRecord,
		int recommendLoginType, int hasCheckCodeLoginRecord, const char* ptMask, SdoBaseHandle* handle);
	int SDOAPI SdoBase_SetCheckAccountTypeCallback(SdoBaseHandle* handle, SdoBase_CheckAccounTypeCallback callback);
	int SDOAPI SdoBase_CheckAccoutType(SdoBaseHandle* handle, const char* inputUserId);

	/**
	* 向手机发送短信验证码
	*/
	typedef void (SDOAPI* SdoBase_SendPhoneCheckCodeCallback)(int resultCode, const char* failReason, SdoBaseHandle* handle);
	int SDOAPI SdoBase_SetSendPhoneCheckCodeCallback(SdoBaseHandle* handle, SdoBase_SendPhoneCheckCodeCallback callback);
	int SDOAPI SdoBase_SendPhoneCheckCode(SdoBaseHandle* handle, const char* inputUserId, int type);

	/**
	* 通过手机接收到的短信验证码登录
	* 如果需要二次认证则回调SdoBase_DynamicLoginCallback
	* 如果不需要二次认证则回调SdoBase_LoginResultCallback
	*/
	int SDOAPI SdoBase_PhoneCheckCodeLogin(SdoBaseHandle* handle, const char* checkCode);


	/**
	* 踢人时向手机发送短信验证码
	*/
	typedef void (SDOAPI* SdoBase_KickoffAccountVerifyCallback)(int resultCode, const char* failReason, SdoBaseHandle* handle);
	int SDOAPI SdoBase_SetKickoffAccountVerifyCallback(SdoBaseHandle* handle, SdoBase_KickoffAccountVerifyCallback callback);
	int SDOAPI SdoBase_KickoffAccountVerify(SdoBaseHandle* handle, const char* tgt);

	/**
	* 踢人时通过手机接收到的短信验证码登录
	*/
	typedef void (SDOAPI* SdoBase_KickoffAccountResultCallback)(int resultCode, const char* failReason, SdoBaseHandle* handle);
	int SDOAPI SdoBase_SetKickoffAccountResultCallback(SdoBaseHandle* handle, SdoBase_KickoffAccountResultCallback callback);
	int SDOAPI SdoBase_KickoffCheckCodeLogin(SdoBaseHandle* handle, const char* tgt, const char* checkCode, int nKickoffAppid, int nKickoffAreaid);

	/**
	* 踢人时图片验证码登陆
	*/
	int SDOAPI SdoBase_KickoffProtectCodeLogin(SdoBaseHandle* handle, const char* tgt, const char* protectCode);

	/**
	* 获取二维码
	*/
	typedef void (SDOAPI* SdoBase_GetQrCodeCallback)(int resultCode, const char* failReason, const char* picData, int picLength, SdoBaseHandle* handle);
	int SDOAPI SdoBase_SetGetQrCodeCallback(SdoBaseHandle* handle, SdoBase_GetQrCodeCallback callback);
	int SDOAPI SdoBase_GetQrCode(SdoBaseHandle* handle);

	/**
	* 检测二维码登录是否成功（循环调用该函数检测二维码登录是否成功）
	* 如果需要二次认证则回调SdoBase_DynamicLoginCallback
	* 如果不需要二次认证则回调SdoBase_LoginResultCallback
	*/
	//2024/10/09 新版扫码登录接口中沿用是否自动登录参数keepLoginFlag 1表示登录器本地自动登录，0表示未勾选自动登录
	int SDOAPI SdoBase_QrCodeLogin(SdoBaseHandle* handle, int autoLoginFlag, int autoLoginKeepTime, int keepLoginFlag);

	/**
	* 获取当前的登录状态
	*/
	typedef void (SDOAPI* SdoBase_GetLoginStateCallback)(int resultCode, const char* failReason, int loginState, SdoBaseHandle* handle);
	int SDOAPI SdoBase_SetGetLoginStateCallback(SdoBaseHandle* handle, SdoBase_GetLoginStateCallback callback);
	int SDOAPI SdoBase_GetLoginState(SdoBaseHandle* handle);
	int SDOAPI SdoBase_GetLoginState2(SdoBaseHandle* handle, const char* tgt);  // 为游戏盒子新增，外部传入tgt，用于校验自己管理的tgt


	/**
	* 延长登录态，登录态默认8小时失效，因此如果需要长时间保持登录态，必须定期调用该接口延长登录态
	*/
	typedef void (SDOAPI* SdoBase_ExtendLoginStateCallback)(int resultCode, const char* failReason, SdoBaseHandle* handle);
	int SDOAPI SdoBase_SetExtendLoginStateCallback(SdoBaseHandle* handle, SdoBase_ExtendLoginStateCallback callback);
	int SDOAPI SdoBase_ExtendLoginState(SdoBaseHandle* handle);
	int SDOAPI SdoBase_ExtendLoginState2(SdoBaseHandle* handle, const char* tgt);


	/**
	* 注销
	*/
	typedef void (SDOAPI* SdoBase_LogoutCallback)(int resultCode, const char* failReason, SdoBaseHandle* handle);
	int SDOAPI SdoBase_SetLogoutCallback(SdoBaseHandle* handle, SdoBase_LogoutCallback callback);
	int SDOAPI SdoBase_Logout(SdoBaseHandle* handle);

	/**
	* 检测账号支持消息确认登录状态
	*/
	typedef void (SDOAPI* SdoBase_GetPushMessageStatusCallback)(int resultCode, const char* failReason, int appInstallStatus,
		int appVersionStatus, int appOnlineStatus,
		int pushMessageSwitchStatus, int blackListStatus, SdoBaseHandle* handle);
	int SDOAPI SdoBase_SetGetPushMessageStatusCallback(SdoBaseHandle* handle, SdoBase_GetPushMessageStatusCallback callback);
	int SDOAPI SdoBase_GetPushMessageStatus(SdoBaseHandle* handle, const char* inputUserId);

	/**
	* 发起消息确认登录
	*/
	typedef void (SDOAPI* SdoBase_SendPushMessageCallback)(int resultCode, const char* failReason, const char* pushMsgSerialNum, SdoBaseHandle* handle);
	int SDOAPI SdoBase_SetSendPushMessageCallback(SdoBaseHandle* handle, SdoBase_SendPushMessageCallback callback);
	int SDOAPI SdoBase_SendPushMessage(SdoBaseHandle* handle, const char* inputUserId, const char* scene = NULL);


	/**
	* 取消一键登录消息
	* 回调函数SdoBase_CancelPushMessageCallback
	*/
	typedef void (SDOAPI* SdoBase_CancelPushMessageCallback)(int resultCode, const char* failReason, SdoBaseHandle* handle);
	int SDOAPI SdoBase_SetCancelPushMessageCallBack(SdoBaseHandle* handle, SdoBase_CancelPushMessageCallback callback);
	int SDOAPI SdoBase_CancelPushMessageLogin(SdoBaseHandle* handle);

	/**
	* 检测消息确认登录是否成功（循环调用该函数检测消息确认登录是否成功）
	* 如果需要二次认证则回调SdoBase_DynamicLoginCallback
	* 如果不需要二次认证则回调SdoBase_LoginResultCallback
	*/
	//2024/10/09 新版一键登录接口中沿用是否自动登录参数keepLoginFlag 1表示登录器本地自动登录，0表示未勾选自动登录
	int SDOAPI SdoBase_PushMessageLogin(SdoBaseHandle* handle, int autoLoginFlag, int autoLoginKeepTime, int keepLoginFlag);

	/**
	* 实名补填
	*/
	//2024/10/09 新版登录接口中实名补填接口增加是否自动登录参数keepLoginFlag 1表示登录器本地自动登录，0表示未勾选自动登录
	typedef void (SDOAPI* SdoBase_FcmLoginCallback)(int resultCode, const char* failReason, bool isNew, SdoBaseHandle* handle);
	int SDOAPI SdoBase_SetFcmLoginCallback(SdoBaseHandle* handle, SdoBase_FcmLoginCallback callback);
	int SDOAPI SdoBase_FcmLogin(SdoBaseHandle* handle, const char* realName, const char* idCard, const char* email, int keepLoginFlag);

	/**
	* 获取账号信息
	* appInstallStatus 盛大通行证手机版安装状态 1 已安装 2 未安装
	* bindPhoneStatus	是否绑定手机 1.已绑定 2.未绑定
	*/
	typedef void (SDOAPI* SdoBase_GetAccountInfoCallback)(int resultCode, const char* failReason, int appInstallStatus,
		int bindPhoneStatus, const char* companyId, const char* mid, SdoBaseHandle* handle);
	int SDOAPI SdoBase_SetGetAccountInfoCallback(SdoBaseHandle* handle, SdoBase_GetAccountInfoCallback callback);
	int SDOAPI SdoBase_GetAccountInfo(SdoBaseHandle* handle, const char* tgt);

	/**
	* 查询账号登陆历史
	* endpointIp	登录IP
	* appName		应用名称
	* requestTime	访问时间
	*/
	typedef void (SDOAPI* SdoBase_GetLoginHistoryCallback)(int resultCode, const char* failReason, const char* endpointIp,
		const char* appName, const char* requestTime, const char* ipLocation, SdoBaseHandle* handle);
	int SDOAPI SdoBase_SetGetLoginHistoryCallback(SdoBaseHandle* handle, SdoBase_GetLoginHistoryCallback callback);
	int SDOAPI SdoBase_GetLoginHistory(SdoBaseHandle* handle, int queryNumber);
	int SDOAPI SdoBase_GetLoginHistory2(SdoBaseHandle* handle, const char* tgt, int queryNumber);

	/**
	* 查询登录用户信息
	* sndaId			数字账号
	* displayAccount	显示登录
	* inputUserId		输入账号
	*/
	typedef void (SDOAPI* SdoBase_GetLoginUserInfoCallback)(int resultCode, const char* failReason, const char* sndaId,
		const char* displayAccount, const char* inputUserId, const char* notename, SdoBaseHandle* handle);
	int SDOAPI SdoBase_SetGetLoginUserInfoCallback(SdoBaseHandle* handle, SdoBase_GetLoginUserInfoCallback callback);
	int SDOAPI SdoBase_GetLoginUserInfo(SdoBaseHandle* handle, const char* tgt);

	typedef void (SDOAPI* SdoBase_SetLoginUserInfoCallback)(int resultCode, const char* failReason, SdoBaseHandle* handle);
	int SDOAPI SdoBase_SetSetLoginUserInfoCallback(SdoBaseHandle* handle, SdoBase_SetLoginUserInfoCallback callback);
	int SDOAPI SdoBase_SetLoginUserInfo(SdoBaseHandle* handle, const char* tgt, const char* notename);

	/**
	* 兼容老接口
	*/
	int SDOAPI SdoBase_Initialize(const char* serverAddr, const char* backupServerAddr,
		int appId, int areaId, int groupId, int locale, int tag, int productId, const char* productVersion,
		int customSecurityLevel, bool verifyCert,
		SdoBase_CheckCodeLoginCallback checkCodeLoginCallback,
		SdoBase_DynamicLoginCallback dynamicLoginCallback,
		SdoBase_FcmLoginCallback fcmLoginCallback,
		SdoBase_LoginResultCallback loginResultCallback,
		SdoBase_GetDynamicKeyCallback getDynamicKeyCallback,
		SdoBase_SendPhoneCheckCodeCallback sendPhoneCheckCodeCallback,
		SdoBase_CheckAccounTypeCallback checkAccountCallback,
		SdoBase_GetQrCodeCallback getCodeKeyCallback,
		SdoBase_GetLoginStateCallback getLoginStatusCallback,
		SdoBase_ExtendLoginStateCallback extendLoginStateCallback,
		SdoBase_LogoutCallback logoutCallback,
		SdoBase_SendPushMessageCallback sendPushMessageCallback,
		SdoBaseHandle** handle,
		const char* gunionServerAddr, const char* gunionBackupServerAddr);

	/**
	* 新建SdoBase_Initialize3，增加客户端类型参数thirdLoginExtern：0官方，1联想，2顺网。。。
	*/
	int SDOAPI SdoBase_Initialize3(const char* serverAddr, const char* backupServerAddr,
		int appId, int areaId, int groupId, int locale, int tag, int productId, const char* productVersion,
		int customSecurityLevel, bool verifyCert,
		SdoBase_CheckCodeLoginCallback checkCodeLoginCallback,
		SdoBase_DynamicLoginCallback dynamicLoginCallback,
		SdoBase_FcmLoginCallback fcmLoginCallback,
		SdoBase_LoginResultCallback loginResultCallback,
		SdoBase_GetDynamicKeyCallback getDynamicKeyCallback,
		SdoBase_SendPhoneCheckCodeCallback sendPhoneCheckCodeCallback,
		SdoBase_CheckAccounTypeCallback checkAccountCallback,
		SdoBase_GetQrCodeCallback getCodeKeyCallback,
		SdoBase_GetLoginStateCallback getLoginStatusCallback,
		SdoBase_ExtendLoginStateCallback extendLoginStateCallback,
		SdoBase_LogoutCallback logoutCallback,
		SdoBase_SendPushMessageCallback sendPushMessageCallback,
		SdoBaseHandle** handle,
		int thirdLoginExtern,
		const char* channelId,
		const char* gunionServerAddr,
		const char* gunionBackupServerAddr,
		bool enableGunionPreInit);

	/**
	/* 通过keyVal，获取内部信息
	*/
	int SdoBase_GetValue(SdoBaseHandle* handle, const char* keyVal, char* val);

	/**
	/* 通过keyVal，设置内部信息
	*/
	int SdoBase_SetValue(SdoBaseHandle* handle, const char* keyVal, const char* val);

	/**
	* 内部接口，不能调用
	*/
	int SdoBase_Internel_SetCallBack(SdoBaseHandle* handle, const char* funcName, const void* func);

	/**
	* 设置回调接口，
	* funcType回调的变量类型
	* funcName 由用户声明的回调函数名
	* 用例：
	* SdoBaseHandle* a;
	* typedef void (SDOAPI* b)();
	* void SDOAPI c();
	* SdoBase_SetCallBack（a, b, c);
	*/
#define  SdoBase_SetCallBack(handle, funcType, funcName) {funcType ft = funcName; SdoBase_Internel_SetCallBack(handle, #funcType, funcName);}

	/**
	/* 等待连接返回，用于同步调用
	*/
	int SdoBase_WaitResponse(SdoBaseHandle* handle, int timeout);

	/**
	/* 终止未完成操作
	*/
	int SDOAPI SdoBase_Cancel(SdoBaseHandle* handle);

	/**
	/* 获取推广信息
	*/
	typedef void (SDOAPI* SdoBase_GetPromotionInfoCallback)(int resultCode, const char* failReason, const char* promotionUrl, SdoBaseHandle* handle);
	int SdoBase_GetPromotionInfo(SdoBaseHandle* handle);

	/**
	 * 获取 UserData
	 */
	void* SdoBase_GetUserData(SdoBaseHandle* handle);

	/**
		设置UserData
	*/
	int SdoBase_SetUserData(SdoBaseHandle* handle, const void* userData);

	/**
	/* 获取最近一次网络请求，返回的内容中，字段名为keyVal对应的值
	*/
	int SdoBase_GetParam(SdoBaseHandle* handle, const char* keyVal, char* val);

	/**
	/* 设置之后，最近一次网络请求的参数，keyVal和val，分别为字段名和键值
	*/
	int SdoBase_SetParam(SdoBaseHandle* handle, const char* keyVal, const char* val);

	/**
	* 下次不再提示推广信息接口,
	* days:不再提示推广信息的天数，-1表示一直不再提示
	*/
	typedef void (SDOAPI* SdoBase_PromotionInfoConfirmCallback)(int resultCode, const char* failReason, SdoBaseHandle* handle);
	int SdoBase_PromotionInfoConfirm(SdoBaseHandle* handle, int days);

	/**
	* 取小票,
	*/
	typedef void (SDOAPI* SdoBase_GetClientVKeyCallback)(int resultCode, const char* failReason, const char* vkey, SdoBaseHandle* handle);
	int SdoBase_GetClientVKey(SdoBaseHandle* handle);
	int SdoBase_GetClientVKey2(SdoBaseHandle* handle, const char* tgt);

	/**
	* 发送用户账号,防木马
	*/
	typedef void (SDOAPI* SdoBase_SendUserAccountCallback)(int resultCode, const char* failReason, SdoBaseHandle* handle);
	int SdoBase_SendUserAccount(SdoBaseHandle* handle, const char* inputUserId);

	/**
	* 获取账号群内的账号
	*/
	typedef void (SDOAPI* SdoBase_GetAccountGroupCallback)(int resultCode, const char* failReason, const char* sndaId, const char* ptMask, const char* ptAccount, const char* bindMid, const char* noteName, const char* companyId, SdoBaseHandle* handle);
	int SDOAPI SdoBase_GetAccountGroup(SdoBaseHandle* handle, const char* tgt);

	/**
	* 群账号登录
	*/
	int SDOAPI SdoBase_AccountGroupLogin(SdoBaseHandle* handle, const char* tgt, const char* sndaId, int autoLoginFlag, int autoLoginKeepTime);

	/**
	* 第三方账号轮询登录
	*/
	int SDOAPI SdoBase_ThirdPartyPollingLogin(SdoBaseHandle* handle, const char* companyId, int autoLoginFlag, int autoLoginKeepTime);

	/**
	* 第三方账号登录
	*/
	int SDOAPI SdoBase_ThirdPartyLogin(SdoBaseHandle* handle, const char* companyId, const char* token, int autoLoginFlag, int autoLoginKeepTime, const char* scene = NULL, const char* phone = NULL, const char* smsCode = NULL);

	/**
	* 获取应用区列表
	*/

	typedef void (SDOAPI* SdoBase_GetLoginAreaInfoCallback)(int resultCode, const char* failReason, const char* areaCode, const char* areaName, const char* areaGroupCode, const char* areaGroupName,
		SdoBaseHandle* handle);
	int SDOAPI SdoBase_SetGetLoginAreaInfoCallback(SdoBaseHandle* handle, SdoBase_GetLoginAreaInfoCallback callback);
	int SDOAPI SdoBase_GetLoginAreaInfo(SdoBaseHandle* handle, int nAppId);

	/**
	* 云游戏登录
	* 优化：添加场景参数 20210408
	*/
	int SDOAPI SdoBase_CloudGameLogin(SdoBaseHandle* handle, const char* tgt, const char* scene = NULL);

	/**
	* 咪咕短信发送
	*/
	typedef void (SDOAPI* SdoBase_SendMiGuSmsCallback)(int resultCode, const char* failReason, SdoBaseHandle* handle);
	int SDOAPI SdoBase_SendMiGuSms(SdoBaseHandle* handle, const char* phone);

	/**
	* 短信发送
	*/
	typedef void (SDOAPI* SdoBase_SendSmsCallback)(int resultCode, const char* failReason, const char* smsSessionKey, const char* checkCodeUrl, int needCheckCode, int width, int height, SdoBaseHandle* handle);
	int SDOAPI SdoBase_SendSms(SdoBaseHandle* handle, const char* smsSessionKey, const char* phone, int smsType, const char* scene = NULL);

	/**
	* 验证Captcha后发送短信
	*/
	typedef void (SDOAPI* SdoBase_CheckCodeToSendSmsCallback)(int resultCode, const char* failReason, const char* smsSessionKey, const char* checkCodeUrl, int needCheckCode, int width, int height, SdoBaseHandle* handle);
	int SDOAPI SdoBase_CheckCodeToSendSms(SdoBaseHandle* handle, const char* smsSessionKey, const char* checkCode, const char* captchaInfo = NULL);

	/**
	* 短信登录
	*/
	//2024/10/09 新版短信一键注册登录接口中沿用是否自动登录参数keepLoginFlag 1表示登录器本地自动登录，0表示未勾选自动登录
	int SDOAPI SdoBase_SmsLogin(SdoBaseHandle* handle, const char* smsSessionKey, const char* smsCode, int autoLoginFlag, int autoLoginKeepTime, int keepLoginFlag);

	/**
	* 用户隐私协议配置
	*/
	typedef void (SDOAPI* SdoBase_UserPrivacyConfigCallback)(int resultCode, const char* failReason, const char* privacyPolicyUrl, int privacyPolicyVersion, const char* servicerAgreementUrl, int serviceAgreementVersion);
	int SDOAPI SdoBase_SetUserPrivacyConfigCallback(SdoBaseHandle* handle, SdoBase_UserPrivacyConfigCallback callback);
	int SdoBase_UserPrivacyConfig(SdoBaseHandle* handle, const char* scene, int privacypolicyversion, int serviceAgreementVersion);

	/**
	* 新版用户隐私协议配置
	*/
	typedef void (SDOAPI* SdoBase_NewVersionUserPrivacyConfigCallback)(int resultCode, const char* failReason, const char* privacyPolicyUrl, int privacyPolicyVersion, const char* servicerAgreementUrl, int serviceAgreementVersion);
	int SDOAPI SdoBase_SetNewVersionUserPrivacyConfigCallback(SdoBaseHandle* handle, SdoBase_NewVersionUserPrivacyConfigCallback callback);
	int SDOAPI SdoBase_NewVersionUserPrivacyConfig(SdoBaseHandle* handle, const char* scene, int privacypolicyversion);

	/**
	* 新建SdoBase_ModitfyThirdLoginExtern，修改客户端类型参数thirdLoginExtern：0官方，1联想，2顺网。。。
	*/
	int SDOAPI SdoBase_ModitfyThirdLoginExtern(SdoBaseHandle* handle, int thirdLoginExtern, bool verifyCert);

	/**
	* 人脸识别功能初始化接口
	*/
	typedef void (SDOAPI* SdoBase_FaceVerifyInitCallback)(int resultCode, const char* resultMsg, int openFace, const char* imageData, const char* contextId, const char* phone, int showSkip);
	int SDOAPI SdoBase_SetFaceVerifyInitCallback(SdoBaseHandle* handle, SdoBase_FaceVerifyInitCallback callback);
	int SDOAPI SdoBase_FaceVerifyInit(SdoBaseHandle* handle, const char* scene);

	/**
	* 人脸识别功能获取扫码结果接口
	*/
	typedef void (SDOAPI* SdoBase_GetFaceCodeResultCallback)(int resultCode, const char* resultMsg, const char* nextAction, const char* promptMsg, const char* ticket);
	int SDOAPI SdoBase_SetGetFaceCodeResultCallback(SdoBaseHandle* handle, SdoBase_GetFaceCodeResultCallback callback);
	int SDOAPI SdoBase_GetFaceCodeResult(SdoBaseHandle* handle, const char* contextId);

	/**
	* 人脸识别功能上报用户行为接口
	*/
	typedef void (SDOAPI* SdoBase_SendActionCallback)(int resultCode, const char* resultMsg, const char* ticket);
	int SDOAPI SdoBase_SetSendActionCallback(SdoBaseHandle* handle, SdoBase_SendActionCallback callback);
	int SDOAPI SdoBase_SendAction(SdoBaseHandle* handle, const char* contextId, int action);

	/**
	* 打开组件日志
	*/
	int SDOAPI SdoBase_InitLog(const char* logFileName, const char* mode);


	/*************************************** 卫士通加密服务接口*************************************************/
	/**
	* 卫士通加密初始化接口
	*/
	typedef void (SDOAPI* SdoBase_UeInitClientCallback)(int resultCode, const char* resultMsg, int operationModel,
		const char* key, const char* license, const char* passportIdAuth, const char* westoneAppId);
	int SDOAPI SdoBase_SetUeInitClientCallback(SdoBaseHandle* handle, SdoBase_UeInitClientCallback callback);
	int SDOAPI SdoBase_UeInitClient(SdoBaseHandle* handle, const char* hash);

	/**
	* 数据上报
	*/
	int SDOAPI SdoBase_UeReport(SdoBaseHandle* handle, const char* szExtendData);


	/*************************************** steam端游模式支付通知 *************************************************/
	int SDOAPI SdoBase_SendSteamPayResult(SdoBaseHandle* handle, const char* szSteamId, const char* szOrderId);

	/*************************************** steam手游模式支付通知 *************************************************/
	int SDOAPI SdoBase_SendSteamChannelPayResult(SdoBaseHandle* handle, const char* szChannel, const char* szTicket, const char* szOrderId);


	/**
	* 获取登录器功能相关配置
	*/
	typedef void (SDOAPI* SdoBase_GetClientConfigCallback)(int resultCode, const char* failReason, const char* config);
	int SDOAPI SdoBase_SetGetClientConfigCallback(SdoBaseHandle* handle, SdoBase_GetClientConfigCallback callback);
	int SDOAPI SdoBase_GetClientConfig(SdoBaseHandle* handle);

	/**
	* 下行短信相关配置初始化
	*/
	typedef void (SDOAPI* SdoBase_GoDownConfigInitCallback)(int resultCode, const char* failReason, const char* nextAction,
		const char* captchaParams, const char* safePhoneTip);
	int SDOAPI SdoBase_SetGoDownConfigInitCallback(SdoBaseHandle* handle, SdoBase_GoDownConfigInitCallback callback);
	int SDOAPI SdoBase_GoDownConfigInit(SdoBaseHandle* handle, const char* account, const char* isVoice, const char* flowid);

	/**
	* 下行短信带图验发送
	*/
	typedef void (SDOAPI* SdoBase_GoDownConfigSendSmsCheckCodeCallback)(int resultCode, const char* failReason, const char* nextAction,
		const char* captchaParams, const char* safePhoneTip);
	int SDOAPI SdoBase_SetGoDownSendSmsCheckCodeCallback(SdoBaseHandle* handle, SdoBase_GoDownConfigSendSmsCheckCodeCallback callback);
	int SDOAPI SdoBase_GoDownSendSmsCheckCode(SdoBaseHandle* handle, const char* captchaInfo);

	/**
	* 确认发送下行短信
	*/
	typedef void (SDOAPI* SdoBase_GoDownconfirmSendSmsCallback)(int resultCode, const char* failReason, const char* nextAction);
	int SDOAPI SdoBase_SetGoDownconfirmSendSmsCallback(SdoBaseHandle* handle, SdoBase_GoDownconfirmSendSmsCallback callback);
	int SDOAPI SdoBase_GoDownGoDownconfirmSendSms(SdoBaseHandle* handle, int isVoice);

	/**
	* 确认下行短信登录
	*/
	//2024/10/09 新版下行登录接口中沿用是否自动登录参数keepLoginFlag 1表示登录器本地自动登录，0表示未勾选自动登录
	typedef void (SDOAPI* SdoBase_GoDownconfirmLoginCallback)(int resultCode, const char* failReason, const char* nextAction,
		bool isNew, const char* sndaId, const char* tgt,
		const char* ticket, const char* autoLoginSessionKey);
	int SDOAPI SdoBase_SetGoDownconfirmLoginCallback(SdoBaseHandle* handle, SdoBase_GoDownconfirmLoginCallback callback);
	int SDOAPI SdoBase_GoDownGoDownconfirmLogin(SdoBaseHandle* handle, const char* account, const char* verifyCode, int keepLoginFlag);

	/**
	*  新增快速登录
	*/
	typedef void (SDOAPI* SdoBase_FastLoginLoginCallback)(int resultCode, const char* failReason, const char* nextAction,
		bool isNew, const char* sndaId, const char* tgt,
		const char* ticket, const char* autoLoginSessionKey);
	int SDOAPI SdoBase_SetFastLoginLoginCallback(SdoBaseHandle* handle, SdoBase_FastLoginLoginCallback callback);
	int SDOAPI SdoBase_FastLoginConfirmLogin(SdoBaseHandle* handle, const char* autoLoginKey);

	/**
	*  新增getticket获取票据
	*/
	typedef void (SDOAPI* SdoBase_NewVersionGetTicketCallback)(int resultCode, const char* failReason, const char* ticket);
	int SDOAPI SdoBase_SetGetTicketLoginCallback(SdoBaseHandle* handle, SdoBase_NewVersionGetTicketCallback callback);
	int SDOAPI SdoBase_GetTicketNew(SdoBaseHandle* handle);

	/**
	*  //其他登录情况绑定安全手机,例如静密等
	*/
	typedef void (SDOAPI* SdoBase_OtherLoginSecurityPhoneCallback)(int resultCode, const char* failReason, SdoBaseHandle* handle);
	int SDOAPI SdoBase_SetOtherLoginSecurityPhoneCallback(SdoBaseHandle* handle, SdoBase_OtherLoginSecurityPhoneCallback callback);

	/**
	*  新增快速登录主动删除账号
	*/
	typedef void (SDOAPI* SdoBase_FastLoginActiveDeleteAccountLoginCallback)(int resultCode, const char* failReason);
	int SDOAPI SdoBase_SetFastLoginActiveDeleteAccountLoginCallback(SdoBaseHandle* handle, SdoBase_FastLoginActiveDeleteAccountLoginCallback callback);
	int SDOAPI SdoBase_FastLoginActiveDeleteAccount(SdoBaseHandle* handle, const char* keepLoginKey, const char* inputUserId);

	/**
	*  判断快速登录列表中账号类型
	*/
	typedef void (SDOAPI* SdoBase_CheckAccountTypeLoginCallback)(int resultCode, const char* failReason, int acctType);
	int SDOAPI SdoBase_SetCheckAccountTypeLoginCallback(SdoBaseHandle* handle, SdoBase_CheckAccountTypeLoginCallback callback);
	int SDOAPI SdoBase_CheckAccountLoginType(SdoBaseHandle* handle, const char* tgt);

	/**
	*  查询指定日期的星座运势情况，配置返回在sdologin内部处理json串
	*/
	typedef void (SDOAPI* SdoBase_CalculateCallback)(int resultCode, const char* failReason, const char* config);
	int SDOAPI SdoBase_SetCalculateCallback(SdoBaseHandle* handle, SdoBase_CalculateCallback callback);
	int SDOAPI SdoBase_CheckCalculate(SdoBaseHandle* handle, const char* star_sign, const char* date);

	/**
	*  广告系统初始化接口获取traceId
	*/
	typedef void (SDOAPI* SdoBase_InitAdvCallback)(int resultCode, const char* failReason, const char* traceId);
	int SDOAPI SdoBase_SetInitAdvCallback(SdoBaseHandle* handle, SdoBase_InitAdvCallback callback);
	int SDOAPI SdoBase_CheckInitAdv(SdoBaseHandle* handle);

	/**
	*  增加公共参数traceId，这个参数是关于登录的广告系统接入的参数
	*/
	int SDOAPI SdoBase_SetTraceId(SdoBaseHandle* handle, const char* traceId);

	/*
	清除扫码登录已经存在的codeKey
	*/
	int SDOAPI SdoBase_ClearCodeKey(SdoBaseHandle* handle);

	/**
	* 监护人信息补填短信发送
	*/
	typedef void (SDOAPI* SdoBase_GuardDianSendSmsCallback)(int resultCode, const char* failReason, const char* flowId,
		const char* nextAction, const char* captchaParams, int captchaType);
	int SDOAPI SdoBase_SetGuardDianSendSmsCallback(SdoBaseHandle* handle, SdoBase_GuardDianSendSmsCallback callback);
	int SDOAPI SdoBase_GuardDianSendSms(SdoBaseHandle* handle, const char* flowId, const char* phone, int isVoice);

	/**
	*  监护人信息补填带图验短信发送
	*/
	typedef void (SDOAPI* SdoBase_GuardDianSendSmsCheckCodeCallback)(int resultCode, const char* failReason, const char* flowId,
		const char* nextAction, const char* captchaParams, int captchaType);
	int SDOAPI SdoBase_SetGuardDianSendSmsCheckCodeCallback(SdoBaseHandle* handle, SdoBase_GuardDianSendSmsCheckCodeCallback callback);
	int SDOAPI SdoBase_GuardDianSendSmsCheckCode(SdoBaseHandle* handle, const char* flowId, const char* captchaInfo, int isVoice);

	/**
	* 监护人信息补填短信确认
	*/
	typedef void (SDOAPI* SdoBase_GuardDianConfrimSendSmsResultCallback)(int resultCode, const char* failReason, int needCheckAge,
		const char* checkAgeRemindTip, const char* nextAction,
		const char* tgt, const char* ticket, const char* sndaId,
		const char* inputUserId, int isNew, const char* keepLoginKey);
	int SDOAPI SdoBase_SetGuardDianConfrimSendSmsResultCallback(SdoBaseHandle* handle, SdoBase_GuardDianConfrimSendSmsResultCallback callback);
	int SDOAPI SdoBase_GuardDianConfrimSendSmsResult(SdoBaseHandle* handle, int ageCheckFlag, const char* flowId, const char* verifyCode,
		const char* tutorName, const char* tutorIdCard, const char* phone);

	/**
	* 监护人信息回调
	*/
	typedef void (SDOAPI* SdoBaseGuardDianCallback)(int resultCode, const char* failReason, const char* flowId, SdoBaseHandle* handle);
	int SDOAPI SdoBase_SetGuardDianCallback(SdoBaseHandle* handle, SdoBaseGuardDianCallback callback);

	/**
	* 设置公共参数sdoVersion:每次更新一个功能都加1默认1.0.0.0
	*/
	int SDOAPI SdoBase_SetSdoVersion(SdoBaseHandle* handle, const char* sdoVersion);

	/**
	* 注册即登录/添加自动登录参数，不影响老街口所以新开一个接口
	* 如果不需要二次认证则回调SdoBase_LoginResultCallback
	*/
	int SDOAPI SdoLsc_RltLoginKeepLogin(SdoBaseHandle* handle, const char* vkey, int keepLoginFlag);

	/**
	* 设置公共参数runTimeId，打点id串联登录器和服务端
	*/
	int SDOAPI SdoBase_SetRunTimerId(SdoBaseHandle* handle, const char* runTimeId);

	/**
	 * 下载图片（用于验证码）
	 * 返回值：0 = 成功，非0 = 失败
	 */
	int SDOAPI SdoBase_DownloadImageA(
		SdoBaseHandle* handle,
		const char* url,
		unsigned char** outBuf,
		int* outSize);

	/**
	* 获取人脸识别页面URL/账号持有人登记页面URL
	*/
	typedef void (SDOAPI* SdoBaseSetFaceRecognitionUrlCallback)(
		int resultCode, 
		const char* failReason, 
		const char* faceRecognitionUr);
	int SDOAPI SdoBase_SetFaceRecognitionUrlCallback(
		SdoBaseHandle* handle, 
		SdoBaseSetFaceRecognitionUrlCallback callback);
	int SdoBase_FaceRecognitionUrlResult(
		SdoBaseHandle* handle, 
		const char* confType, 
		const char* confKey);

	/**
	* 查询人脸识别票据状态,验证成功或者失败
	*/
	typedef void (SDOAPI* SdoBaseSetQueryFaceVerifyTicketStatusCallback)(
		int resultCode, 
		const char* failReason, 
		bool verifyResult, 
		const char* resultMsg, 
		bool needPopup);
	int SDOAPI SdoBase_SetQueryFaceVerifyTicketStatusCallback(
		SdoBaseHandle* handle, 
		SdoBaseSetQueryFaceVerifyTicketStatusCallback callback);
	int SDOAPI SdoBase_QueryFaceVerifyTicketStatusResult(
		SdoBaseHandle* handle, 
		const char* appId, 
		const char* ticket);

	/**
	* 获取人脸识别页面URL/账号持有人登记页面URL
	*/
	typedef void (SDOAPI* SdoBaseSetFaceHolderRegistrationUrlCallback)(
		int resultCode, 
		const char* failReason, 
		const char* holderRegistrationUrl);
	int SDOAPI SdoBase_SetFaceHolderRegistrationUrlCallback(
		SdoBaseHandle* handle, 
		SdoBaseSetFaceHolderRegistrationUrlCallback callback);
	int SDOAPI SdoBase_FaceHolderRegistrationUrlResult(
		SdoBaseHandle* handle,
		const char* confType, 
		const char* confKey);

	/**
	* 激活码登录
	*/
	typedef void (SDOAPI* SdoBaseActiveCodeLoginCallback)(
		int resultCode, const char* failReason, const char* nextAction,
		bool isNew, const char* sndaId, const char* tgt,
		const char* ticket, const char* autoLoginSessionKey);
	int SDOAPI SdoBase_SetActiveCodeLoginCallback(
		SdoBaseHandle* handle,
		SdoBaseActiveCodeLoginCallback callback);
	int SDOAPI SdoBase_GetActiveCodeLogin(
		SdoBaseHandle* handle,
		const char* activeCode);

	/**
	* 需要展示激活界面
	*/
	typedef void (SDOAPI* SdoBaseShowActiveCodeLoginDlgCallback)(
		int resultCode, const char* failReason, const char* tgt);
	int SDOAPI SdoBase_SetShowActiveCodeLoginDlgCallback(
		SdoBaseHandle* handle,
		SdoBaseShowActiveCodeLoginDlgCallback callback);

	/**
	*  GunionWegame 初始化
	*/
	typedef void (SDOAPI* SdoBase_CreateGunionWegameInitCallback)(
		int resultCode,
		const char* msg,
		const char* weixin_appId,
		const char* weixin_key,
		const char* qq_appId,
		const char* qq_key,
		const char* weibo_appKey,
		const char* weibo_redirectUrl,
		const char* voicetip_one,
		const char* voicetip_two,
		const char* voicetip_button,
		const char* game_name,
		const char* brand_logo,
		const char* brand_name,
		const char* is_match,
		const char* new_device_id_server,
		const char* login_button,
		const char* login_icon,
		const char* official_auth_func,
		int accountDeletionPeriod,
		const char* sms_login_prompt,
		const char* static_login_prompt,
		const char* token,
		const char* randKey);

	int SDOAPI SdoBase_SetCreatGunionWegameInitCallback(
		SdoBaseHandle* handle,
		SdoBase_CreateGunionWegameInitCallback callback);

	int SDOAPI SdoBase_GunionWegameInit(
		SdoBaseHandle* handle,
		const char* appId);

	/*
	* 腾讯 WeGame 登录（Gunion）
	* 回调 SdoBase_CreateGunionWegameLoginCallback
	*/
	typedef void (SDOAPI* SdoBase_CreateGunionWegameLoginCallback)(
		int resultCode,
		const char* failReason,
		const char* userid,
		const char* ticket,
		bool isNewUser,
		int showBindPhone,
		int canSkipBindPhone,
		int skipBindPhonePeriod,
		int canChangePhone,
		const char* bindPhone,
		const char* bindPhoneSndaId,
		SdoBaseHandle* handle);

	int SDOAPI SdoBase_SetCreatGunionWegameLoginCallback(
		SdoBaseHandle* handle,
		SdoBase_CreateGunionWegameLoginCallback callback);

	int SDOAPI SdoBase_GunionWeGameLogin(
		SdoBaseHandle* handle,
		const char* rail_id,
		const char* rail_session_ticket,
		bool is_limited,
		int company_id);

	/*
	* GunionWegame 短信发送
	* 回调 SdoBase_CreateGunionWegameSmsSendCallback
	*/
	typedef void (SDOAPI* SdoBase_CreateGunionWegameSmsSendCallback)(
		int resultCode,
		const char* msg,
		const char* checkCodeUrl,
		int isGeetTestCode,
		int width,
		int height,
		SdoBaseHandle* handle);

	int SDOAPI SdoBase_SetCreatGunionWegameSmsSendCallback(
		SdoBaseHandle* handle,
		SdoBase_CreateGunionWegameSmsSendCallback callback);

	int SDOAPI SdoBase_GunionWegameSmsSend(
		SdoBaseHandle* handle,
		const char* phone,
		const char* voice_msg,
		const char* str_new_deviceid_server);

	/*
	* GunionWegame 带图验短信发送
	* 回调 SdoBase_CreateGunionWegamePicSmsSendCallback
	*/
	typedef void (SDOAPI* SdoBase_CreateGunionWegamePicSmsSendCallback)(
		int resultCode,
		const char* msg,
		const char* checkCodeUrl,
		int isGeetTestCode,
		int width,
		int height,
		SdoBaseHandle* handle);

	int SDOAPI SdoBase_SetCreatGunionWegamePicSmsSendCallback(
		SdoBaseHandle* handle,
		SdoBase_CreateGunionWegamePicSmsSendCallback callback);

	int SDOAPI SdoBase_GunionWegamePicSmsSend(
		SdoBaseHandle* handle,
		const char* imagecodeType,
		const char* voice_msg,
		const char* captchaInfo,
		const char* str_new_deviceid_server);

	/*
	* GunionWegame 三方账号绑定手机号
	* 回调 SdoBase_CreateGunionWegameThirdAccountBindPhoneCallback
	*/
	typedef void (SDOAPI* SdoBase_CreateGunionWegameThirdAccountBindPhoneCallback)(
		int resultCode,
		const char* msg,
		const char* bindPhone,
		const char* bindPhoneSndaId,
		const char* thirdNickName,
		const char* realInfoFlowId);

	int SDOAPI SdoBase_SetCreatGunionWegameThirdAccountBindPhoneCallback(
		SdoBaseHandle* handle,
		SdoBase_CreateGunionWegameThirdAccountBindPhoneCallback callback);

	int SDOAPI SdoBase_GunionWegameThirdAccountBindPhone(
		SdoBaseHandle* handle,
		const char* phone,
		const char* sms_code,
		const char* str_new_deviceid_server);

	/*
	* GunionWegame 实名
	* 回调 SdoBase_CreateGunionWegameFillRealinfoCallback
	*/
	typedef void (SDOAPI* SdoBase_CreateGunionWegameFillRealinfoCallback)(
		int resultCode,
		const char* msg);

	int SDOAPI SdoBase_SetCreatGunionWegameFillRealinfoCallback(
		SdoBaseHandle* handle,
		SdoBase_CreateGunionWegameFillRealinfoCallback callback);

	int SDOAPI SdoBase_GunionWegameFillRealinfo(
		SdoBaseHandle* handle,
		const char* idcard,
		const char* name,
		const char* realInfoFlowId,
		const char* str_new_deviceid_server);

	/*
	* GunionWegame 获取票据
	* 回调 SdoBase_CreateGunionWegameGetTicketCallback
	*/
	typedef void (SDOAPI* SdoBase_CreateGunionWegameGetTicketCallback)(
		int resultCode,
		const char* msg,
		const char* ticket);

	int SDOAPI SdoBase_SetCreatGunionWegameGetTicketCallback(
		SdoBaseHandle* handle,
		SdoBase_CreateGunionWegameGetTicketCallback callback);

	int SDOAPI SdoBase_GunionWegameGetTicket(
		SdoBaseHandle* handle,
		int time_s,
		const char* str_new_deviceid_server);

	/*
	* GunionWegame 获取支付票据
	* 回调 SdoBase_CreateGunionWegameGetPayTicketCallback
	*/
	typedef void (SDOAPI* SdoBase_CreateGunionWegameGetPayTicketCallback)(
		int resultCode,
		const char* msg,
		const char* ticket);

	int SDOAPI SdoBase_SetCreatGunionWegameGetPayTicketCallback(
		SdoBaseHandle* handle,
		SdoBase_CreateGunionWegameGetPayTicketCallback callback);

	int SDOAPI SdoBase_GunionWegameGetPayTicket(
		SdoBaseHandle* handle,
		int time_s,
		const char* str_new_deviceid_server);

	/*
	* 重新写入 token
	*/
	int SDOAPI SdoBase_GunionWegameSetToken(
		SdoBaseHandle* handle,
		const char* token);

	/*
	* 重新写入 randKey
	*/
	int SDOAPI SdoBase_GunionWegameSetRandKey(
		SdoBaseHandle* handle,
		const char* randKey);

	/*************************************** GHOME_PC 统一扩展接口 *************************************************/

	//GHOME_PC

	/**
	 * 获取隐私协议跳转URL
	 */
	typedef void (SDOAPI* SdoBase_CreateDoPrivateCallback)(
		int resultCode,
		const char* msg,
		const char* url);

	int SDOAPI SdoBase_SetCreatDoPrivateCallback(
		SdoBaseHandle* handle,
		SdoBase_CreateDoPrivateCallback callback);

	int SDOAPI SdoBase_DoPrivate(
		SdoBaseHandle* handle,
		const char* appId,
		const char* scene);

	//GHOME 短信登录接口

	/**
	 * GHOME_PC offical账号短信发送
	 */
	typedef void (SDOAPI* SdoBase_CreateDoSmsSendCallback)(int resultCode,
		const char* msg,
		const char* checkCodeUrl,
		int isGeetTestCode,
		int width,
		int height,
		SdoBaseHandle* handle);
	int SDOAPI SdoBase_SetCreatDoSmsSendCallback(
		SdoBaseHandle* handle, 
		SdoBase_CreateDoSmsSendCallback callback);
	int SDOAPI SdoBase_DoSmsSend(
		SdoBaseHandle* handle, 
		const char* phone, 
		const char* voiceMsg,
		const char* smsType,
		const char* str_new_deviceid_server);

	/**
	 * GHOME_PC offical账号短信带图验发送
	 */
	typedef void (SDOAPI* SdoBase_CreateCheckPicSmsSendCallback)(int resultCode,
		const char* msg,
		const char* checkCodeUrl,
		int isGeetTestCode,
		int width,
		int height,
		SdoBaseHandle* handle);
	int SDOAPI SdoBase_SetCreatCheckPicSmsSendCallback(
		SdoBaseHandle* handle, 
		SdoBase_CreateCheckPicSmsSendCallback callback);
	int SDOAPI SdoBase_CheckPicSmsSend(
		SdoBaseHandle* handle, 
		const char* imagecodeType, 
		const char* voiceMsg,
		const char* captchaInfo,
		const char* smsType,
		const char* str_new_deviceid_server);

	/**
	 * GHOME_PC offical账号短信登录
	 */
	typedef void (SDOAPI* SdoBase_CreateCheckSmsLoginCallback)(
		int resultCode,
		const char* msg,
		const char* userid,
		const char* ticket,
		const char* autokey,
		int has_realInfo,
		int isNewUser,
		const char* realMessage,
		int activation,
		int isAdult,
		int age,
		int needTutor,
		const char* tutorNotification,
		const char* selectLoginContext,
		const char* selectAccountList,
		const char* token,
		const char* randKey);

	int SDOAPI SdoBase_SetCreatCheckSmsLoginCallback(
		SdoBaseHandle* handle,
		SdoBase_CreateCheckSmsLoginCallback callback);

	int SDOAPI SdoBase_CheckSmsLogin(
		SdoBaseHandle* handle,
		const char* phone,
		const char* sms_code,
		const char* str_new_deviceid_server);

	//GHOME_PC实名

	/**
	 * GHOME_PC 实名认证
	 */
	typedef void (SDOAPI* SdoBase_CreateCheckRealNameCallback)(
		int resultCode,
		const char* msg,
		int isAdult,
		int age,
		int needTutor,
		const char* tutorNotification,
		int blockLogin,
		const char* blockNotification);

	int SDOAPI SdoBase_SetCreatCheckRealNameCallback(
		SdoBaseHandle* handle,
		SdoBase_CreateCheckRealNameCallback callback);

	int SDOAPI SdoBase_CheckRealName(
		SdoBaseHandle* handle,
		const char* idcard,
		const char* name,
		const char* str_new_deviceid_server);

	/**
	 * GHOME_PC 自动登录
	 */
	typedef void (SDOAPI* SdoBase_CreateCheckAutoLoginCallback)(
		int resultCode,
		const char* msg,
		const char* userid,
		const char* ticket,
		const char* autokey,
		int has_realInfo,
		int isNewUser,
		const char* realMessage,
		int activation,
		int isAdult,
		int age,
		int needTutor,
		const char* tutorNotification,
		const char* token,
		const char* randKey);

	int SDOAPI SdoBase_SetCreatCheckAutoLoginCallback(
		SdoBaseHandle* handle,
		SdoBase_CreateCheckAutoLoginCallback callback);

	int SDOAPI SdoBase_CheckAutoLogin(
		SdoBaseHandle* handle,
		const char* autokey,
		const char* str_new_deviceid_server);

	/**
	 * GHOME_PC 登出
	 */
	typedef void (SDOAPI* SdoBase_CreateCheckLogoutCallback)(
		int resultCode,
		const char* msg,
		bool flag);

	int SDOAPI SdoBase_SetCreatCheckLogoutCallback(
		SdoBaseHandle* handle,
		SdoBase_CreateCheckLogoutCallback callback);

	int SDOAPI SdoBase_CheckLogout(
		SdoBaseHandle* handle,
		const char* autokey,
		const char* str_new_deviceid_server);

	/**
	 * GHOME_PC 静密登录
	 */
	typedef void (SDOAPI* SdoBase_CreateDoPwdLoginCallback)(
		int resultCode,
		const char* msg,
		const char* userid,
		const char* ticket,
		const char* autokey,
		int has_realInfo,
		int isNewUser,
		const char* realMessage,
		int activation,
		int isAdult,
		int age,
		int needTutor,
		const char* tutorNotification,
		const char* selectLoginContext,
		const char* selectAccountList,
		const char* token,
		const char* randKey);

	int SDOAPI SdoBase_SetCreatDoPwdLoginCallback(
		SdoBaseHandle* handle,
		SdoBase_CreateDoPwdLoginCallback callback);

	int SDOAPI SdoBase_DoPwdLogin(
		SdoBaseHandle* handle,
		const char* phone,
		const char* password,
		const char* str_new_deviceid_server);

	/**
	 * GHOME_PC 静密图验登录（图验校验）
	 */
	typedef void (SDOAPI* SdoBase_CreateCheckPicPwdLoginCallback)(
		int resultCode,
		const char* msg,
		const char* userid,
		const char* ticket,
		const char* autokey,
		int has_realInfo,
		int isNewUser,
		const char* checkCodeUrl,
		int isGeetTestCode,
		int width,
		int height,
		const char* realMessage,
		int activation,
		int isAdult,
		int age,
		int needTutor,
		const char* tutorNotification,
		const char* selectLoginContext,
		const char* selectAccountList,
		const char* token,
		const char* randKey);

	int SDOAPI SdoBase_SetCreatCheckPicPwdLoginCallback(
		SdoBaseHandle* handle,
		SdoBase_CreateCheckPicPwdLoginCallback callback);

	int SDOAPI SdoBase_CheckPicPwdLogin(
		SdoBaseHandle* handle,
		const char* imagecodeType,
		const char* captchaInfo,
		const char* str_new_deviceid_server);

	/**
	 * GHOME_PC 三方登录校验（QQ/WX/WB 等）
	 */
	typedef void (SDOAPI* SdoBase_CreateCheckThirdLoginCallback)(
		int resultCode,
		const char* msg,
		const char* userid,
		const char* ticket,
		const char* autokey,
		int has_realInfo,
		int isNewUser,
		int activation,
		int isAdult,
		int age,
		int needTutor,
		const char* tutorNotification);

	int SDOAPI SdoBase_SetCreatCheckThirdLoginCallback(
		SdoBaseHandle* handle,
		SdoBase_CreateCheckThirdLoginCallback callback);

	int SDOAPI SdoBase_CheckThirdLogin(
		SdoBaseHandle* handle,
		const char* company_id,
		const char* third_ticket,
		const char* code_verify,
		const char* openid,
		const char* uid,
		const char* str_new_deviceid_server);

	//个性账号静密登录

	/**
	 * 个性账号 静密登录
	 */
	typedef void (SDOAPI* SdoBase_CreateDoSpecialPwdLoginCallback)(
		int resultCode,
		const char* msg,
		const char* userid,
		const char* ticket,
		const char* autokey,
		int has_realInfo,
		int isNewUser,
		const char* smsSendGuid,
		const char* lastSafePhone,
		int isPt,
		const char* realMessage,
		int activation,
		int isAdult,
		int age,
		int needTutor,
		const char* tutorNotification,
		const char* token,
		const char* randKey);

	int SDOAPI SdoBase_SetCreatDoSpecialPwdLoginCallback(
		SdoBaseHandle* handle,
		SdoBase_CreateDoSpecialPwdLoginCallback callback);

	int SDOAPI SdoBase_DoSpecialPwdLogin(
		SdoBaseHandle* handle,
		const char* account,
		const char* password,
		const char* str_new_deviceid_server);

	//个性账号静密带图验登录

	/**
	 * 个性账号 静密图验登录
	 */
	typedef void (SDOAPI* SdoBase_CreateCheckSpecialPicPwdLoginCallback)(
		int resultCode,
		const char* msg,
		const char* userid,
		const char* ticket,
		const char* autokey,
		int has_realInfo,
		int isNewUser,
		const char* checkCodeUrl,
		int isGeetTestCode,
		int width,
		int height,
		const char* smsSendGuid,
		const char* lastSafePhone,
		int isPt,
		const char* realMessage,
		int activation,
		int isAdult,
		int age,
		int needTutor,
		const char* tutorNotification,
		const char* token,
		const char* randKey);

	int SDOAPI SdoBase_SetCreatCheckSpecialPicPwdLoginCallback(
		SdoBaseHandle* handle,
		SdoBase_CreateCheckSpecialPicPwdLoginCallback callback);

	int SDOAPI SdoBase_CheckSpecialPicPwdLogin(
		SdoBaseHandle* handle,
		const char* imagecodeType,
		const char* captchaInfo,
		const char* str_new_deviceid_server);

	/**
	 * 用户登录区服记录校验（写入/校验区服信息）
	 */
	typedef void (SDOAPI* SdoBase_CreateCheckLoginAreaCallback)(
		int resultCode,
		const char* msg,
		const char* userid);

	int SDOAPI SdoBase_SetCreatCheckLoginAreaCallback(
		SdoBaseHandle* handle,
		SdoBase_CreateCheckLoginAreaCallback callback);

	int SDOAPI SdoBase_CheckLoginAreaLogin(
		SdoBaseHandle* handle,
		const char* userid,
		const char* area,
		const char* group,
		const char* str_new_deviceid_server);

	/**
	 * 获取区服配置信息
	 */
	typedef void (SDOAPI* SdoBase_CreateCheckGetLoginAreaCallback)(
		int resultCode,
		const char* msg,
		const char* json_msg);

	int SDOAPI SdoBase_SetCreatCheckGetLoginAreaCallback(
		SdoBaseHandle* handle,
		SdoBase_CreateCheckGetLoginAreaCallback callback);

	int SDOAPI SdoBase_CheckGetLoginAreaLogin(
		SdoBaseHandle* handle,
		const char* str_new_deviceid_server);

	//个性账号短信发送

	/**
	 * 个性账号 短信发送（获取验证码）
	 */
	typedef void (SDOAPI* SdoBase_CreateDoSpecialSmsSendCallback)(
		int resultCode,
		const char* msg,
		const char* checkCodeUrl,
		int isGeetTestCode,
		int width,
		int height,
		const char* smsSendGuid,
		const char* lastSafePhone,
		SdoBaseHandle* handle);

	int SDOAPI SdoBase_SetCreatDoSpecialSmsSendCallback(
		SdoBaseHandle* handle,
		SdoBase_CreateDoSpecialSmsSendCallback callback);

	int SDOAPI SdoBase_DoSpecialSmsSend(
		SdoBaseHandle* handle,
		const char* account,
		const char* voice_msg,
		const char* str_new_deviceid_server);

	//个性账号带图验验证码发送

	/**
	 * 个性账号 图验后短信发送
	 */
	typedef void (SDOAPI* SdoBase_CreateCheckSpecialPicSmsSendCallback)(
		int resultCode,
		const char* msg,
		const char* checkCodeUrl,
		int isGeetTestCode,
		int width,
		int height,
		const char* smsSendGuid,
		const char* lastSafePhone,
		SdoBaseHandle* handle);

	int SDOAPI SdoBase_SetCreatCheckSpecialPicSmsSendCallback(
		SdoBaseHandle* handle,
		SdoBase_CreateCheckSpecialPicSmsSendCallback callback);

	int SDOAPI SdoBase_CheckSpecialPicSmsSend(
		SdoBaseHandle* handle,
		const char* imagecodeType,
		const char* captchaInfo,
		const char* str_new_deviceid_server);

	//个性账号安全手机确认发送

	/**
	 * 个性账号 安全手机确认发送短信验证码
	 */
	typedef void (SDOAPI* SdoBase_CreateDoSpecialConfirmSmsSendCallback)(
		int resultCode,
		const char* msg);

	int SDOAPI SdoBase_SetCreatDoSpecialConfirmSmsSendCallback(
		SdoBaseHandle* handle,
		SdoBase_CreateDoSpecialConfirmSmsSendCallback callback);

	int SDOAPI SdoBase_DoSpecialConfirmSmsSend(
		SdoBaseHandle* handle,
		const char* str_new_deviceid_server);

	//个性账号短信登录

	/**
	 * 个性账号 短信登录
	 */
	typedef void (SDOAPI* SdoBase_CreateDoSpecialCheckSmsLoginCallback)(
		int resultCode,
		const char* msg,
		const char* userid,
		const char* ticket,
		const char* autokey,
		int has_realInfo,
		int isNewUser,
		int pt,
		const char* realMessage,
		int activation,
		int isAdult,
		int age,
		int needTutor,
		const char* tutorNotification,
		const char* token,
		const char* randKey);

	int SDOAPI SdoBase_SetCreatDoSpecialCheckSmsLoginCallback(
		SdoBaseHandle* handle,
		SdoBase_CreateDoSpecialCheckSmsLoginCallback callback);

	int SDOAPI SdoBase_DoSpecialCheckSmsLogin(
		SdoBaseHandle* handle,
		const char* account,
		const char* sms_code,
		const char* str_new_deviceid_server);

	//统一收银台支付

	/**
	 * 统一收银台 支付创建（返回支付链接）
	 */
	typedef void (SDOAPI* SdoBase_CreatePayEntranceCallback)(
		int resultCode,
		const char* msg,
		const char* orderid,
		const char* payorderid,
		const char* paymenturl);

	int SDOAPI SdoBase_SetCreatePayEntranceCallback(
		SdoBaseHandle* handle,
		SdoBase_CreatePayEntranceCallback callback);

	int SDOAPI SdoBase_DoCreatePayEntrance(
		SdoBaseHandle* handle,
		const char* szOrderId,
		const char* szProductId,
		const char* szGroupId,
		const char* szAreaId,
		const char* szExtend,
		int iSSandboxAccount,
		const char* str_new_deviceid_server);


	//统一收银台支付订单查询

	/**
	 * 统一收银台 支付订单查询
	 */
	typedef void (SDOAPI* SdoBase_CreateCheckPayOrderStatus)(
		int resultCode,
		const char* msg,
		const char* status);

	int SDOAPI SdoBase_SetCreateCheckPayOrderStatusCallback(
		SdoBaseHandle* handle,
		SdoBase_CreateCheckPayOrderStatus callback);

	int SDOAPI SdoBase_DoCreateCheckPayOrderStatus(
		SdoBaseHandle* handle,
		const char* szOrderId,
		int iSSandboxAccount,
		const char* str_new_deviceid_server);

	//激活码

	/**
	 * 激活码 校验
	 */
	typedef void (SDOAPI* SdoBase_CreateCheckActivation)(
		int resultCode,
		const char* msg);

	int SDOAPI SdoBase_SetCreateCheckActivationCallback(
		SdoBaseHandle* handle,
		SdoBase_CreateCheckActivation callback);

	int SDOAPI SdoBase_DoCreateCheckActivation(
		SdoBaseHandle* handle,
		const char* activation,
		const char* str_new_deviceid_server);

	/**
	 *  获取游戏隐私协议相关内容
	 */
	typedef void (SDOAPI* SdoBase_CreateCheckGameAgreementUrlContentCallback)(
		int resultCode,                // 结果码
		const char* failReason,        // 错误或提示信息
		const char* serviceUrl,        // 服务协议 URL
		const char* serviceContent,    // 服务协议 HTML 内容
		const char* privacyUrl,        // 隐私协议 URL
		const char* privacyContent     // 隐私协议 HTML 内容
		);

	int SDOAPI SdoBase_SetCreatGetAgreementUrlContentCallback(
		SdoBaseHandle* handle,
		SdoBase_CreateCheckGameAgreementUrlContentCallback callback);

	int SDOAPI SdoBase_CheckGameAgreementUrlContent(
		SdoBaseHandle* handle,
		const char* appId,
		const char* scene);

	/**
	 * 监护人信息收集/校验
	 */
	typedef void (SDOAPI* SdoBase_CreateCheckSetTutorInfoCallback)(
		int resultCode,
		const char* msg,
		int blockLogin,
		const char* blockNotification,
		int needConfirm,
		const char* confirmKey,
		const char* needConfirmNotification);

	int SDOAPI SdoBase_SetCreatSetTutorInfoCallback(
		SdoBaseHandle* handle,
		SdoBase_CreateCheckSetTutorInfoCallback callback);

	int SDOAPI SdoBase_CheckSetTutorInfo(
		SdoBaseHandle* handle,
		const char* idcard,
		const char* name,
		const char* phone,
		const char* smscode,
		const char* confirmKey,
		const char* str_new_deviceid_server);

	/**
	 * Wegame账号登录（校验/确认）
	 */
	typedef void (SDOAPI* SdoBase_CreateGhomeGunionWegameLoginCallback)(
		int resultCode,
		const char* msg,
		const char* userid,
		const char* ticket,
		const char* autokey,
		int has_realInfo,
		int isNewUser,
		const char* realMessage,
		int activation);

	int SDOAPI SdoBase_SetCreatGhomeGunionWegameLoginCallback(
		SdoBaseHandle* handle,
		SdoBase_CreateGhomeGunionWegameLoginCallback callback);

	int SDOAPI SdoBase_CheckGhomeGunionWegameLogin(
		SdoBaseHandle* handle,
		const char* selectLoginContext,
		const char* companyId,
		const char* str_new_deviceid_server);

	/**
	 * 获取公钥GetPublicKey
	 */
	typedef void (SDOAPI* SdoBase_CreateGunionGetPublicKeyCallback)(
		int resultCode,
		const char* msg,
		const char* gunionPublicKey);

	int SDOAPI SdoBase_SetCreatGunionGetPublicKeyCallback(
		SdoBaseHandle* handle,
		SdoBase_CreateGunionGetPublicKeyCallback callback);

	int SDOAPI SdoBase_CheckGunionGetPublicKey(
		SdoBaseHandle* handle);

	/**
	 *  握手handshake
	 */
	typedef void (SDOAPI* SdoBase_CreateGunionHandShakeCallback)(
		int resultCode,
		const char* msg,
		const char* gunionToken);

	int SDOAPI SdoBase_SetCreatGunionHandShakeCallback(
		SdoBaseHandle* handle,
		SdoBase_CreateGunionHandShakeCallback callback);

	int SDOAPI SdoBase_CheckGunionHandShake(
		SdoBaseHandle* handle,
		const char* publicKey);
	//GHOME_PC 隐私协议-初始化-短信发送-短信图验-短信登录-实名认证-自动登录-登出-获取ticket-静密登录-静密图验-三方登录(QQ,WX,WB,WEGAME)
	//个性账号静密登录-个性账号带图验静密登录-用户登录区服记录-获取区服配置信息-个性账号短信发送-个性账号校验图片验证码发送短信-个性账号短信登录
	//个性账号安全手机确认发送短信验证码-个性账号短信登录
	//统一收银台支付
	//统一收银台支付订单查询
	//激活码
	//获取相关协议
	//获取公钥
	//握手
#ifdef __cplusplus
}
#endif

#endif // SDO_BASE_CLIENT_H
