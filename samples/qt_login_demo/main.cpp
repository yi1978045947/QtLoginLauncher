#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include <windows.h>
#include <oleauto.h>

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMetaObject>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>

#include "legacy/interface.h"

namespace {

QPlainTextEdit* gLogView = nullptr;
std::atomic_bool gLoginRunning = false;

QString wideText(LPCWSTR value)
{
    return QString::fromWCharArray(value ? value : L"");
}

QString bstrText(BSTR value)
{
    return QString::fromWCharArray(value ? value : L"");
}

void appendLog(const QString& text)
{
    if (!gLogView) {
        return;
    }
    QMetaObject::invokeMethod(gLogView, [text]() {
        gLogView->appendPlainText(text);
    }, Qt::QueuedConnection);
}

void runAsync(const QString& title, const std::function<void()>& action)
{
    appendLog(QStringLiteral(">> %1").arg(title));
    std::thread([title, action]() {
        action();
        appendLog(QStringLiteral("<< %1").arg(title));
    }).detach();
}

int CALLBACK loginCallback(int errorCode, const SDOLLoginResult* result, int userData, int)
{
    QString text = QStringLiteral("[LoginCallback] error=%1 userData=%2").arg(errorCode).arg(userData);
    if (result) {
        text += QStringLiteral("\n  SessionId=%1\n  Sndaid=%2\n  IdentityState=%3\n  Appendix=%4")
            .arg(wideText(result->SessionId))
            .arg(wideText(result->Sndaid))
            .arg(wideText(result->IdentityState))
            .arg(wideText(result->Appendix));
    }
    appendLog(text);
    return SDOL_LOGINRESULT_CLOSE;
}

BOOL CALLBACK doubleLoginCallback(int errorCode, const SDOLDoubleLoginResult* result, int userData, int)
{
    QString text = QStringLiteral("[DoubleLoginCallback] error=%1 userData=%2").arg(errorCode).arg(userData);
    if (result) {
        text += QStringLiteral("\n  SessionId=%1\n  Sndaid=%2\n  DoubleSessionId=%3")
            .arg(wideText(result->SessionId))
            .arg(wideText(result->Sndaid))
            .arg(wideText(result->szSessionIdDoubleLogin));
    }
    appendLog(text);
    return TRUE;
}

int CALLBACK lenovoPayClosedCallback()
{
    appendLog(QStringLiteral("[RegisterEvent] Lenovo pay window closed"));
    return SDOL_ERRORCODE_OK;
}

BOOL CALLBACK faceVerifyCallback(int errorCode, LPCSTR token)
{
    appendLog(QStringLiteral("[FaceVerifyCallback] error=%1 token=%2")
        .arg(errorCode)
        .arg(QString::fromLocal8Bit(token ? token : "")));
    return TRUE;
}

BOOL CALLBACK collectUserMsgCallback(int errorCode, LPCSTR message)
{
    appendLog(QStringLiteral("[CollectUserMsgCallback] error=%1 message=%2")
        .arg(errorCode)
        .arg(QString::fromLocal8Bit(message ? message : "")));
    return TRUE;
}

BOOL CALLBACK payCallback(int errorCode, const char* message)
{
    appendLog(QStringLiteral("[GhomePayCallback] error=%1 message=%2")
        .arg(errorCode)
        .arg(QString::fromLocal8Bit(message ? message : "")));
    return TRUE;
}

class SdkDemoSession {
public:
    ~SdkDemoSession()
    {
        terminal();
    }

    bool ensureInitialized(HWND owner)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (login_) {
            return true;
        }

        sdk_ = LoadLibraryW(L"sdologinsdk.dll");
        if (!sdk_) {
            appendLog(QStringLiteral("LoadLibrary(sdologinsdk.dll) failed, GetLastError=%1").arg(GetLastError()));
            return false;
        }

        initialize_ = reinterpret_cast<LPSDOLInitialize>(GetProcAddress(sdk_, "SDOLInitialize"));
        getModule_ = reinterpret_cast<LPSDOLGetModule>(GetProcAddress(sdk_, "SDOLGetModule"));
        terminal_ = reinterpret_cast<LPSDOLTerminal>(GetProcAddress(sdk_, "SDOLTerminal"));
        if (!initialize_ || !getModule_ || !terminal_) {
            appendLog(QStringLiteral("SDK exports missing: SDOLInitialize/SDOLGetModule/SDOLTerminal"));
            unloadLocked();
            return false;
        }

