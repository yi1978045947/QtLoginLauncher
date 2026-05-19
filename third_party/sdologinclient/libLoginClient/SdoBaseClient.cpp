#include "StdAfx.h"
#include "SdoBaseClient.h"
#include "LoginClient.h"
#include "Tracer.h"
#include "CurlHttpDownload.h"

/**
 * @brief 三方登录功能的外部扩展标识（进程级静态变量定义）
 *
 * 业务说明：
 *   - 用于标识三方登录的入口来源或业务场景（如 QQ / 微信 / 微博 / Lenovo / 顺网 等）
 *   - 该值由业务层在登录前设置，LoginClient 仅做透传
 *   - 服务端可根据该标识区分登录来源、应用策略、风控等级等
 *
 * 生命周期：
 *   - 进程级（static）
 *   - 在程序启动时初始化，默认值为 0
 *   - 在进程生命周期内保持有效，直到被显式修改
 *
 * 注意事项：
 *   - 该变量不与单个 LoginClient 实例绑定
 *   - 多 LoginClient 共用同一份扩展标识
 *   - 修改该值会影响后续所有三方登录请求
 */
int LoginClient::thirdLoginExtern = 0;

/**
 * @brief Gunion-WeGame 三方登录扩展标识（进程级静态变量定义）
 *
 * 业务说明：
 *   - 用于 Gunion-WeGame 登录体系下的第三方登录扩展参数
 *   - 该值通常由业务层在登录前设置，用于区分不同 WeGame 登录入口或场景
 *   - LoginClient 仅负责透传该参数，不参与业务判断
 *
 * 与 thirdLoginExtern 的区别：
 *   - thirdLoginExtern                ：通用三方登录（QQ / WX / WB / Lenovo 等）
 *   - gunionWegameThirdLoginExtern    ：Gunion-WeGame 专用登录体系
 *   - 二者分离设计，避免不同登录体系参数混用
 *
 * 生命周期：
 *   - 进程级（static）
 *   - 在程序启动时初始化为空字符串
 *   - 在进程生命周期内保持有效，直到被显式修改
 *
 * 注意事项：
 *   - 该变量不与单个 LoginClient 实例绑定
 *   - 多个 LoginClient 实例共享同一份 Gunion-WeGame 扩展标识
 *   - 修改该值会影响后续所有 Gunion-WeGame 相关请求
 */
std::string LoginClient::gunionWegameThirdLoginExtern;

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
std::string LoginClient::channelIdExtern;

extern "C" int SDOAPI SdoBase_Initialize(const char* serverAddr, const char* backupServerAddr,
	int appId, int areaId, int groupId, int locale, int tag,
	int productId, const char* productVersion, int customSecurityLevel, bool verifyCert,
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
	SdoBaseHandle * *handle,
	const char* gunionServerAddr, const char* gunionBackupServerAddr)
{
	LoginClient* app = new LoginClient(verifyCert);
	*handle = (SdoBaseHandle*)app;
	return app->Initialize(serverAddr, backupServerAddr, appId, areaId, groupId, locale, tag,
		productId, productVersion, customSecurityLevel,
		checkCodeLoginCallback, dynamicLoginCallback, fcmLoginCallback,
		loginResultCallback, getDynamicKeyCallback, sendPhoneCheckCodeCallback,
		checkAccountCallback, getCodeKeyCallback, getLoginStatusCallback,
		extendLoginStateCallback, logoutCallback, sendPushMessageCallback,true, gunionServerAddr, gunionBackupServerAddr);
}

extern "C" int SDOAPI SdoBase_Initialize3(const char* serverAddr, const char* backupServerAddr,
	int appId, int areaId, int groupId, int locale, int tag,
	int productId, const char* productVersion, int customSecurityLevel, bool verifyCert,
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
	SdoBaseHandle * *handle,
	int thirdLoginExtern,
	const char* channelId,
	const char* gunionServerAddr,
	const char* gunionBackupServerAddr,
	bool enableGunionPreInit
)
{
	LoginClient* app = new LoginClient(verifyCert);
	*handle = (SdoBaseHandle*)app;
	LoginClient::thirdLoginExtern = thirdLoginExtern;
	LoginClient::channelIdExtern = channelId;
	return app->Initialize(serverAddr, backupServerAddr, appId, areaId, groupId, locale, tag,
		productId, productVersion, customSecurityLevel,
		checkCodeLoginCallback, dynamicLoginCallback, fcmLoginCallback,
		loginResultCallback, getDynamicKeyCallback, sendPhoneCheckCodeCallback,
		checkAccountCallback, getCodeKeyCallback, getLoginStatusCallback,
		extendLoginStateCallback, logoutCallback, sendPushMessageCallback, 
		enableGunionPreInit, gunionServerAddr, gunionBackupServerAddr);
}

extern "C" int SdoBase_SetValue(SdoBaseHandle * handle, const char* keyVal, const char* val)
{
	LoginClient* app = (LoginClient*)handle;

	if (app == NULL)
	{
		return ERROR_HANDLE_EQ_NULL;
	}

	return app->SetValue(keyVal, val);
}

extern "C" int SdoBase_GetValue(SdoBaseHandle * handle, const char* keyVal, char* val)
{
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL)
	{
		return ERROR_HANDLE_EQ_NULL;
	}
	return app->GetValue(keyVal, val);
}

extern "C" int SdoBase_SetUserData(SdoBaseHandle * handle, const void* userData)
{
	LoginClient* app = (LoginClient*)handle;

	if (app == NULL)
	{
		return ERROR_HANDLE_EQ_NULL;
	}

	return app->SetUserData(userData);
}

extern "C" void* SdoBase_GetUserData(SdoBaseHandle * handle)
{
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL)
	{
		return NULL;
	}
	return app->GetUserData();
}

extern "C" int SdoBase_Internel_SetCallBack(SdoBaseHandle * handle, const char* funcName, const void* func)
{
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL)
	{
		return ERROR_HANDLE_EQ_NULL;
	}
	return app->SetCallBack(funcName, func);
}

extern "C" int SdoBase_WaitResponse(SdoBaseHandle * handle, int timeout)
{
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL)
	{
		return ERROR_HANDLE_EQ_NULL;
	}
	return app->WaitResponse(timeout);
}

extern "C" int SDOAPI SdoBase_Cancel(SdoBaseHandle * handle)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL)
	{
		return ERROR_HANDLE_EQ_NULL;
	}
	return app->Cancel();
}

extern "C" int SdoBase_GetPromotionInfo(SdoBaseHandle * handle)
{
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL)
	{
		return ERROR_HANDLE_EQ_NULL;
	}
	return app->GetPromotionInfo();
}

extern "C" int SDOAPI SdoBase_Initialize2(const char* serverAddr, const char* backupServerAddr,
	int appId, int areaId, int groupId, int locale, int tag, int productId, const char* productVersion,
	int customSecurityLevel, bool verifyCert, SdoBaseHandle * *handle)
{
	LoginClient* app = new LoginClient(verifyCert);
	*handle = (SdoBaseHandle*)app;
	return app->Initialize2(serverAddr, backupServerAddr, appId, areaId, groupId, locale, tag, productId, productVersion, customSecurityLevel);
}

extern "C" int SDOAPI SdoBase_Release(SdoBaseHandle * handle)
{
	LoginClient* app = (LoginClient*)handle;
	delete app;
	return 0;
}

extern "C" int SDOAPI SdoBase_SetTimeout(SdoBaseHandle * handle, int timeout, int secTimeout)
{
	LoginClient* app = (LoginClient*)handle;
	return app->SetTimeout(timeout, secTimeout);
}

extern "C" int SDOAPI SdoBase_ModifyAppInfo(SdoBaseHandle * handle, int appId, int areaId, int groupId)
{
	LoginClient* app = (LoginClient*)handle;
	return app->ModifyAppInfo(appId, areaId, groupId);
}

extern "C" int SDOAPI SdoBase_GetDynamicKey(SdoBaseHandle * handle)
{
	LoginClient* app = (LoginClient*)handle;
	return app->GetDynamicKey();
}

extern "C" int SDOAPI SdoBase_StaticLogin(SdoBaseHandle * handle, const char* inputUserId, const char* password,
	int accountDomain, int autoLoginFlag, int autoLoginKeepTime, int isSupportGeetest, int keepLoginFlag)
{
	LoginClient* app = (LoginClient*)handle;
	return app->StaticLogin(inputUserId, password, accountDomain, autoLoginFlag, autoLoginKeepTime, isSupportGeetest, keepLoginFlag);
}
//WegGame
extern "C" int SDOAPI SdoBase_WeGameLogin(SdoBaseHandle * handle, const char* rail_id, const char* rail_session_ticket, bool is_limited)
{
	LoginClient* app = (LoginClient*)handle;
	return app->WeGameLogin(rail_id, rail_session_ticket, is_limited);
}
extern "C" int SDOAPI SdoBase_WeGameLogin2(SdoBaseHandle * handle, const char* rail_id, const char* rail_session_ticket, bool is_limited, int company_id)
{
	LoginClient* app = (LoginClient*)handle;
	return app->WeGameLogin(rail_id, rail_session_ticket, is_limited, company_id);
}

//QQGame
extern "C" int SDOAPI SdoBase_QQGameLogin(SdoBaseHandle * handle, const char* openid, const char* openkey, bool is_limited, int company_id)
{
	LoginClient* app = (LoginClient*)handle;
	return app->QQGameLogin(openid, openkey, is_limited, company_id);
}

