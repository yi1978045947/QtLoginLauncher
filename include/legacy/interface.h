#ifndef SDOLOGIN_INTERFACE_H
#define SDOLOGIN_INTERFACE_H


#include <objbase.h>
#include <windows.h>

#pragma pack(push,1)

#define SDOL_ERRORCODE_OK					0      // 成功
#define SDOL_ERRORCODE_FAILED				-1     // 失败
#define SDOL_ERRORCODE_UNEXCEPT				-100   // 未知异常
#define SDOL_ERRORCODE_LOGINCANCEL			-101   // 用户按取消关闭了登录对话框
#define SDOL_ERRORCODE_INVALIDPARAM			-102   // 参数无效
#define SDOL_ERRORCODE_INVALIDBUFFER		-103   // 缓冲区大小不足
#define SDOL_ERRORCODE_GETTICKET_TIMEOUT	-104   // GetTicket超时
#define SDOL_ERRORCODE_NOT_SUPPORT          -105   // 不支持该操作

// COM服务相关
#define SDOL_ERRORCODE_COM_CREATE_ERROR          -110   // COM创建失败
#define SDOL_ERRORCODE_CONNECT_CREAT_ERROR       -111   // COM连接点创建失败

#define SDOL_LOGINRESULT_KEEP_SHOWN     0      // 登录窗口继续显示
#define SDOL_LOGINRESULT_CLOSE          1      // 关闭登录窗口

#define 	NormalLoginMode				0
#define 	LauncherLoginMode			1
#define		AttachToLoginMode			2

#define     UPDATE_ACTION_DOWNLOAD      1
#define     UPDATE_ACTION_SETUP         2

struct SDOLAppInfo
{
	DWORD		Size;			// = sizeof(SDOLAppInfo)
	int			AppID;			// 盛趣统一游戏ID
	LPCWSTR		AppName;		// 游戏名称
	LPCWSTR		AppVer;		   	// 游戏版本
	int			AreaId;			// 游戏区ID	(注意：不可用时传-1)
	int			GroupId;        // 游戏组ID	(注意：不可用时传-1)
};

struct SDOLLoginResult
{
	DWORD       Size;			// sizeof(SDOLLoginResult)，为以后扩充提供可能
	LPCWSTR     SessionId;		// 用于后台验证的token串
	LPCWSTR     Sndaid;		    // 用户ID
	LPCWSTR     IdentityState;	// 是否成年标识，0未成年，1成年
	LPCWSTR     Appendix;		// 附加字段，保留
};

struct SDOLUpdateReport
{
	DWORD       Size;				// sizeof(SDOLUpdateReport)，为以后扩充提供可能
	LPCWSTR     LocalVersion;		// 用于后台验证的token串
	LPCWSTR     ServerVersion;		// 用户ID
	LPCWSTR     CurrentFile;		// 当前正在下载或者安装的补丁包名称
	int         CurrentProgress;	// 当前补丁包下载或者安装的进度（有效值范围0~100）
	int         TotalProgress;		// 总体更新进度（有效值范围0~100）
	int         Action;				// 当前正在执行的操作，UPDATE_ACTION_DOWNLOAD（正在下载补丁），UPDATE_ACTION_SETUP（正在安装补丁）
};

struct SDOLDoubleLoginResult
{
	DWORD       Size;			// sizeof(SDOLLoginResult)，为以后扩充提供可能
	LPCWSTR     SessionId;		// 用于后台验证的token串
	LPCWSTR     Sndaid;		    // 用户ID
	LPCWSTR     IdentityState;	// 是否成年标识，0未成年，1成年
	LPCWSTR     Appendix;		// 附加字段，保留
	LPCWSTR		szSessionIdDoubleLogin;		// 用于后台验证的token串
};

struct GhomeSdoChannelInfo
{
	LPCSTR szApplicationChannel;  //一级渠道号
	LPCSTR  szChannelCode;        //二级渠道号
	LPCSTR  szAdtraceId;          //买量上报Id
};