        SDOLAppInfo appInfo{};
        appInfo.Size = sizeof(appInfo);
        appInfo.AppID = 1;
        appInfo.AppName = L"Qt Login Demo";
        appInfo.AppVer = L"1.0.0";
        appInfo.AreaId = 1;
        appInfo.GroupId = 1;

        int code = initialize_(&appInfo);
        appendLog(QStringLiteral("SDOLInitialize => %1").arg(code));
        if (code != SDOL_ERRORCODE_OK) {
            unloadLocked();
            return false;
        }

        code = getModule_(__uuidof(ISDOLLogin7), reinterpret_cast<void**>(&login_));
        appendLog(QStringLiteral("SDOLGetModule(ISDOLLogin7) => %1").arg(code));
        if (code != SDOL_ERRORCODE_OK || !login_) {
            unloadLocked();
            return false;
        }

        appendLog(QStringLiteral("SetOwnerWindow => %1").arg(login_->SetOwnerWindow(owner)));
        appendLog(QStringLiteral("SetLoginMode(NormalLoginMode) => %1").arg(login_->SetLoginMode(NormalLoginMode)));
        appendLog(QStringLiteral("SetGameClientType(qt_login_demo) => %1").arg(login_->SetGameClientType(L"qt_login_demo")));
        appendLog(QStringLiteral("ModifyServerId(1) => %1").arg(login_->ModifyServerId("1")));
        appendLog(QStringLiteral("RegisterEvent => %1").arg(login_->RegisterEvent(&lenovoPayClosedCallback)));
        appendLog(QStringLiteral("SetDoubleLoginCallBack => %1").arg(login_->SetDoubleLoginCallBack(&doubleLoginCallback)));
        appendLog(QStringLiteral("KeyBoardMessageNotify => %1").arg(login_->KeyBoardMessageNotify(nullptr)));
        return true;
    }

    void showLogin(HWND owner)
    {
        if (gLoginRunning.exchange(true)) {
            appendLog(QStringLiteral("ShowLoginDialog ignored: login is already running"));
            return;
        }

        ISDOLLogin7* login = nullptr;
        {
            if (!ensureInitialized(owner)) {
                gLoginRunning = false;
                return;
            }
            std::lock_guard<std::mutex> lock(mutex_);
            login = login_;
        }

        const int code = login->ShowLoginDialog(&loginCallback, 2026, 0);
        appendLog(QStringLiteral("ShowLoginDialog => %1").arg(code));
        gLoginRunning = false;
    }

    void closeLoginDialog()
    {
        withLogin([](ISDOLLogin7* login) {
            appendLog(QStringLiteral("CloseLoginDialog => %1").arg(login->CloseLoginDialog()));
        });
    }

    void getTicket()
    {
        withLogin([](ISDOLLogin7* login) {
            BSTR ticket = nullptr;
            BSTR sndaid = nullptr;
            const int code = login->GetTicket(&ticket, &sndaid);
            appendLog(QStringLiteral("GetTicket => %1\n  ticket=%2\n  sndaid=%3")
                .arg(code)
                .arg(bstrText(ticket))
                .arg(bstrText(sndaid)));
            if (ticket) {
                SysFreeString(ticket);
            }
            if (sndaid) {
                SysFreeString(sndaid);
            }
        });
    }

    void getTicketForAppid(int appId)
    {
        withLogin([appId](ISDOLLogin7* login) {
            BSTR ticket = nullptr;
            BSTR sndaid = nullptr;
            const int code = login->GetTicketForAppid(&ticket, &sndaid, appId);
            appendLog(QStringLiteral("GetTicketForAppid(%1) => %2\n  ticket=%3\n  sndaid=%4")
                .arg(appId)
                .arg(code)
                .arg(bstrText(ticket))
                .arg(bstrText(sndaid)));
            if (ticket) {
                SysFreeString(ticket);
            }
            if (sndaid) {
                SysFreeString(sndaid);
            }
        });
    }