//三方，Lenovo，顺网登
extern "C" int SDOAPI SdoBase_ThirdLogin(SdoBaseHandle * handle, const char* third_token, const char* companyId,
	const char* scene, const char* phone, const char* smsCode, const char* extend, const char* szIsLimited)
{
	LoginClient* app = (LoginClient*)handle;
	return app->ThirdLogin(third_token, companyId, scene, phone, smsCode, extend, szIsLimited);
}

//三方登陆，QQ、WX、WB
extern "C" int SDOAPI SdoBase_ForThirdLogin(SdoBaseHandle * handle, const char* third_token, const char* companyId,
	const char* scene, const char* phone, const char* smsCode, const char* extend, const char* szIsLimited)
{
	LoginClient* app = (LoginClient*)handle;
	return app->ThirdForLogin(third_token, companyId, scene, phone, smsCode, extend, szIsLimited);
}

/////////////////////////////////////////////////
//三方登录灵活获取皮肤展示设置
extern "C" int SDOAPI SdoBase_SetCreateThirdLoginSkinCallback(SdoBaseHandle * handle, SdoBase_CreateThirdLoginSkinCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SetThirdLoginSkinCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_CreateThirdLoginSkin(SdoBaseHandle * handle)
{
	LoginClient* app = (LoginClient*)handle;
	return app->GetThirdLoginSkin();
}
/////////////////////////////////////////////////

extern "C" int SDOAPI SdoBase_StaticLogin2(SdoBaseHandle * handle, const char* inputUserId, const char* password,
	int accountDomain, int autoLoginFlag, int autoLoginKeepTime, int isSupportGeetest, int keepLoginFlag)
{
	LoginClient* app = (LoginClient*)handle;
	return app->StaticLogin2(inputUserId, password, accountDomain, autoLoginFlag, autoLoginKeepTime, isSupportGeetest, keepLoginFlag);
}

extern "C" int SDOAPI SdoBase_StaticLoginWithGameAccount(SdoBaseHandle * handle, const char* inputUserId, const char* password,
	int accountDomain, int autoLoginFlag, int autoLoginKeepTime, int isSupportGeetest, int inputUserType, int keepLoginFlag, const char* scene)
{
	LoginClient* app = (LoginClient*)handle;
	return app->StaticLoginWithGameAccount(inputUserId, password, accountDomain, autoLoginFlag, autoLoginKeepTime, isSupportGeetest, inputUserType, keepLoginFlag, scene);
}

extern "C" int SDOAPI SdoBase_CheckCodeLogin(SdoBaseHandle * handle, const char* checkCode, const char* challenge, const char* validate, const char* seccode, const char* outinfo, int keepLoginFlag, const char* captchaInfo)
{
	LoginClient* app = (LoginClient*)handle;
	return app->CheckCodeLogin(checkCode, challenge, validate, seccode, outinfo, keepLoginFlag, captchaInfo);
}

extern "C" int SDOAPI SdoBase_DynamicLogin(SdoBaseHandle * handle, const char* password, int keepLoginFlag)
{
	LoginClient* app = (LoginClient*)handle;
	return app->DynamicLogin(password, keepLoginFlag);
}

extern "C" int SDOAPI SdoBase_FcmLogin(SdoBaseHandle * handle, const char* realName,
	const char* idCard, const char* email, int keepLoginFlag)
{
	LoginClient* app = (LoginClient*)handle;
	return app->FcmLogin(realName, idCard, email, keepLoginFlag);
}

extern "C" int SDOAPI SdoBase_AutoLogin(SdoBaseHandle * handle, const char* autoLoginSessionKey)
{
	LoginClient* app = (LoginClient*)handle;
	return app->AutoLogin(autoLoginSessionKey);
}

extern "C" int SDOAPI SdoBase_SsoLogin(SdoBaseHandle * handle)
{
	LoginClient* app = (LoginClient*)handle;
	return app->SsoLogin(NULL, NULL);
}

extern "C" int SDOAPI SdoBase_SsoLogin2(SdoBaseHandle * handle, const char* tgt)
{
	LoginClient* app = (LoginClient*)handle;
	return app->SsoLogin(tgt, NULL);
}

extern "C" int SDOAPI SdoBase_SsoLogin3(SdoBaseHandle * handle, const char* tgt, const char* scene)
{
	LoginClient* app = (LoginClient*)handle;
	return app->SsoLogin(tgt, scene);
}

extern "C" int SDOAPI SdoBase_SsoLogin4(SdoBaseHandle * handle, const char* scene, int appId)
{
	LoginClient* app = (LoginClient*)handle;
	return app->SsoLogin(NULL, scene, appId);
}

extern "C" int SDOAPI SdoBase_GetAuthcode(SdoBaseHandle * handle, const char* tgt, const char* scene)
{
	LoginClient* app = (LoginClient*)handle;
	return app->GetAuthCodeRequest(tgt, scene);
}

extern "C" int SDOAPI SdoBase_SetCreateAuthCodeTicketCallback(SdoBaseHandle * handle, SdoBase_CreateAuthCodeTicketCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SetCreateAuthCodeCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_CheckAuthCode(SdoBaseHandle * handle, const char* authCode)
{
	LoginClient* app = (LoginClient*)handle;
	return app->CheckAuthCodeRequest(authCode);
}

extern "C" int SDOAPI SdoBase_SetGetTicketCallback(SdoBaseHandle * handle, SdoBase_GetTicketCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SetGetTicketCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_GetTicket(SdoBaseHandle * handle)
{
	LoginClient* app = (LoginClient*)handle;
	return app->GetTicket();
}

// wegame
extern "C" int SDOAPI SdoBase_SetCreateWeGameOrderCallback(SdoBaseHandle * handle, SdoBase_CreateWeGameOrderCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SetCreateWeGameOrderCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_CreateWeGameOrder(SdoBaseHandle * handle, const char* ticket, const char* szOrderId, const char* szProductId,
	const char* szGroupId, const char* szAreaId, const char* szExtend, const char* szSign, const char* channel)
{
	LoginClient* app = (LoginClient*)handle;
	return app->CreateWeGameOrder(ticket, szOrderId, szProductId, szGroupId, szAreaId, szExtend, szSign, channel);
}

///////////////////////////////////////////////////////////////////////////////////

// 传奇4
//订单创建
extern "C" int SDOAPI SdoBase_SetCreateCQ4OrderCallback(SdoBaseHandle * handle, SdoBase_CreateCQ4OrderCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	app->SetCreateCQ4OrderCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_CreateCQ4Order(SdoBaseHandle * handle, const char* ticket, 
	const char* szOrderId, const char* szProductId,const char* szGroupId, 
	const char* szAreaId, const char* szExtend, const char* szSign,
	const char* channel, int iSSandboxAccount)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->CreateCQ4Order(ticket, szOrderId, szProductId, 
		szGroupId, szAreaId, szExtend, szSign, channel, iSSandboxAccount);
}

//订单查询
extern "C" int SDOAPI SdoBase_SetCreateCQ4QueryCallback(SdoBaseHandle * handle, SdoBase_CreateCQ4QueryCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	app->SetCreateCQ4QueryCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_CreateCQ4Query(SdoBaseHandle * handle, const char* ticket, 
	const char* szOrderId, const char* szSign, const char* channel, int iSSandboxAccount)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->CreateCQ4Query(ticket, szOrderId, szSign, channel, iSSandboxAccount);
}

///////////////////////////////////////////////////////////////////////////////////

extern "C" int SDOAPI SdoBase_SetWeGameStatusCallback(SdoBaseHandle * handle, SdoBase_WeGameStatusCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	app->SetWeGameStatusCallback(callback);
	return 0;
}

int SDOAPI SdoBase_WeGameStatus(SdoBaseHandle* handle)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->WeGameStatus();
}

// steam手游模式
extern "C" int SDOAPI SdoBase_SetCreateSteamChannelOrderCallback(SdoBaseHandle * handle, SdoBase_CreateSteamChannelOrderCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	app->SetCreateSteamChannelOrderCallback(callback);
	return 0;
}

int SDOAPI SdoBase_CreateSteamChannelOrder(SdoBaseHandle* handle, const char* ticket, const char* szOrderId, const char* szProductId,
	const char* szGroupId, const char* szAreaId, const char* szExtend, const char* szSign, const char* channel)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->CreateSteamChannelOrder(ticket, szOrderId, szProductId, szGroupId, szAreaId, szExtend, szSign, channel);
}


int SDOAPI SdoBase_SetQQGameIsLoginCallback(SdoBaseHandle* handle, SdoBase_QQGameIsLoginCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	app->SetQQGameIsLoginCallback(callback);
	return 0;
}

int SDOAPI SdoBase_QQGameIsLogin(SdoBaseHandle* handle, const char* openid, const char* openkey, int company_id)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->QQGameIsLogin(openid, openkey, company_id);
}


// qqgame
int SDOAPI SdoBase_SetCreateQQGameOrderCallback(SdoBaseHandle* handle, SdoBase_CreateQQGameOrderCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	app->SetCreateQQGameOrderCallback(callback);
	return 0;
}

int SDOAPI SdoBase_CreateQQGameOrder(SdoBaseHandle* handle, const char* ticket, const char* szOrderId, const char* szProductId,
	const char* szGroupId, const char* szAreaId, const char* szExtend, const char* szOpenId, const char* szOpenKey, const char* szPfKey, const char* szSign, const char* channel)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->CreateQQGameOrder(ticket, szOrderId, szProductId, szGroupId, szAreaId, szExtend, szOpenId, szOpenKey, szPfKey, szSign, channel);
}