// 登录回调函数
// 返回 SDOL_LOGINRESULT_KEEP_SHOWN 登录窗口继续显示，返回 SDOL_LOGINRESULT_CLOSE 关闭登录窗口
// nErrorCode:     登录返回错误代码
// pLoginResult:   登录返回结果，返回成功时才有效
// nUserData:      在调用ShowLoginDialog时传入的用户数据
// nReserved:      保留，暂不使用，目前总是为0(不等于ShowLoginDialog中传入的Reserved参数)
typedef int (CALLBACK *LPSDOLLOGINCALLBACKPROC)(int nErrorCode, const SDOLLoginResult* pLoginResult, int nUserData, int nReserved);

/*Lenovo支付窗口关闭回调

*/
typedef int (CALLBACK *LPLENOVOPAYWINDOWCLOSEDCALLBACKPROC)();

// 2025.04.08
// 扩展功能，LoginResultDoubleLogin结构里面的参数增加第二个sessionId
typedef BOOL (CALLBACK *LPSDOLDOUBLELOGINCALLBACKPROC)(int nErrorCode, const SDOLDoubleLoginResult* pLoginResult, int nUserData, int nReserved);

MIDL_INTERFACE("AD887932-2D1C-48ec-B3E0-535B609C12D6")
ISDOLLogin : public IUnknown
{
public:
	// 设置游戏窗口
	STDMETHOD_(int,SetOwnerWindow)(THIS_ HWND hWnd) PURE;
	// 显示登录窗口
	STDMETHOD_(int,ShowLoginDialog)(THIS_ LPSDOLLOGINCALLBACKPROC fnCallback, int nUserData, int nReserved) PURE;
	// 关闭登录窗口
	STDMETHOD_(int,CloseLoginDialog)(THIS) PURE;
	// 移动登陆窗口到指定位置（嵌入方式时有效，x,y坐标是相对于owner window的坐标）
	STDMETHOD_(int,MoveLoginDialog)(THIS_ int x, int y) PURE;
	// 注销登陆
	STDMETHOD_(int,Logout)(THIS) PURE;
};

MIDL_INTERFACE("7B06DAD6-6832-4455-AFC6-6C8BE902534B")
ISDOLLogin2 : public ISDOLLogin
{
public:
	// 调用登陆功能（等同于点击登陆界面的登陆按钮）
	STDMETHOD_(int,DoLogin)(THIS) PURE;
	// 设置LoginMode（用于区分是Launcher还是游戏启动了登录器）
	STDMETHOD_(int,SetLoginMode)(THIS_ int nLoginMode) PURE;
	// 获取Ticket
	STDMETHOD_(int,GetTicket)(THIS_ BSTR* bstrTicket, BSTR* bstrSndaid) PURE;
	// 当AppInfo中的信息发生变化是可以通过该接口修改AppInfo
	STDMETHOD_(int,ModifyAppInfo)(THIS_ const SDOLAppInfo* pAppInfo) PURE;
};

MIDL_INTERFACE("D09EE9A2-8C44-42d6-9FE2-E34727BBEA10")
ISDOLLogin3 : public ISDOLLogin2
{
public:
	STDMETHOD_(int,RltLogin)(THIS_ BSTR bstrVkey) PURE;
	STDMETHOD_(int,LoginFeedback)(THIS_ LPCWSTR sessionId, int result, int errorCode) PURE;
};

MIDL_INTERFACE("1A325A91-B5C3-4b7d-B912-4FF08B38B08F")
ISDOLLogin4 : public ISDOLLogin3
{
public:
	STDMETHOD_(int,OpenWindow)(THIS_ LPCWSTR pwcsWinType, LPCWSTR pwcsWinName, LPCWSTR pwcsSrc, int nLeft, int nTop, int nWidth, int nHeight, LPCWSTR pwcsMode) PURE;

};

MIDL_INTERFACE("A418995E-1F86-4601-B638-488173458B09")
ISDOLLogin5 : public ISDOLLogin4
{
public:
	STDMETHOD_(int,GetLoginedCompanyId)() PURE;
};