    void openBrowserWindow()
    {
        withLogin([](ISDOLLogin7* login) {
            const int code = login->OpenWindow(
                L"HTML",
                L"扫码支付",
                L"https://cas.sdo.com/cas/login?gateway=true&service=http%3a%2f%2fqrcode.sdo.com%2fqrcode%2findex%3fappId%3d1001%26areaId%3d1",
                0,
                0,
                465,
                500,
                L"center");
            appendLog(QStringLiteral("OpenWindow(扫码支付) => %1").arg(code));
        });
    }

    void openPayment()
    {
        withLogin([](ISDOLLogin7* login) {
            const char* extend =
                "{\"areaid\":\"1\",\"productid\":\"GWPAY-791000855\",\"gameorder\":\"QT-DEMO-ORDER\",\"extend\":\"test\",\"groupid\":\"1\"}";
            const int code = login->GhomePay(extend, &payCallback);
            appendLog(QStringLiteral("GhomePay(%1) => %2").arg(QString::fromLocal8Bit(extend)).arg(code));
        });
    }

    void openFaceVerify()
    {
        withLogin([](ISDOLLogin7* login) {
            const int code = login->OpenFaceVertifyDlg(&faceVerifyCallback, L"demo");
            appendLog(QStringLiteral("OpenFaceVertifyDlg => %1").arg(code));
        });
    }

    void openCollectUserMsg()
    {
        withLogin([](ISDOLLogin7* login) {
            const int code = login->OpenCollectUserMsgDlg(&collectUserMsgCallback, L"demo");
            appendLog(QStringLiteral("OpenCollectUserMsgDlg => %1").arg(code));
        });
    }

    void logout()
    {
        withLogin([](ISDOLLogin7* login) {
            appendLog(QStringLiteral("Logout => %1").arg(login->Logout()));
        });
    }

    void terminal()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (login_) {
            login_->Release();
            login_ = nullptr;
        }
        if (terminal_) {
            appendLog(QStringLiteral("SDOLTerminal => %1").arg(terminal_()));
        }
        unloadLocked();
    }

private:
    void withLogin(const std::function<void(ISDOLLogin7*)>& action)
    {
        ISDOLLogin7* login = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            login = login_;
        }
        if (!login) {
            appendLog(QStringLiteral("SDK not initialized. Click 初始化 SDK first."));
            return;
        }
        action(login);
    }

    void unloadLocked()
    {
        initialize_ = nullptr;
        getModule_ = nullptr;
        terminal_ = nullptr;
        if (sdk_) {
            FreeLibrary(sdk_);
            sdk_ = nullptr;
        }
    }

    std::mutex mutex_;
    HMODULE sdk_ = nullptr;
    LPSDOLInitialize initialize_ = nullptr;
    LPSDOLGetModule getModule_ = nullptr;
    LPSDOLTerminal terminal_ = nullptr;
    ISDOLLogin7* login_ = nullptr;
};

SdkDemoSession gSession;