int SDOAPI SdoBase_SetCreateLxOrderCallback(SdoBaseHandle* handle, SdoBase_CreateLxOrderCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	app->SetCreateLxOrderCallback(callback);
	return 0;
}

int SDOAPI SdoBase_CreateLxOrder(SdoBaseHandle* handle, const char* ticket, const char* szOrderId, const char* szProductId, const char* szGroupId,
	const char* szAreaId, const char* szExtend, const char* szSign, const char* channel, const char* szLenovoTgt, const char* szRole)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->CreateLxOrder(ticket, szOrderId, szProductId, szGroupId, szAreaId, szExtend, szSign, channel, szLenovoTgt, szRole);
}

extern "C" int SDOAPI SdoBase_FastLogin(SdoBaseHandle * handle)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->FastLogin();
}

extern "C" int SDOAPI SdoBase_PhoneCheckCodeLogin(SdoBaseHandle * handle, const char* checkCode)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->PhoneCheckCodeLogin(checkCode);
}

extern "C" int SDOAPI SdoBase_QrCodeLogin(SdoBaseHandle * handle, int autoLoginFlag, int autoLoginKeepTime, int keepLoginFlag)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->QrCodeLogin(autoLoginFlag, autoLoginKeepTime, keepLoginFlag);
}

extern "C" int SDOAPI SdoBase_CheckAccoutType(SdoBaseHandle * handle, const char* inputUserId)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->CheckAccoutType(inputUserId);
}

extern "C" int SDOAPI SdoBase_SendPhoneCheckCode(SdoBaseHandle * handle,
	const char* inputUserId, int type)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->SendPhoneCheckCode(inputUserId, type);
}

extern "C" int SDOAPI SdoBase_GetQrCode(SdoBaseHandle * handle)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->GetQrCode();
}

extern "C" int SDOAPI SdoBase_GetLoginState(SdoBaseHandle * handle)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->GetLoginState();
}

extern "C" int SDOAPI SdoBase_GetLoginState2(SdoBaseHandle * handle, const char* tgt)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->GetLoginState();
}

extern "C" int SDOAPI SdoBase_GetSessionId(SdoBaseHandle * handle, char* sessioId)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	string tgt;
	if (app->GetEnableGunionPreInit())
	{
		//gunion场景的扫码支付
		tgt = app->GetToken();
	}
	else
	{
		//登录器场景的扫码支付
		tgt = app->GetTgt();
	}

	if (sessioId == NULL || tgt.length() == 0)
		return -1;
	strcpy(sessioId, tgt.c_str());
	return 0;
}

extern "C" int SDOAPI SdoBase_SetSessionId(SdoBaseHandle * handle, const char* sessioId)
{
	LoginClient* app = (LoginClient*)handle;
	app->SetTgt(sessioId);
	return 0;
}

extern "C" int SDOAPI SdoBase_ExtendLoginState(SdoBaseHandle * handle)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	app->ExtendLoginState(NULL);
	return 0;
}

extern "C" int SDOAPI SdoBase_ExtendLoginState2(SdoBaseHandle * handle, const char* tgt)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	app->ExtendLoginState(tgt);
	return 0;
}

extern "C" int SDOAPI SdoBase_Logout(SdoBaseHandle * handle)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->Logout();
}

extern "C" int SDOAPI SdoBase_PushMessageLogin(SdoBaseHandle * handle, int autoLoginFlag, int autoLoginKeepTime, int keepLoginFlag)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->PushMessageLogin(autoLoginFlag, autoLoginKeepTime, keepLoginFlag);
}

extern "C" int SDOAPI SdoBase_SendPushMessage(SdoBaseHandle * handle, const char* inputUserId, const char* scene)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->SendPushMessage(inputUserId, scene);
}

extern "C" int SDOAPI SdoBase_GetSessionIdStates(SdoBaseHandle * handle, const char* sessioId)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->GetSessionIdStates(sessioId);
}

extern "C" int SDOAPI SdoBase_KickoffAccountVerify(SdoBaseHandle * handle, const char* tgt)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->KickoffAccountVerify(tgt);
}

extern "C" int SDOAPI SdoBase_KickoffProtectCodeLogin(SdoBaseHandle * handle, const char* tgt, const char* protectCode)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->KickoffProtectCodeLogin(tgt, protectCode);
}

extern "C" int SDOAPI SdoBase_KickoffCheckCodeLogin(SdoBaseHandle * handle, const char* tgt, const char* checkCode, int nKickoffAppid, int nKickoffAreaid)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->KickoffCheckCodeLogin(tgt, checkCode, nKickoffAppid, nKickoffAreaid);
}

extern "C" int SDOAPI SdoLsc_ProxyEnable(SdoBaseHandle * handle, int enable)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->ProxyEnable(enable);
}

extern "C" int SDOAPI SdoLsc_RltLogin(SdoBaseHandle * handle, const char* vkey)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->RltLogin(vkey);
}