MIDL_INTERFACE("0FD51705-7360-43f1-92A9-C16895866A83")
ISDOLLogin6 : public ISDOLLogin5
{
public:

	/** 注册回调函数
	@param [in] fnCallback 支付回调函数
    @return 0表示成功  其他值表示失败
    */
	STDMETHOD_(int,RegisterEvent)(LPLENOVOPAYWINDOWCLOSEDCALLBACKPROC fnCallback) PURE;

	/** 打开联想渠道登录框
	@param [in] fnCallback	登录结果回调
	@param [in] nUserData	
	@param [in] nReserved	
    @return 0表示成功  其他值表示失败
    */
	STDMETHOD_(int,ShowLenovoLoginDlg)(THIS_ LPSDOLLOGINCALLBACKPROC fnCallback, int nUserData, int nReserved) PURE;

	/** 显示Lenovo支付窗口（异步 SetOwnerWindow 设置父窗口）
	@param [in] szOrderId	订单id
	@param [in] szProductId	商品id
	@param [in] szGroupId	组id
	@param [in] szAreaId	区id
	@param [in] szRoleId	角色id
	@param [in] szExtend	扩展字段，自定义结构
    @return 0表示成功  其他值表示失败
    */
	STDMETHOD_(int,AsyncShowLenovoPayWindow)(LPCWSTR szOrderId,LPCWSTR szProductId,LPCWSTR szGroupId,LPCWSTR szAreaId,
		LPCWSTR szRoleId,LPCWSTR szExtend) PURE;
};

// 授权码验证回调函数
// nErrorCode:     返回错误代码，0表示授权码验证正常，非0表示授权验证失败
// ticket    :     授权码验证返回的ticket
typedef BOOL (CALLBACK *LPLOGINAUTHCODECALLBACKPROC)(int nErrorCode,LPCSTR ticket);

// 按下Esc捕获消息通知给调用方
// nErrorCode:     返回错误代码，1表示按下，0表示没有按下
typedef BOOL (CALLBACK *LPLOGINESCNOTIFYCALLBACKPROC)(int nErrorCode);

//人脸打开逻辑
// nErrorCode:     返回错误代码，0表示打开正常，非0表示打开不正常
// token    :      人脸验证结果
typedef BOOL (CALLBACK *LPFACEVERTIFYCALLBACKPROC)(int nErrorCode,LPCSTR token);


//收集用户信息
// nErrorCode:     返回错误代码，0表示打开正常，非0表示打开不正常
// nResultMsg    : 收集信息内容
typedef BOOL (CALLBACK *LPCOLLECTUSERMSGCALLBACKPROC)(int nErrorCode,LPCSTR nResultMsg);

//统一收银台支付回调函数
// pay_state:     返回支付状态，0表示没有支付，1并且msg为已经支付代表支付成功
// msg            返回支付信息，msg为空的话表示接口调用成功，不为空的话会有错误信息
typedef BOOL (CALLBACK *LPLOGINPAYCALLBACKPROC)(int nErrorCode,const char* nErrormsg);

//返回一级/二级渠道号/广告id
// nErrorCode:          返回错误码，0表示正常
// pGhomeChannelInfo:   返回的GhomeChannelInfo内容
//typedef BOOL (CALLBACK *LPLOGINGETCHANNELCODECALLBACKPROC)(int nErrorCode,const GhomeSdoChannelInfo* pGhomeChannelInfo);