QPushButton* addButton(QGridLayout* grid, int row, int column, const QString& text, QWidget* parent)
{
    auto* button = new QPushButton(text, parent);
    button->setMinimumHeight(34);
    grid->addWidget(button, row, column);
    return button;
}

}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QWidget window;
    window.setWindowTitle(QStringLiteral("QtLoginRewrite SDK Demo"));
    window.resize(760, 520);

    auto* layout = new QVBoxLayout(&window);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(12);

    auto* title = new QLabel(QStringLiteral("QtLoginRewrite SDK 接入 Demo"), &window);
    title->setStyleSheet(QStringLiteral("QLabel{font:20px 'Microsoft YaHei';font-weight:600;color:#2d2118;}"));
    layout->addWidget(title);

    auto* note = new QLabel(QStringLiteral("示例按老接入方式动态加载 sdologinsdk.dll，并调用初始化、登录、Ticket、网页/支付、实名/人脸、注销和 Terminal 等接口。"), &window);
    note->setWordWrap(true);
    note->setStyleSheet(QStringLiteral("QLabel{font:13px 'Microsoft YaHei';color:#6b5845;}"));
    layout->addWidget(note);

    auto* grid = new QGridLayout();
    grid->setHorizontalSpacing(8);
    grid->setVerticalSpacing(8);
    layout->addLayout(grid);

    auto* initButton = addButton(grid, 0, 0, QStringLiteral("初始化 SDK"), &window);
    auto* loginButton = addButton(grid, 0, 1, QStringLiteral("显示登录框"), &window);
    auto* closeButton = addButton(grid, 0, 2, QStringLiteral("关闭登录框"), &window);
    auto* ticketButton = addButton(grid, 1, 0, QStringLiteral("GetTicket"), &window);
    auto* ticketAppButton = addButton(grid, 1, 1, QStringLiteral("GetTicketForAppid"), &window);
    auto* logoutButton = addButton(grid, 1, 2, QStringLiteral("Logout"), &window);
    auto* browserButton = addButton(grid, 2, 0, QStringLiteral("OpenWindow 网页"), &window);
    auto* payButton = addButton(grid, 2, 1, QStringLiteral("GhomePay 支付"), &window);
    auto* previewButton = addButton(grid, 2, 2, QStringLiteral("直接预览 sdologin"), &window);
    auto* faceButton = addButton(grid, 3, 0, QStringLiteral("人脸核验回调"), &window);
    auto* collectButton = addButton(grid, 3, 1, QStringLiteral("实名资料回调"), &window);
    auto* terminalButton = addButton(grid, 3, 2, QStringLiteral("SDOLTerminal"), &window);

    gLogView = new QPlainTextEdit(&window);
    gLogView->setReadOnly(true);
    gLogView->setMinimumHeight(250);
    gLogView->setStyleSheet(QStringLiteral(
        "QPlainTextEdit{background:#fffdf8;border:1px solid #dfcdb5;color:#3b2a1c;"
        "font:13px Consolas,'Microsoft YaHei';padding:10px;}"));
    layout->addWidget(gLogView, 1);
    appendLog(QStringLiteral("等待操作。"));

    QObject::connect(initButton, &QPushButton::clicked, &window, [&window]() {
        HWND owner = reinterpret_cast<HWND>(window.winId());
        runAsync(QStringLiteral("初始化 SDK"), [owner]() {
            gSession.ensureInitialized(owner);
        });
    });

    QObject::connect(loginButton, &QPushButton::clicked, &window, [&window]() {
        HWND owner = reinterpret_cast<HWND>(window.winId());
        runAsync(QStringLiteral("ShowLoginDialog"), [owner]() {
            gSession.showLogin(owner);
        });
    });

    QObject::connect(closeButton, &QPushButton::clicked, &window, []() {
        runAsync(QStringLiteral("CloseLoginDialog"), []() {
            gSession.closeLoginDialog();
        });
    });

    QObject::connect(ticketButton, &QPushButton::clicked, &window, []() {
        runAsync(QStringLiteral("GetTicket"), []() {
            gSession.getTicket();
        });
    });

    QObject::connect(ticketAppButton, &QPushButton::clicked, &window, []() {
        runAsync(QStringLiteral("GetTicketForAppid"), []() {
            gSession.getTicketForAppid(1);
        });
    });

    QObject::connect(logoutButton, &QPushButton::clicked, &window, []() {
        runAsync(QStringLiteral("Logout"), []() {
            gSession.logout();
        });
    });

    QObject::connect(browserButton, &QPushButton::clicked, &window, []() {
        runAsync(QStringLiteral("OpenWindow"), []() {
            gSession.openBrowserWindow();
        });
    });

    QObject::connect(payButton, &QPushButton::clicked, &window, []() {
        runAsync(QStringLiteral("GhomePay"), []() {
            gSession.openPayment();
        });
    });

    QObject::connect(faceButton, &QPushButton::clicked, &window, []() {
        runAsync(QStringLiteral("OpenFaceVertifyDlg"), []() {
            gSession.openFaceVerify();
        });
    });

    QObject::connect(collectButton, &QPushButton::clicked, &window, []() {
        runAsync(QStringLiteral("OpenCollectUserMsgDlg"), []() {
            gSession.openCollectUserMsg();
        });
    });

    QObject::connect(terminalButton, &QPushButton::clicked, &window, []() {
        runAsync(QStringLiteral("SDOLTerminal"), []() {
            gSession.terminal();
        });
    });

    QObject::connect(previewButton, &QPushButton::clicked, &window, []() {
        const QString exePath = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("sdologin.exe"));
        QProcess::startDetached(exePath, QStringList() << QStringLiteral("--standalone-demo"));
    });

    QObject::connect(&app, &QCoreApplication::aboutToQuit, []() {
        gSession.terminal();
    });

    window.show();
    return app.exec();
}