extern "C" int SDOAPI SdoBase_SetCheckCodeLoginCallback(SdoBaseHandle * handle, SdoBase_CheckCodeLoginCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	app->SetCheckCodeLoginCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_SetDynamicLoginCallback(SdoBaseHandle * handle, SdoBase_DynamicLoginCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	app->SetDynamicLoginCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_SetKickoffAccountVerifyCallback(SdoBaseHandle * handle, SdoBase_KickoffAccountVerifyCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	app->SetKickoffAccountVerifyCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_SetKickoffAccountResultCallback(SdoBaseHandle * handle, SdoBase_KickoffAccountResultCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	app->SetKickoffAccountResultCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_SetFcmLoginCallback(SdoBaseHandle * handle, SdoBase_FcmLoginCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	app->SetFcmLoginCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_SetLoginResultCallback(SdoBaseHandle * handle, SdoBase_LoginResultCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	app->SetLoginResultCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_SetGetDynamicKeyCallback(SdoBaseHandle * handle, SdoBase_GetDynamicKeyCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	app->SetGetDynamicKeyCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_SetSendPhoneCheckCodeCallback(SdoBaseHandle * handle, SdoBase_SendPhoneCheckCodeCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	app->SetSendPhoneCheckCodeCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_SetCheckAccountTypeCallback(SdoBaseHandle * handle, SdoBase_CheckAccounTypeCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	app->SetCheckAccountTypeCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_SetGetQrCodeCallback(SdoBaseHandle * handle, SdoBase_GetQrCodeCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	app->SetGetQrCodeCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_SetGetLoginStateCallback(SdoBaseHandle * handle, SdoBase_GetLoginStateCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	app->SetGetLoginStateCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_SetExtendLoginStateCallback(SdoBaseHandle * handle, SdoBase_ExtendLoginStateCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	app->SetExtendLoginStateCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_SetLogoutCallback(SdoBaseHandle * handle, SdoBase_LogoutCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	app->SetLogoutCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_SetSendPushMessageCallback(SdoBaseHandle * handle, SdoBase_SendPushMessageCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	app->SetSendPushMessageCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_SetCancelPushMessageCallBack(SdoBaseHandle * handle, SdoBase_CancelPushMessageCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	app->SetCancelPushMessageCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_CancelPushMessageLogin(SdoBaseHandle * handle)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	app->CancelPushMessageLogin();
	return 0;
}

extern "C" int SDOAPI SdoBase_SetGetPushMessageStatusCallback(SdoBaseHandle * handle, SdoBase_GetPushMessageStatusCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	app->SetGetPushMessageStatusCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_SetGetSessionIdStatesCallBack(SdoBaseHandle * handle, SdoBase_GetSessionIdStatesCallBack callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	app->SetGetSessionIdStatesCallBack(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_GetPushMessageStatus(SdoBaseHandle * handle, const char* inputUserId)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->GetPushMessageStatus(inputUserId);
}

extern "C" int SDOAPI SdoBase_SetGetAccountInfoCallback(SdoBaseHandle * handle, SdoBase_GetAccountInfoCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	app->SetGetAccountInfoCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_GetAccountInfo(SdoBaseHandle * handle, const char* tgt)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->GetAccountInfo(tgt);
}

extern "C" int SDOAPI SdoBase_SetGetLoginHistoryCallback(SdoBaseHandle * handle, SdoBase_GetLoginHistoryCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	app->SetGetLoginHistoryCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_GetLoginHistory(SdoBaseHandle * handle, int queryNumber)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->GetLoginHistory(NULL, queryNumber);
}

extern "C" int SDOAPI SdoBase_GetLoginHistory2(SdoBaseHandle * handle, const char* tgt, int queryNumber)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->GetLoginHistory(tgt, queryNumber);
}

extern "C" int SDOAPI SdoBase_SetGetLoginUserInfoCallback(SdoBaseHandle * handle, SdoBase_GetLoginUserInfoCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	app->SetGetLoginUserInfoCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_GetLoginUserInfo(SdoBaseHandle * handle, const char* tgt)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->GetLoginUserInfo(tgt);
}

extern "C" int SDOAPI SdoBase_SetLoginUserInfo(SdoBaseHandle * handle, const char* tgt, const char* notename)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->SetLoginUserInfo(tgt, notename);
}

extern "C" int SdoBase_GetParam(SdoBaseHandle * handle, const char* keyVal, char* val)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->GetParam(keyVal, val);
}

extern "C" int SdoBase_SetParam(SdoBaseHandle * handle, const char* keyVal, const char* val)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->SetParam(keyVal, val);
}

extern "C" int SdoBase_PromotionInfoConfirm(SdoBaseHandle * handle, int days)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->PromotionInfoConfirm(days);
}

extern "C" int SdoBase_GetClientVKey(SdoBaseHandle * handle)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->GetClientVKey(NULL);
}

extern "C" int SdoBase_GetClientVKey2(SdoBaseHandle * handle, const char* tgt)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->GetClientVKey(tgt);
}

extern "C" int SdoBase_SendUserAccount(SdoBaseHandle * handle, const char* inputUserId)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->SendUserAccount(inputUserId);
}

extern "C" int SDOAPI SdoBase_GetAccountGroup(SdoBaseHandle * handle, const char* tgt)
{
	TRACET();
	LoginClient* app = (LoginClient*)(handle);
	return app->GetAccountGroup(tgt);
}

extern "C" int SDOAPI SdoBase_AccountGroupLogin(SdoBaseHandle * handle, const char* tgt, const char* sndaId, int autoLoginFlag, int autoLoginKeepTime)
{
	TRACET();
	LoginClient* app = (LoginClient*)(handle);
	return app->AccountGroupLogin(tgt, sndaId, autoLoginFlag, autoLoginKeepTime);
}

extern "C" int SDOAPI SdoBase_ThirdPartyPollingLogin(SdoBaseHandle * handle, const char* companyId, int autoLoginFlag, int autoLoginKeepTime)
{
	TRACET();
	LoginClient* app = (LoginClient*)(handle);
	return app->ThirdPartyPollingLogin(companyId, autoLoginFlag, autoLoginKeepTime);
}

extern "C" int SDOAPI SdoBase_ThirdPartyLogin(SdoBaseHandle * handle, const char* companyId, const char* token, int autoLoginFlag, int autoLoginKeepTime, const char* scene, const char* phone, const char* smsCode)
{
	TRACET();
	LoginClient* app = (LoginClient*)(handle);
	return app->ThirdPartyLogin(companyId, token, autoLoginFlag, autoLoginKeepTime, scene, phone, smsCode);
}

extern "C" int SDOAPI SdoBase_SetGetLoginAreaInfoCallback(SdoBaseHandle * handle, SdoBase_GetLoginAreaInfoCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	app->SetGetLoginAreaInfoCallback(callback);
	return 0;
}

//extern "C" int SDOAPI SdoBase_SetWeGameLoginCallback(SdoBaseHandle* handle, SdoBase_LoginResultCallback callback)
//{
//	LoginClient* app = (LoginClient*)handle;
//	app->SetWeGameLoginCallback(callback);
//	return 0;
//}

extern "C" int SDOAPI SdoBase_GetLoginAreaInfo(SdoBaseHandle * handle, int nAppId)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->GetLoginAreaInfo(nAppId);
}

extern "C" int SDOAPI SdoBase_CloudGameLogin(SdoBaseHandle * handle, const char* tgt, const char* scene/*=NULL*/)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->CloudGameLogin(tgt, scene);
}

extern "C" int SDOAPI SdoBase_SendMiGuSms(SdoBaseHandle * handle, const char* phone)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->SendMiGuSms(phone);
}

extern "C" int SDOAPI SdoBase_SendSms(SdoBaseHandle * handle, const char* smsSessionKey, const char* phone, int smsType, const char* scene)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->SendSms(smsSessionKey, phone, smsType, scene);
}

extern "C" int SDOAPI SdoBase_CheckCodeToSendSms(SdoBaseHandle * handle, const char* smsSessionKey, const char* checkCode, const char* captchaInfo)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->CheckCodeToSendSms(smsSessionKey, checkCode, captchaInfo);
}

extern "C" int SDOAPI SdoBase_SmsLogin(SdoBaseHandle * handle, const char* smsSessionKey, const char* smsCode, int autoLoginFlag, int autoLoginKeepTime, int keepLoginFlag)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->SmsLogin(smsSessionKey, smsCode, autoLoginFlag, autoLoginKeepTime, keepLoginFlag);
}

extern "C" int SDOAPI SdoBase_SetUserPrivacyConfigCallback(SdoBaseHandle * handle, SdoBase_UserPrivacyConfigCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL) {
		return -1;
	}
	app->SetUserPrivacyConfigCallback(callback);
	return 0;
}

extern "C" int SdoBase_UserPrivacyConfig(SdoBaseHandle * handle, const char* scene, int privacypolicyversion, int serviceAgreementVersion)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL) {
		return -1;
	}
	return app->UserPrivacyConfig(scene, privacypolicyversion, serviceAgreementVersion);
}

//新版隐私协议
extern "C" int SDOAPI SdoBase_SetNewVersionUserPrivacyConfigCallback(SdoBaseHandle * handle, SdoBase_NewVersionUserPrivacyConfigCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL) {
		return -1;
	}
	app->SetNewVersionUserPrivacyConfigCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_NewVersionUserPrivacyConfig(SdoBaseHandle * handle, const char* scene, int privacypolicyversion)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL)
	{
		return -1;
	}
	return app->NewVersionUserPrivacyConfig(scene, privacypolicyversion);
}
/////

extern "C" int SDOAPI SdoBase_ModitfyThirdLoginExtern(SdoBaseHandle * handle, int thirdLoginExtern, bool verifyCert)
{
	TRACET();
	LoginClient* app = new LoginClient(verifyCert);
	LoginClient::thirdLoginExtern = thirdLoginExtern;
	return 0;
}

extern "C" int SDOAPI SdoBase_SetFaceVerifyInitCallback(SdoBaseHandle * handle, SdoBase_FaceVerifyInitCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL) {
		return -1;
	}
	app->SetFaceVerifyInitCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_FaceVerifyInit(SdoBaseHandle * handle, const char* scene)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL) {
		return -1;
	}
	return app->FaceVerifyInit(scene);
}

extern "C" int SDOAPI SdoBase_SetGetFaceCodeResultCallback(SdoBaseHandle * handle, SdoBase_GetFaceCodeResultCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL) {
		return -1;
	}
	app->SetGetFaceCodeResultCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_GetFaceCodeResult(SdoBaseHandle * handle, const char* contextId)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL) {
		return -1;
	}
	return app->GetFaceCodeResult(contextId);
}

extern "C" int SDOAPI SdoBase_SetSendActionCallback(SdoBaseHandle * handle, SdoBase_SendActionCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL) {
		return -1;
	}
	app->SetSendActionCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_SendAction(SdoBaseHandle * handle, const char* contextId, int action)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL) {
		return -1;
	}
	return app->SendAction(contextId, action);
}

extern "C" int SDOAPI SdoBase_InitLog(const char* logFileName, const char* mode)
{
	TRACET();
	freopen(logFileName, mode, stdout);
	freopen(logFileName, mode, stderr);

	return 0;
}

extern "C" int SDOAPI SdoBase_SetUeInitClientCallback(SdoBaseHandle * handle, SdoBase_UeInitClientCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL) {
		return -1;
	}
	app->SetUeInitClientCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_UeInitClient(SdoBaseHandle * handle, const char* hash)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL) {
		return -1;
	}
	return app->UeInitClient(hash);
}

extern "C" int SDOAPI SdoBase_UeReport(SdoBaseHandle * handle, const char* szExtendData)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL) {
		return -1;
	}
	return app->UeReport(szExtendData);
}

extern "C" int SDOAPI SdoBase_SendSteamPayResult(SdoBaseHandle * handle, const char* szSteamId, const char* szOrderId)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL) {
		return -1;
	}
	return app->SendSteamPayResult(szSteamId, szOrderId);
}

extern "C" int SDOAPI SdoBase_SendSteamChannelPayResult(SdoBaseHandle * handle, const char* szChannel, const char* szTicket, const char* szOrderId)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL) {
		return -1;
	}
	return app->SendSteamChannelPayResult(szChannel, szTicket, szOrderId);
}

//获取登录器功能相关配置
extern "C" int SDOAPI SdoBase_SetGetClientConfigCallback(SdoBaseHandle * handle, SdoBase_GetClientConfigCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL) {
		return -1;
	}
	app->SetGetClientConfigCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_GetClientConfig(SdoBaseHandle * handle)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL)
	{
		return -1;
	}
	return app->GetClientConfig();
}

//下行短信相关配置初始化
extern "C" int SDOAPI SdoBase_SetGoDownConfigInitCallback(SdoBaseHandle * handle, SdoBase_GoDownConfigInitCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL) {
		return -1;
	}
	app->SetGoDownConfigCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_GoDownConfigInit(SdoBaseHandle * handle, const char* account, const char* isVoice, const char* m_flowid)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL)
	{
		return -1;
	}
	return app->GoDownConfigInit(account, isVoice, m_flowid);
}