MIDL_INTERFACE("9419A37F-5879-482b-875D-593DA2E3B579")
ISDOLLogin7 : public ISDOLLogin6
{
public:
	/** 设置登录窗状态
	@param [in] nState	1 显示； 2 隐藏
    @return 0表示成功  其他值表示失败
    */
	STDMETHOD_(int,SetLoginDialogState)(int nState) PURE;

	/** 设置渠道参数
	@param [in] szGameClientType 渠道
    @return 0表示成功  其他值表示失败
    */
	STDMETHOD_(int,SetGameClientType)(LPCWSTR szGameClientType) PURE;

	//游戏内打开活动窗口界面
	STDMETHOD_(int,OpenActivityWindow)(THIS_ LPCWSTR pwcsWinType, LPCWSTR pwcsWinName, LPCWSTR pwcsSrc, int nLeft, int nTop, int nWidth, int nHeight, LPCWSTR pwcsMode) PURE;

	STDMETHOD_(int,AuthCodeLogin)(THIS_ LPLOGINAUTHCODECALLBACKPROC fnCallback,LPCSTR authCode) PURE;

	STDMETHOD_(int,OpenXinIeWindow)(THIS_ LPCWSTR pwcsWinType, LPCWSTR pwcsWinName, LPCWSTR pwcsSrc, int nLeft, int nTop, int nWidth, int nHeight, LPCWSTR pwcsMode) PURE;

	STDMETHOD_(int,OpenXinYouLoginIeAgain)() PURE;

	STDMETHOD_(int,XinYouLoginIeCenter)() PURE;

	//按下ESC捕获消息通知给调用方
	STDMETHOD_(int,KeyBoardMessageNotify)(THIS_ LPLOGINESCNOTIFYCALLBACKPROC fnCallback) PURE;

	//泡泡堂:2P登录回调
	STDMETHOD_(int,SetDoubleLoginCallBack)(LPSDOLDOUBLELOGINCALLBACKPROC fnCallback) PURE;

	/** 修改serverId
	@param [in] serverId 服务器id,这个id对应心游登录网页serverid以及支付网页server两个id，如果不修改默认为1
    @return 0表示成功  其他值表示失败
    */
	STDMETHOD_(int,ModifyServerId)(LPCSTR serverId) PURE;

	//打开人脸识别验证
	STDMETHOD_(int,OpenFaceVertifyDlg)(LPFACEVERTIFYCALLBACKPROC fnCallback, LPCWSTR szVertifyType) PURE;

	//收集用户信息
	STDMETHOD_(int,OpenCollectUserMsgDlg)(THIS_ LPCOLLECTUSERMSGCALLBACKPROC fnCallback, LPCWSTR szCollectMsgType) PURE;

	////获取ticket，通过appId入参获取相对应的ticket
	STDMETHOD_(int,GetTicketForAppid)(THIS_ BSTR* bstrTicket, BSTR* bstrSndaid,int appId) PURE;


	/** 显示统一收银台，适用手游支付方式
    */
	STDMETHOD_(int,GhomePay)(THIS_ const char* extend,LPLOGINPAYCALLBACKPROC fnCallback) PURE;

	/** 获取一级渠道号/二级渠道号/广告id
    */
	//STDMETHOD_(int,GhomeGetCPSChannelInfo)(THIS_ LPLOGINGETCHANNELCODECALLBACKPROC fnCallback) PURE;
};


// 事件回调函数，当用户操作时，或者登录器需要更新程序执行操某个作时，会调用该回调函数。
// 执行成功时返回 SDOL_ERRORCODE_OK，如果不支持某个操作应该返回 SDOL_ERRORCODE_NOT_SUPPORT
// 执行某个操作失败时返回错误码。

// strCommand:     更新程序需要支持的命令，参数格式和命令行参数类似，登录器会发出如下命令，
//                 "Start" "Stop" "Pause" "Quit"
// nUserData:      在调用SetEventCallback时传入的用户数据
// nReserved:      保留，暂不使用，目前总是为0(不等于SetEventCallback中传入的Reserved参数)
typedef int (CALLBACK *LPSDOLUPDATEREVENTCALLBACK)(LPCWSTR strCommand, int nUserData, int nReserved);

MIDL_INTERFACE("94C8623A-446B-41c7-A369-10E016EFB3DA")
ISDOLUpdater : public IUnknown
{
public:
	STDMETHOD_(int,SetEventCallback)(THIS_ LPSDOLUPDATEREVENTCALLBACK fnCallback, int nUserData, int nReserved) PURE;
	STDMETHOD_(int,UpdateReport)(THIS_ const SDOLUpdateReport* pReport) PURE;
	STDMETHOD_(int,QuitLauncher)(THIS_) PURE;
};

typedef int (WINAPI* LPSDOLInitialize)(const SDOLAppInfo* pAppInfo);
typedef int (WINAPI* LPSDOLGetModule)(REFIID riid, void** intf);
typedef int (WINAPI* LPSDOLTerminal)();

#pragma pack(pop)
#endif /* SDOLOGIN_INTERFACE_H */