//下行短信带图验验证
extern "C" int SDOAPI SdoBase_SetGoDownSendSmsCheckCodeCallback(SdoBaseHandle * handle, SdoBase_GoDownConfigSendSmsCheckCodeCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL) {
		return -1;
	}
	app->SetGoDownSendSmsCheckCodeCallBack(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_GoDownSendSmsCheckCode(SdoBaseHandle * handle, const char* captchaInfo)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL)
	{
		return -1;
	}
	return app->GoDownSendSmsCheckCode(captchaInfo);
}

//确认发送下行短信
extern "C" int SDOAPI SdoBase_SetGoDownconfirmSendSmsCallback(SdoBaseHandle * handle, SdoBase_GoDownconfirmSendSmsCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL) {
		return -1;
	}
	app->SetGoDownconfirmSendSmsCallBack(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_GoDownGoDownconfirmSendSms(SdoBaseHandle * handle, int isVoice)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL)
	{
		return -1;
	}
	return app->GoDownconfirmSendSms(isVoice);
}

//确认下行短信登录
extern "C" int SDOAPI SdoBase_SetGoDownconfirmLoginCallback(SdoBaseHandle * handle, SdoBase_GoDownconfirmLoginCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL) {
		return -1;
	}
	app->SetGoDownconfirmLoginCallBack(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_GoDownGoDownconfirmLogin(SdoBaseHandle * handle, const char* account, const char* verifyCode, int keepLoginFlag)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL)
	{
		return -1;
	}
	return app->GoDownconfirmLogin(account, verifyCode, keepLoginFlag);
}

//快速登录
extern "C" int SDOAPI SdoBase_SetFastLoginLoginCallback(SdoBaseHandle * handle, SdoBase_FastLoginLoginCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL) {
		return -1;
	}
	app->SetFastLoginLoginCallBack(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_FastLoginConfirmLogin(SdoBaseHandle * handle, const char* autoLoginKey)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL)
	{
		return -1;
	}
	return app->FastLogin(autoLoginKey);
}

extern "C" int SDOAPI SdoBase_SetGetTicketLoginCallback(SdoBaseHandle * handle, SdoBase_NewVersionGetTicketCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL) {
		return -1;
	}
	app->SetGetTicketLoginCallBack(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_GetTicketNew(SdoBaseHandle * handle)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL)
	{
		return -1;
	}
	return app->GetTicketNew();
}

extern "C" int SDOAPI SdoBase_SetOtherLoginSecurityPhoneCallback(SdoBaseHandle * handle, SdoBase_OtherLoginSecurityPhoneCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL) {
		return -1;
	}
	app->SetOtherLoginSecurityPhoneCallBack(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_SetFastLoginActiveDeleteAccountLoginCallback(SdoBaseHandle * handle, SdoBase_FastLoginActiveDeleteAccountLoginCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL) {
		return -1;
	}
	app->SetFastLoginActiveDeleteAccountCallBack(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_FastLoginActiveDeleteAccount(SdoBaseHandle * handle, const char* keepLoginKey, const char* inputUserId)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL)
	{
		return -1;
	}
	return app->FastLoginActiveDeleteAccount(keepLoginKey, inputUserId);
}

extern "C" int SDOAPI SdoBase_SetCheckAccountTypeLoginCallback(SdoBaseHandle * handle, SdoBase_CheckAccountTypeLoginCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL) {
		return -1;
	}
	app->SetCheckAccountTypeLoginCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_CheckAccountLoginType(SdoBaseHandle * handle, const char* tgt)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL)
	{
		return -1;
	}
	return app->CheckAccountLoginType(tgt);
}

//查询指定日期的星座运势情况，配置返回在sdologin内部处理json串
extern "C" int SDOAPI SdoBase_SetCalculateCallback(SdoBaseHandle * handle, SdoBase_CalculateCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL) {
		return -1;
	}
	app->SetCalculateCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_CheckCalculate(SdoBaseHandle * handle, const char* star_sign, const char* date)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL)
	{
		return -1;
	}
	return app->CheckCalculate(star_sign, date);
}

extern "C" int SDOAPI SdoBase_SetInitAdvCallback(SdoBaseHandle * handle, SdoBase_InitAdvCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL) {
		return -1;
	}
	app->SetInitAdvCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_CheckInitAdv(SdoBaseHandle * handle)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL)
	{
		return -1;
	}
	return app->CheckInitAdv();
}

extern "C" int SDOAPI SdoBase_SetTraceId(SdoBaseHandle * handle, const char* traceId)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL)
	{
		return -1;
	}
	return app->SetTraceId(traceId);
}

extern "C" int SDOAPI SdoBase_ClearCodeKey(SdoBaseHandle * handle)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL)
	{
		return -1;
	}
	return app->ClearCodeKey();
}

extern "C" int SDOAPI SdoBase_SetGuardDianSendSmsCallback(SdoBaseHandle * handle, SdoBase_GuardDianSendSmsCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL) {
		return -1;
	}
	app->SetGuardDianSendSmsCallBack(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_GuardDianSendSms(SdoBaseHandle * handle, const char* flowId, const char* phone, int isVoice)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL)
	{
		return -1;
	}
	return app->GuardDianSendSms(flowId, phone, isVoice);
}

extern "C" int SDOAPI SdoBase_SetGuardDianSendSmsCheckCodeCallback(SdoBaseHandle * handle, SdoBase_GuardDianSendSmsCheckCodeCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL) {
		return -1;
	}
	app->SetGuardDianSendSmsCheckCodeCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_GuardDianSendSmsCheckCode(SdoBaseHandle * handle, const char* flowId, const char* captchaInfo, int isVoice)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL)
	{
		return -1;
	}
	return app->GuardDianSendSmsCheckCode(flowId, captchaInfo, isVoice);
}

extern "C" int SDOAPI SdoBase_SetGuardDianConfrimSendSmsResultCallback(SdoBaseHandle * handle, SdoBase_GuardDianConfrimSendSmsResultCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL) {
		return -1;
	}
	app->SetGuardDianConfrimSendSmsResultCallBack(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_GuardDianConfrimSendSmsResult(SdoBaseHandle * handle, int ageCheckFlag, const char* flowId, const char* verifyCode,
	const char* tutorName, const char* tutorIdCard, const char* phone)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL)
	{
		return -1;
	}
	return app->GuardDianConfrimSendSmsResult(ageCheckFlag, flowId, verifyCode, tutorName, tutorIdCard, phone);
}

extern "C" int SDOAPI SdoBase_SetGuardDianCallback(SdoBaseHandle * handle, SdoBaseGuardDianCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL) {
		return -1;
	}
	app->SetGuardDianResultCallBack(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_SetSdoVersion(SdoBaseHandle * handle, const char* sdoVersion)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL)
	{
		return -1;
	}
	return app->SetSdoVersion(sdoVersion);
}

extern "C" int SDOAPI SdoLsc_RltLoginKeepLogin(SdoBaseHandle * handle, const char* vkey, int keepLoginFlag)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL)
	{
		return -1;
	}
	app->RltLoginKeepLogin(vkey, keepLoginFlag);
	return 0;
}

extern "C" int SDOAPI SdoBase_SetRunTimerId(SdoBaseHandle * handle, const char* runTimeId)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL)
	{
		return -1;
	}
	app->SetRunTimerId(runTimeId);
	return 0;
}

extern "C" int SDOAPI SdoBase_DownloadImageA(
	SdoBaseHandle * handle,
	const char* url,
	unsigned char** outBuf,
	int* outSize)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL)
	{
		return -1;
	}

	if (!url || !outBuf || !outSize)
		return -1;

	*outBuf = nullptr;
	*outSize = 0;

	std::vector<unsigned char> buffer;

	// ---------------------------------------------------------
	// Debug 输出（采用 TRACEI）
	// ---------------------------------------------------------
	TRACEI("[DL-IMG] SdoBase_DownloadImageA URL = %s", url);

	// ---------------------------------------------------------
	// 使用 LoginClient 中的 verifyCert 控制证书校验
	// ---------------------------------------------------------
	bool verifyCert = app->GetVerifyCert();  // 核心点

	// ---------------------------------------------------------
	// 调用 Curl 下载
	// ---------------------------------------------------------
	bool ok = CurlHttpDownload::DownloadToMemory(
		url,
		buffer,
		5000,          // 超时
		verifyCert     // 证书校验开关
	);

	if (!ok)
	{
		TRACEE(L"[DL-IMG] CurlHttpDownload FAILED url=%S", url);
		return -2;
	}

	if (buffer.empty())
	{
		TRACEE(L"[DL-IMG] Download OK but NO DATA url=%S", url);
		return -3;
	}

	// ---------------------------------------------------------
	// 分配返回缓冲区
	// ---------------------------------------------------------
	*outSize = (int)buffer.size();
	*outBuf = (unsigned char*)malloc(*outSize);

	if (!*outBuf)
	{
		TRACEE(L"[DL-IMG] malloc FAILED, size=%d", *outSize);
		return -4;
	}

	memcpy(*outBuf, buffer.data(), *outSize);

	TRACEI("[DL-IMG] Download SUCCESS size=%d bytes url=%s", *outSize, url);

	return 0;
}

extern "C" int SDOAPI SdoBase_SetFaceRecognitionUrlCallback(
	SdoBaseHandle* handle, 
	SdoBaseSetFaceRecognitionUrlCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL) {
		return -1;
	}
	app->SetFaceRecognitionUrlCallBack(callback);
	return 0;
}

extern "C" int SdoBase_FaceRecognitionUrlResult(
	SdoBaseHandle* handle, 
	const char* confType, 
	const char* confKey)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL)
	{
		return -1;
	}
	return app->SetFaceRecognitionUrl(confType, confKey);
}

extern "C" int SDOAPI SdoBase_SetQueryFaceVerifyTicketStatusCallback(
	SdoBaseHandle* handle, 
	SdoBaseSetQueryFaceVerifyTicketStatusCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL) {
		return -1;
	}
	app->SetQueryFaceVerifyTicketStatusCallBack(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_QueryFaceVerifyTicketStatusResult(
	SdoBaseHandle* handle, 
	const char* appId, 
	const char* ticket)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL)
	{
		return -1;
	}
	return app->SetQueryFaceVerifyTicketStatus(appId, ticket);
}

extern "C" int SDOAPI SdoBase_SetFaceHolderRegistrationUrlCallback(
	SdoBaseHandle* handle, 
	SdoBaseSetFaceHolderRegistrationUrlCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL) {
		return -1;
	}
	app->SetFaceHolderRegistrationUrlCallBack(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_FaceHolderRegistrationUrlResult(
	SdoBaseHandle* handle, 
	const char* confType, 
	const char* confKey)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL)
	{
		return -1;
	}
	return app->SetFaceHolderRegistrationUrl(confType, confKey);
}

/**
* 登录器激活码登录
*/
extern "C" int SDOAPI SdoBase_SetActiveCodeLoginCallback(
	SdoBaseHandle * handle,
	SdoBaseActiveCodeLoginCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL) {
		return -1;
	}
	app->SetActiveLoginCallBack(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_GetActiveCodeLogin(
	SdoBaseHandle * handle,
	const char* activeCode)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL)
	{
		return -1;
	}
	return app->GetActiveCodeLogin(activeCode);
}

/**
* 设置需要展示激活界面的回调
*/
extern "C" int SDOAPI SdoBase_SetShowActiveCodeLoginDlgCallback(
	SdoBaseHandle * handle,
	SdoBaseShowActiveCodeLoginDlgCallback callback)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	if (app == NULL) {
		return -1;
	}
	app->SetShowActiveLoginDlgCallBack(callback);
	return 0;
}

/**
* 设置 GunionWegame 初始化回调
*/
extern "C" int SDOAPI SdoBase_SetCreatGunionWegameInitCallback(
	SdoBaseHandle * handle,
	SdoBase_CreateGunionWegameInitCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SetCreateGunionWegameInitCallback(callback);
	return 0;
}

/**
* GunionWegame 初始化
*/
extern "C" int SDOAPI SdoBase_GunionWegameInit(
	SdoBaseHandle * handle,
	const char* appId)
{
	LoginClient* app = (LoginClient*)handle;
	return app->CheckGunionWegameInit(appId);
}

/**
* 设置 GunionWegame 登录回调
*/
extern "C" int SDOAPI SdoBase_SetCreatGunionWegameLoginCallback(
	SdoBaseHandle * handle,
	SdoBase_CreateGunionWegameLoginCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SetCreateGunionWegameLoginCallback(callback);
	return 0;
}

/**
* GunionWegame 登录
*/
extern "C" int SDOAPI SdoBase_GunionWeGameLogin(
	SdoBaseHandle * handle,
	const char* rail_id,
	const char* rail_session_ticket,
	bool is_limited,
	int company_id)
{
	LoginClient* app = (LoginClient*)handle;
	return app->GetCheckGunionWegameLogin(
		rail_id,
		rail_session_ticket,
		is_limited,
		company_id);
}

/**
* 设置 GunionWegame 短信发送回调
*/
extern "C" int SDOAPI SdoBase_SetCreatGunionWegameSmsSendCallback(
	SdoBaseHandle * handle,
	SdoBase_CreateGunionWegameSmsSendCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SetCreateGunionWegameSmsSendCallback(callback);
	return 0;
}

/**
* GunionWegame 短信发送
*/
extern "C" int SDOAPI SdoBase_GunionWegameSmsSend(
	SdoBaseHandle * handle,
	const char* phone,
	const char* voice_msg,
	const char* str_new_deviceid_server)
{
	LoginClient* app = (LoginClient*)handle;
	return app->CheckGunionWegameSmsSend(
		phone,
		voice_msg,
		str_new_deviceid_server);
}

/**
* 设置 GunionWegame 带图验短信发送回调
*/
extern "C" int SDOAPI SdoBase_SetCreatGunionWegamePicSmsSendCallback(
	SdoBaseHandle * handle,
	SdoBase_CreateGunionWegamePicSmsSendCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SetCreateGunionWegamePicSmsSendCallback(callback);
	return 0;
}

/**
* GunionWegame 带图验短信发送
*/
extern "C" int SDOAPI SdoBase_GunionWegamePicSmsSend(
	SdoBaseHandle * handle,
	const char* imagecodeType,
	const char* voice_msg,
	const char* captchaInfo,
	const char* str_new_deviceid_server)
{
	LoginClient* app = (LoginClient*)handle;
	return app->CheckGunionWegamePicSmsSend(
		imagecodeType,
		voice_msg,
		captchaInfo,
		str_new_deviceid_server);
}

/**
* 设置 GunionWegame 三方账号绑定手机回调
*/
extern "C" int SDOAPI SdoBase_SetCreatGunionWegameThirdAccountBindPhoneCallback(
	SdoBaseHandle * handle,
	SdoBase_CreateGunionWegameThirdAccountBindPhoneCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SetCreateGunionWegameThirdAccountBindPhoneCallback(callback);
	return 0;
}

/**
* GunionWegame 三方账号绑定手机号
*/
extern "C" int SDOAPI SdoBase_GunionWegameThirdAccountBindPhone(
	SdoBaseHandle * handle,
	const char* phone,
	const char* sms_code,
	const char* str_new_deviceid_server)
{
	LoginClient* app = (LoginClient*)handle;
	return app->CheckGunionWegameThirdAccountBindPhone(
		phone,
		sms_code,
		str_new_deviceid_server);
}

/**
* 设置 GunionWegame 实名回调
*/
extern "C" int SDOAPI SdoBase_SetCreatGunionWegameFillRealinfoCallback(
	SdoBaseHandle * handle,
	SdoBase_CreateGunionWegameFillRealinfoCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SdoBase_SetCreatGunionWegameFillRealinfoCallback(callback);
	return 0;
}

/**
* GunionWegame 实名
*/
extern "C" int SDOAPI SdoBase_GunionWegameFillRealinfo(
	SdoBaseHandle * handle,
	const char* idcard,
	const char* name,
	const char* realInfoFlowId,
	const char* str_new_deviceid_server)
{
	LoginClient* app = (LoginClient*)handle;
	return app->CheckGunionWegameFillRealinfo(
		idcard,
		name,
		realInfoFlowId,
		str_new_deviceid_server);
}

/**
* 设置 GunionWegame 获取票据回调
*/
extern "C" int SDOAPI SdoBase_SetCreatGunionWegameGetTicketCallback(
	SdoBaseHandle * handle,
	SdoBase_CreateGunionWegameGetTicketCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SdoBase_SetCreatCheckGetTicketCallback(callback);
	return 0;
}

/**
* GunionWegame 获取票据
*/
extern "C" int SDOAPI SdoBase_GunionWegameGetTicket(
	SdoBaseHandle * handle,
	int time_s,
	const char* str_new_deviceid_server)
{
	LoginClient* app = (LoginClient*)handle;
	return app->CheckGunionWegameGetTicket(
		time_s,
		str_new_deviceid_server);
}

/**
* 设置 GunionWegame 获取支付票据回调
*/
extern "C" int SDOAPI SdoBase_SetCreatGunionWegameGetPayTicketCallback(
	SdoBaseHandle * handle,
	SdoBase_CreateGunionWegameGetPayTicketCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SdoBase_SetCreatCheckGetPayTicketCallback(callback);
	return 0;
}

/**
* GunionWegame 获取支付票据
*/
extern "C" int SDOAPI SdoBase_GunionWegameGetPayTicket(
	SdoBaseHandle * handle,
	int time_s,
	const char* str_new_deviceid_server)
{
	LoginClient* app = (LoginClient*)handle;
	return app->CheckGunionWegameGetPayTicket(
		time_s,
		str_new_deviceid_server);
}

/**
* GunionWegame 设置 token
*/
extern "C" int SDOAPI SdoBase_GunionWegameSetToken(
	SdoBaseHandle * handle,
	const char* token)
{
	LoginClient* app = (LoginClient*)handle;
	return app->CheckGunionWegameSetToken(token);
}

/**
* GunionWegame 设置 randKey
*/
extern "C" int SDOAPI SdoBase_GunionWegameSetRandKey(
	SdoBaseHandle * handle,
	const char* randKey)
{
	LoginClient* app = (LoginClient*)handle;
	return app->CheckGunionWegameSetRandKey(randKey);
}

//======================================================================
// GHOME_PC C接口导出层
// 说明：
// 1. 所有 handle 实际为 LoginClient* 类型
// 2. 本层只做接口导出与类型转换
// 3. 不包含任何业务逻辑
// 4. 所有实际逻辑在 LoginClient 内部完成
//======================================================================

//======================================================
// 隐私协议相关接口
//======================================================
/**
 * 设置隐私协议跳转回调
 */
extern "C" int SDOAPI SdoBase_SetCreatDoPrivateCallback(
	SdoBaseHandle * handle,
	SdoBase_CreateDoPrivateCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SetCreateDoPrivateCallback(callback);
	return 0;
}
/**
 * 发起隐私协议请求
 */
extern "C" int SDOAPI SdoBase_DoPrivate(
	SdoBaseHandle * handle,
	const char* appId,
	const char* scene)
{
	LoginClient* app = (LoginClient*)handle;
	return app->CheckDoPrivateRequest(appId, scene);
}

/**
 * 设置短信发送回调
 */
extern "C" int SDOAPI SdoBase_SetCreatDoSmsSendCallback(
	SdoBaseHandle * handle, 
	SdoBase_CreateDoSmsSendCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SetCreateDoSmsSendCallback(callback);
	return 0;
}

/**
 * 短信发送
 */
extern "C" int SDOAPI SdoBase_DoSmsSend(
	SdoBaseHandle * handle, 
	const char* phone, 
	const char* voiceMsg,
	const char* smsType,
	const char* str_new_deviceid_server)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->CheckDoSmsSend(phone, voiceMsg, smsType, str_new_deviceid_server);
}

/**
 * 设置图验短信发送回调
 */
extern "C" int SDOAPI SdoBase_SetCreatCheckPicSmsSendCallback(
	SdoBaseHandle * handle, 
	SdoBase_CreateCheckPicSmsSendCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SetCreateCheckPicSmsSendCallback(callback);
	return 0;
}

/**
 * 短信带图验发送
 */
extern "C" int SDOAPI SdoBase_CheckPicSmsSend(
	SdoBaseHandle * handle, 
	const char* imagecodeType, 
	const char* voiceMsg,
	const char* captchaInfo,
	const char* smsType,
	const char* str_new_deviceid_server)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->CheckCheckPicSmsSend(imagecodeType, voiceMsg, captchaInfo, smsType, str_new_deviceid_server);
}

/**
 * 设置短信登录回调
 */
extern "C" int SDOAPI SdoBase_SetCreatCheckSmsLoginCallback(
	SdoBaseHandle * handle,
	SdoBase_CreateCheckSmsLoginCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SetCreateCheckSmsLoginCallback(callback);
	return 0;
}

/**
 * 短信验证码登录
 */
extern "C" int SDOAPI SdoBase_CheckSmsLogin(
	SdoBaseHandle * handle,
	const char* phone,
	const char* sms_code,
	const char* str_new_deviceid_server)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->CheckCheckSmsLogin(phone, sms_code, str_new_deviceid_server);
}

//======================================================
// 实名认证
//======================================================

/**
 * 设置实名认证回调
 */
extern "C" int SDOAPI SdoBase_SetCreatCheckRealNameCallback(
	SdoBaseHandle * handle,
	SdoBase_CreateCheckRealNameCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SdoBase_SetCreatCheckRealNameCallback(callback);
	return 0;
}

/**
 * 实名认证校验
 */
extern "C" int SDOAPI SdoBase_CheckRealName(
	SdoBaseHandle * handle,
	const char* idcard,
	const char* name,
	const char* str_new_deviceid_server)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->CheckCheckRealName(idcard, name, str_new_deviceid_server);
}

//======================================================
// 自动登录
//======================================================
/**
 * 设置自动登录回调
 */
extern "C" int SDOAPI SdoBase_SetCreatCheckAutoLoginCallback(
	SdoBaseHandle * handle,
	SdoBase_CreateCheckAutoLoginCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SdoBase_SetCreatCheckAutoLoginCallback(callback);
	return 0;
}

/**
 * 执行自动登录
 */
extern "C" int SDOAPI SdoBase_CheckAutoLogin(
	SdoBaseHandle * handle,
	const char* autokey,
	const char* str_new_deviceid_server)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->CheckCheckAutoLogin(autokey, str_new_deviceid_server);
}

//======================================================
// 登出
//======================================================

/**
 * 设置登出回调
 */
extern "C" int SDOAPI SdoBase_SetCreatCheckLogoutCallback(
	SdoBaseHandle * handle,
	SdoBase_CreateCheckLogoutCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SdoBase_SetCreatCheckLogOutCallback(callback);
	return 0;
}

/**
 * 执行登出操作
 */
extern "C" int SDOAPI SdoBase_CheckLogout(
	SdoBaseHandle * handle,
	const char* autokey,
	const char* str_new_deviceid_server)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->CheckCheckLogout(autokey, str_new_deviceid_server);
}

//======================================================
// 密码登录
//======================================================
/**
 * 设置密码登录回调
 */
extern "C" int SDOAPI SdoBase_SetCreatDoPwdLoginCallback(
	SdoBaseHandle * handle,
	SdoBase_CreateDoPwdLoginCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SdoBase_SetCreatCheckPwdLoginCallback(callback);
	return 0;
}

/**
 * 发起密码登录请求
 */
extern "C" int SDOAPI SdoBase_DoPwdLogin(
	SdoBaseHandle * handle,
	const char* phone,
	const char* password,
	const char* str_new_deviceid_server)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->CheckDoPwdLogin(phone, password, str_new_deviceid_server);
}

//======================================================
// 密码登录图验流程
//======================================================
/**
 * 设置图验密码登录回调
 */
extern "C" int SDOAPI SdoBase_SetCreatCheckPicPwdLoginCallback(
	SdoBaseHandle * handle,
	SdoBase_CreateCheckPicPwdLoginCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SdoBase_SetCreatCheckPicPwdLoginCallback(callback);
	return 0;
}

 //======================================================
 // 静密登录执行图验校验登录
 //======================================================
extern "C" int SDOAPI SdoBase_CheckPicPwdLogin(
	SdoBaseHandle * handle,
	const char* imagecodeType,
	const char* captchaInfo,
	const char* str_new_deviceid_server)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->CheckDoPicPwdLogin(imagecodeType, captchaInfo, str_new_deviceid_server);
}


//======================================================
// 第三方登录（QQ/WX/WB 等）
//======================================================
/**
 * 设置第三方登录回调
 */
extern "C" int SDOAPI SdoBase_SetCreatCheckThirdLoginCallback(
	SdoBaseHandle * handle,
	SdoBase_CreateCheckThirdLoginCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SdoBase_SetCreatCheckThirdLoginCallback(callback);
	return 0;
}

/**
 * 执行第三方登录QQ/WX/WB
 */
extern "C" int SDOAPI SdoBase_CheckThirdLogin(
	SdoBaseHandle * handle,
	const char* company_id,
	const char* third_ticket,
	const char* code_verify,
	const char* openid,
	const char* uid,
	const char* str_new_deviceid_server)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->CheckDoThirdLogin(company_id, third_ticket, code_verify, openid, uid, str_new_deviceid_server);
}

//======================================================
// 个性账号静密登录
//======================================================
extern "C" int SDOAPI SdoBase_SetCreatDoSpecialPwdLoginCallback(
	SdoBaseHandle * handle,
	SdoBase_CreateDoSpecialPwdLoginCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SdoBase_SetCreatCheckSpecialPwdLoginCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_DoSpecialPwdLogin(
	SdoBaseHandle * handle,
	const char* account,
	const char* password,
	const char* str_new_deviceid_server)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->CheckDoSpecialPwdLogin(account, password, str_new_deviceid_server);
}

//======================================================
// 个性账号图验静密登录
//======================================================
extern "C" int SDOAPI SdoBase_SetCreatCheckSpecialPicPwdLoginCallback(
	SdoBaseHandle * handle,
	SdoBase_CreateCheckSpecialPicPwdLoginCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SdoBase_SetCreatCheckSpecialPicPwdLoginCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_CheckSpecialPicPwdLogin(
	SdoBaseHandle * handle,
	const char* imagecodeType,
	const char* captchaInfo,
	const char* str_new_deviceid_server)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->CheckDoSpecialPicPwdLogin(imagecodeType, captchaInfo, str_new_deviceid_server);
}	

//======================================================
// 登记区服记录
//======================================================
extern "C" int SDOAPI SdoBase_SetCreatCheckLoginAreaCallback(
	SdoBaseHandle * handle,
	SdoBase_CreateCheckLoginAreaCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SdoBase_SetCreatCheckLoginAreaLoginCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_CheckLoginAreaLogin(
	SdoBaseHandle * handle,
	const char* userid,
	const char* area,
	const char* group,
	const char* str_new_deviceid_server)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->CheckDoLoginArea(userid, area, group, str_new_deviceid_server);
}

//======================================================
// 获取区服配置
//======================================================
extern "C" int SDOAPI SdoBase_SetCreatCheckGetLoginAreaCallback(
	SdoBaseHandle * handle,
	SdoBase_CreateCheckGetLoginAreaCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SdoBase_SetCreatCheckGetLoginAreaLoginCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_CheckGetLoginAreaLogin(
	SdoBaseHandle * handle,
	const char* str_new_deviceid_server)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->CheckDoGetLoginArea(str_new_deviceid_server);
}

//======================================================
// 个性账号短信发送
//======================================================
extern "C" int SDOAPI SdoBase_SetCreatDoSpecialSmsSendCallback(
	SdoBaseHandle * handle,
	SdoBase_CreateDoSpecialSmsSendCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SetCreateDoSpecialSmsSendCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_DoSpecialSmsSend(
	SdoBaseHandle * handle,
	const char* account,
	const char* voice_msg,
	const char* str_new_deviceid_server)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->CheckDoSpecialSmsSend(account, voice_msg, str_new_deviceid_server);
}

//======================================================
// 个性账号图验短信发送
//======================================================
extern "C" int SDOAPI SdoBase_SetCreatCheckSpecialPicSmsSendCallback(
	SdoBaseHandle * handle,
	SdoBase_CreateCheckSpecialPicSmsSendCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SetCreateCheckSpecialPicSmsSendCallback(callback);
	return 0;
}

//======================================================
// 个性账号图验短信确认发送
//======================================================
extern "C" int SDOAPI SdoBase_CheckSpecialPicSmsSend(
	SdoBaseHandle * handle,
	const char* imagecodeType,
	const char* captchaInfo,
	const char* str_new_deviceid_server)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->CheckSpecialCheckPicSmsSend(imagecodeType, captchaInfo, str_new_deviceid_server);
}

extern "C" int SDOAPI SdoBase_SetCreatDoSpecialConfirmSmsSendCallback(
	SdoBaseHandle * handle, 
	SdoBase_CreateDoSpecialConfirmSmsSendCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SetCreateDoSpecialConfirmSmsSendCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_DoSpecialConfirmSmsSend(
	SdoBaseHandle * handle,
	const char* str_new_deviceid_server)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	app->CheckDoSpecialConfirmSmsSend(str_new_deviceid_server);
	return 0;
}

//======================================================
// 个性账号短信登录
//======================================================
extern "C" int SDOAPI SdoBase_SetCreatDoSpecialCheckSmsLoginCallback(
	SdoBaseHandle * handle, 
	SdoBase_CreateDoSpecialCheckSmsLoginCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SetCreatDoSpecialCheckSmsLoginCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_DoSpecialCheckSmsLogin(
	SdoBaseHandle * handle, 
	const char* account, 
	const char* sms_code,
	const char* str_new_deviceid_server)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	app->CheckDoSpecialCheckSmsLogin(account, sms_code, str_new_deviceid_server);
	return 0;
}

//======================================================
// 统一收银台支付
//======================================================
extern "C" int SDOAPI SdoBase_SetCreatePayEntranceCallback(
	SdoBaseHandle * handle,
	SdoBase_CreatePayEntranceCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SdoBase_SetCreatPayEntranceCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_DoCreatePayEntrance(
	SdoBaseHandle * handle,
	const char* szOrderId,
	const char* szProductId,
	const char* szGroupId,
	const char* szAreaId,
	const char* szExtend,
	int iSSandboxAccount,
	const char* str_new_deviceid_server)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	app->CheckDoCreatPayEntrance(szOrderId, 
		szProductId, 
		szGroupId, szAreaId, szExtend, iSSandboxAccount,str_new_deviceid_server);
	return 0;
}

//======================================================
// 统一收银台支付订单状态查询
//======================================================
extern "C" int SDOAPI SdoBase_SetCreateCheckPayOrderStatusCallback(
	SdoBaseHandle * handle,
	SdoBase_CreateCheckPayOrderStatus callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SdoBase_SetCreatCreateCheckPayOrderStatusCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_DoCreateCheckPayOrderStatus(
	SdoBaseHandle * handle, 
	const char* szOrderId, 
	int iSSandboxAccount,
	const char* str_new_deviceid_server)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	app->CheckDoCreateCheckPayOrderStatus(szOrderId, iSSandboxAccount, str_new_deviceid_server);
	return 0;
}

//======================================================
// 激活码登录
//======================================================
extern "C" int SDOAPI SdoBase_SetCreateCheckActivationCallback(SdoBaseHandle * handle, SdoBase_CreateCheckActivation callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SdoBase_SetCreatCreateCheckActivationCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_DoCreateCheckActivation(SdoBaseHandle * handle, 
	const char* activation,
	const char* str_new_deviceid_server)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	app->CheckDoCreateCheckActivation(activation, str_new_deviceid_server);
	return 0;
}

//======================================================
// 获取隐私协议相关URL
//======================================================
extern "C" int SDOAPI SdoBase_SetCreatGetAgreementUrlContentCallback(SdoBaseHandle * handle, SdoBase_CreateCheckGameAgreementUrlContentCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SetCreatGetAgreementUrlContentCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_CheckGameAgreementUrlContent(SdoBaseHandle * handle, const char* appId, const char* scene)
{
	LoginClient* app = (LoginClient*)handle;
	return app->CheckGameAgreementUrlContent(appId, scene);
}

//======================================================
// 监护人收集
//======================================================
extern "C" int SDOAPI SdoBase_SetCreatSetTutorInfoCallback(SdoBaseHandle * handle, SdoBase_CreateCheckSetTutorInfoCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SdoBase_SetCreatCheckSetTutorInfoCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_CheckSetTutorInfo(SdoBaseHandle * handle, 
	const char* idcard, 
	const char* name, 
	const char* phone, 
	const char* smscode, 
	const char* confirmKey,
	const char* str_new_deviceid_server)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->CheckSetTutorInfo(idcard, name, phone, smscode, confirmKey, str_new_deviceid_server);
}

//======================================================
// GunionWegame登录
//======================================================
extern "C" int SDOAPI SdoBase_SetCreatGhomeGunionWegameLoginCallback(
	SdoBaseHandle * handle, 
	SdoBase_CreateGhomeGunionWegameLoginCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SdoBase_SetCreatCheckGunionWegameLoginCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_CheckGhomeGunionWegameLogin(
	SdoBaseHandle * handle,
	const char* selectLoginContext,
	const char* companyId,
	const char* str_new_deviceid_server)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->CheckDoGunionWegameLogin(selectLoginContext, companyId, str_new_deviceid_server);
}

//======================================================
// Gunion获取公钥
//======================================================
extern "C" int SDOAPI SdoBase_SetCreatGunionGetPublicKeyCallback(
	SdoBaseHandle * handle,
	SdoBase_CreateGunionGetPublicKeyCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SdoBase_SetCreatCheckGunionGetPublicKeyCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_CheckGunionGetPublicKey(
	SdoBaseHandle * handle)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->CheckDoGunionGetPublicKey();
}

//======================================================
// Gunion握手
//======================================================
extern "C" int SDOAPI SdoBase_SetCreatGunionHandShakeCallback(
	SdoBaseHandle * handle,
	SdoBase_CreateGunionHandShakeCallback callback)
{
	LoginClient* app = (LoginClient*)handle;
	app->SdoBase_SetCreatCheckGunionHandShakeCallback(callback);
	return 0;
}

extern "C" int SDOAPI SdoBase_CheckGunionHandShake(
	SdoBaseHandle * handle,
	const char* publicKey)
{
	TRACET();
	LoginClient* app = (LoginClient*)handle;
	return app->CheckDoGunionHandShake(publicKey);
}
//======================================================================
// GHOME_PC C接口导出层
// 说明：
// 1. 所有 handle 实际为 LoginClient* 类型
// 2. 本层只做接口导出与类型转换
// 3. 不包含任何业务逻辑
// 4. 所有实际逻辑在 LoginClient 内部完成
//======================================================================

/*
	库名	主要解决的问题	为什么 VS2008 不需要 VS2019 却需要
	wininet.lib	旧版 HttpClient、HttpSendRequest、InternetOpen 使用它	VS2008 的 Windows SDK 自动引入，VS2019 不会自动引入
	ws2_32.lib	socket、connect、recv、getaddrinfo 等网络 API	VS2008 自动引入 Winsock，VS2019 不引入
	libcurl.lib / libcurl_imp.lib	使用 libcurl 的核心静态/导入库	改成 curl 以后必须手动加
	ole32.lib	CoInitialize、CoCreateInstance、VariantInit 等 COM API	VS2008 自动引入，VS2019 不引入
	shell32.lib	ShellExecuteW、SHGetSpecialFolderPathW 等	VS2008 自动引入，VS2019 不引入
	SafeStore.lib	自带的库，必须显式加载	这是第三方/自家库，不可能自动引入
	Advapi32.lib	GetUserNameW、注册表 RegOpenKeyExW 等	VS2008 自动引入 Advapi32，VS2019 不引入
*/

// 网络模块说明：
//   - 本工程在 Visual Studio 2019 环境下编译
//   - HTTP/HTTPS 请求使用 libcurl 实现
//   - TLS/SSL 加密使用 OpenSSL
//   - 依赖库以动态库形式提供（DLL）
//
// 依赖组件：
//   libcurl.dll
//   libssl-*.dll
//   libcrypto-*.dll
//
// 说明：
//   使用 libcurl + OpenSSL 代替 WinInet / Schannel，
//   以提高 HTTPS 兼容性并避免系统 TLS 版本限制。

// ============================================================================
// HTTP Network Layer
//
// Previous implementation:
//   WinInet
//
// Current implementation:
//   libcurl + OpenSSL
//
// Build Toolchain:
//   Visual Studio 2019
//
// Library Type:
//   Dynamic Libraries (DLL)
//
// Advantages:
//   - Avoid Windows Schannel TLS limitations
//   - Better HTTPS compatibility
//   - More detailed network error detection
//     * DNS error
//     * TCP connection error
//     * TLS handshake error
//     * HTTP timeout
//
// Error Mapping Example:
//   -2 : DNS resolve failed
//   -3 : TCP connect failed
//   -4 : TLS handshake failed
// ============================================================================