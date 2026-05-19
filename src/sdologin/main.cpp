#include <string>

#include <windows.h>

#ifdef QTLOGIN_ENABLE_QT_UI
#include <QApplication>
#include <QDir>
#include <QTimer>
#include "dpi_scaler.h"
#include "login_window.h"
#include "protocol_acceptance_model.h"
#include "protocol_browser_launcher.h"
#include "qt_dpi_config.h"
#include "window_embed_style.h"
#endif

#include "command_line.h"
#include "config_manager.h"
#include "logger.h"
#include "named_pipe.h"
#include "process_util.h"
#include "protocol_types.h"
#include "string_util.h"

namespace {

std::string fieldOrEmpty(const qtlogin::protocol::Message& message, const char* key)
{
    const auto it = message.fields.find(key);
    return it == message.fields.end() ? std::string() : it->second;
}

qtlogin::protocol::Message makeResultMessage(uint64_t requestId, bool success)
{
    qtlogin::protocol::Message result;
    result.requestId = requestId;
    if (success) {
        result.type = qtlogin::protocol::MessageType::LoginSuccess;
        result.fields["sessionId"] = "mock-session-id";
        result.fields["sndaid"] = "mock-sndaid";
        result.fields["identityState"] = "1";
        result.fields["ticket"] = "mock-ticket";
    } else {
        result.type = qtlogin::protocol::MessageType::LoginCancel;
    }
    return result;
}

#ifdef QTLOGIN_ENABLE_QT_UI
qtlogin::sdologin::LoginWindowContext contextFromShowLogin(const qtlogin::protocol::Message& command)
{
    qtlogin::sdologin::LoginWindowContext context;
    context.appName = qtlogin::common::utf8ToWide(fieldOrEmpty(command, "appName"));
    context.appVersion = qtlogin::common::utf8ToWide(fieldOrEmpty(command, "appVer"));
    try {
        context.appId = std::stoi(fieldOrEmpty(command, "appId"));
    } catch (...) {
        context.appId = 0;
    }
    try {
        context.areaId = std::stoi(fieldOrEmpty(command, "areaId"));
    } catch (...) {
        context.areaId = 0;
    }
    try {
        context.groupId = std::stoi(fieldOrEmpty(command, "groupId"));
    } catch (...) {
        context.groupId = 0;
    }
    try {
        context.ownerHwnd = reinterpret_cast<void*>(static_cast<uintptr_t>(std::stoull(fieldOrEmpty(command, "ownerHwnd"))));
    } catch (...) {
        context.ownerHwnd = nullptr;
    }
    context.embedWindow = fieldOrEmpty(command, "embedWindow") == "1";
    try {
        context.posX = std::stoi(fieldOrEmpty(command, "posX"));
        context.posY = std::stoi(fieldOrEmpty(command, "posY"));
    } catch (...) {
        context.posX = 0;
        context.posY = 0;
    }
    try {
        const std::string scale = fieldOrEmpty(command, "legacyDpiScale");
        if (!scale.empty()) {
            context.legacyDpiScaleFactor = std::stod(scale);
        }
    } catch (...) {
        context.legacyDpiScaleFactor = 0.0;
    }
    try {
        const std::string percent = fieldOrEmpty(command, "legacyDpiPercent");
        if (!percent.empty()) {
            context.legacyDpiScaleFactor = qtlogin::sdologin::legacyDpiScaleFactorFromPercent(std::stoi(percent));
        }
    } catch (...) {
        context.legacyDpiScaleFactor = 0.0;
    }
    return context;
}

void applyConfig(qtlogin::sdologin::LoginWindowContext* context)
{
    qtlogin::config::ConfigManager config;
    if (!config.load(qtlogin::common::currentExecutableDirectory())) {
        return;
    }

    context->quickLoginEnabled = config.quickLoginEnabled();
    context->skinType = config.clientInfoValue(L"SkinType", config.value(qtlogin::config::ConfigFile::SkinType, L"SkinType"));
    if (context->appId <= 0) {
        context->appId = config.integerValue(qtlogin::config::ConfigFile::ClientInfo, L"AppID", 0);
    }
    if (context->areaId <= 0) {
        context->areaId = config.integerValue(qtlogin::config::ConfigFile::ClientInfo, L"AreaId", 0);
    }
    if (context->groupId <= 0) {
        context->groupId = config.integerValue(qtlogin::config::ConfigFile::ClientInfo, L"GroupId", 0);
    }
    if (context->appVersion.empty()) {
        context->appVersion = config.value(qtlogin::config::ConfigFile::Version, L"Version", L"value", L"1.0.0");
    }
    context->udpPassport = config.systemValue(L"UdpPassport", L"https://cas.sdo.com");
    context->tcpPassport = config.systemValue(L"TcpPassport", L"http://cas.sdo.com");
    context->verifyCert = config.clientInfoValue(L"DebugMode") == L"True";
    context->privacyPolicyUrl = config.systemValue(L"PrivacyPolicyUrl");
    context->serviceAgreementUrl = config.systemValue(L"ServicerAgreementUrl");
    context->supplementaryRulesUrl = config.systemValue(L"SupplementaryRulesUrl");
    context->daoyuHomeUrl = config.systemValue(L"DaoyuHomeUrl", L"https://www.daoyu8.com/#/");
    context->forgotPasswordUrl = config.systemValue(L"ForgotPasswordUrl", L"https://i.sdo.com/index/findPassword");
    const int savedPrivacyPolicyVersion =
        config.integerValue(qtlogin::config::ConfigFile::UserInfo, L"PrivacyPolicyVersion", -1);
    const int savedServiceAgreementVersion =
        config.integerValue(qtlogin::config::ConfigFile::UserInfo, L"ServicerAgreementVersion", -1);
    context->savedPrivacyPolicyVersion = savedPrivacyPolicyVersion;
    context->savedServiceAgreementVersion = savedServiceAgreementVersion;
    context->protocolReviewRequired = qtlogin::sdologin::shouldRequireInitialProtocolReview(savedPrivacyPolicyVersion);
    context->acceptedPrivacyPolicyVersion = -1;
    context->acceptedServiceAgreementVersion = savedServiceAgreementVersion;
    context->protocolAgreeDelaySeconds = qtlogin::sdologin::normalizedProtocolAgreeDelaySeconds(
        config.integerValue(qtlogin::config::ConfigFile::System, L"ProtocolTimer", 5));
    context->codeKeyRefreshClickIntervalMs = qtlogin::sdologin::normalizedCodeKeyRefreshClickIntervalMs(
        config.integerValue(qtlogin::config::ConfigFile::System, L"ClickInterval", 1500));
    context->accountHistoryMaxCount = config.maxAccountCount(10);
    context->accountHistory = config.userAccounts(context->accountHistoryMaxCount);
    context->fetchClientConfig = config.clientInfoValue(L"InitSwitch") != L"True";
}

qtlogin::protocol::Message makeQtResultMessage(uint64_t requestId, const qtlogin::sdologin::LoginWindowResult& uiResult)
{
    qtlogin::protocol::Message result;
    result.requestId = requestId;
    if (!uiResult.success) {
        result.type = qtlogin::protocol::MessageType::LoginCancel;
        return result;
    }

    result.type = qtlogin::protocol::MessageType::LoginSuccess;
    result.fields["sessionId"] = uiResult.sessionId;
    result.fields["sndaid"] = uiResult.sndaid;
    result.fields["identityState"] = uiResult.identityState;
    result.fields["ticket"] = uiResult.ticket;
    return result;
}

void installEmbeddedOwnerMonitor(qtlogin::sdologin::LoginWindow* window, const qtlogin::sdologin::LoginWindowContext& context)
{
    if (!window || !context.embedWindow || !context.ownerHwnd) {
        return;
    }

    HWND owner = reinterpret_cast<HWND>(context.ownerHwnd);
    auto* monitor = new QTimer(window);
    monitor->setInterval(200);
    QObject::connect(monitor, &QTimer::timeout, window, [window, owner]() {
        if (qtlogin::sdologin::shouldCloseForMissingEmbeddedOwner(true, owner, IsWindow(owner) != FALSE)) {
            qtlogin::common::logLine(L"sdologin", L"owner window disappeared, closing embedded login");
            window->close();
        }
    });
    monitor->start();
}

int runStandaloneDemo()
{
    qtlogin::sdologin::LoginWindowContext context;
    context.appName = L"QtLoginRewrite Demo";
    context.appVersion = L"1.0.0";
    context.standalone = true;
    applyConfig(&context);

    qtlogin::sdologin::LoginWindow window(context);
    window.setCompletionHandler([](const qtlogin::sdologin::LoginWindowResult&) {
        QApplication::quit();
    });
    window.show();
    window.applyEmbedding();
    return QApplication::exec();
}

int runStandaloneProtocol(const std::wstring& protocolName)
{
    qtlogin::sdologin::LoginWindowContext context;
    applyConfig(&context);

    std::wstring title = L"隐私政策";
    std::wstring url = context.privacyPolicyUrl;
    if (protocolName == L"agreement") {
        title = L"用户协议";
        url = context.serviceAgreementUrl;
    } else if (protocolName == L"rules") {
        title = L"补充规则";
        url = context.supplementaryRulesUrl;
    }

    return qtlogin::sdologin::launchProtocolBrowser(title, url) ? 0 : 8;
}
#endif

}

int main(int argc, char* argv[])
{
#ifdef QTLOGIN_ENABLE_QT_UI
    qtlogin::sdologin::configureQtLoginDpi();
#endif

    const qtlogin::sdologin::SdologinOptions options =
        qtlogin::sdologin::parseCommandLineArguments(qtlogin::sdologin::currentProcessArguments());

#ifdef QTLOGIN_ENABLE_QT_UI
    QApplication app(argc, argv);
    if (!options.standaloneUrl.empty()) {
        return qtlogin::sdologin::launchProtocolBrowser(L"协议页面", options.standaloneUrl) ? 0 : 8;
    }
    if (!options.standaloneProtocol.empty()) {
        return runStandaloneProtocol(options.standaloneProtocol);
    }
    if (options.standaloneDemo) {
        return runStandaloneDemo();
    }
#else
    (void)argc;
    (void)argv;
    if (options.standaloneDemo) {
        qtlogin::common::logLine(L"sdologin", L"standalone demo requires QTLOGIN_ENABLE_QT_UI");
        return 2;
    }
#endif

    if (options.pipeName.empty()) {
        qtlogin::common::logLine(L"sdologin", L"missing --pipe argument");
        return 2;
    }

    qtlogin::ipc::PipeConnection connection;
    qtlogin::ipc::PipeClient client;
    if (!client.connect(options.pipeName, 10000, &connection)) {
        qtlogin::common::logLine(L"sdologin", L"failed to connect sdk pipe");
        return 3;
    }

    qtlogin::protocol::Message hello;
    hello.type = qtlogin::protocol::MessageType::Hello;
    hello.fields["loginProcessId"] = std::to_string(GetCurrentProcessId());
    if (!connection.send(hello)) {
        return 4;
    }

    qtlogin::protocol::Message ack;
    if (!connection.receive(&ack) || ack.type != qtlogin::protocol::MessageType::HelloAck) {
        qtlogin::common::logLine(L"sdologin", L"missing hello ack");
        return 5;
    }

    qtlogin::protocol::Message command;
    if (!connection.receive(&command) || command.type != qtlogin::protocol::MessageType::ShowLogin) {
        qtlogin::common::logLine(L"sdologin", L"missing show login command");
        return 6;
    }

    const std::wstring appName = qtlogin::common::utf8ToWide(fieldOrEmpty(command, "appName"));
    qtlogin::common::logLine(L"sdologin", L"show login requested for " + appName);

    if (options.mockExitAfterShow) {
        qtlogin::common::logLine(L"sdologin", L"mock exit after show login command");
        return 99;
    }

#ifdef QTLOGIN_ENABLE_QT_UI
    if (options.mockSuccess && options.autoSuccessMs < 0) {
        return connection.send(makeResultMessage(command.requestId, !options.mockCancel)) ? 0 : 7;
    }

    qtlogin::sdologin::LoginWindowContext context = contextFromShowLogin(command);
    applyConfig(&context);
    qtlogin::sdologin::LoginWindow window(context);
    window.setCompletionHandler([&connection, requestId = command.requestId](const qtlogin::sdologin::LoginWindowResult& uiResult) {
        connection.send(makeQtResultMessage(requestId, uiResult));
        QApplication::quit();
    });
    window.applyEmbedding();
    window.show();
    window.applyEmbedding();
    installEmbeddedOwnerMonitor(&window, context);

    if (options.mockCancel) {
        QTimer::singleShot(options.autoSuccessMs >= 0 ? options.autoSuccessMs : 50, &window, [&window]() { window.close(); });
    } else if (options.autoSuccessMs >= 0) {
        QTimer::singleShot(options.autoSuccessMs, &window, [&window]() { window.triggerAutoSuccess(); });
    }

    return app.exec();
#else
    return connection.send(makeResultMessage(command.requestId, !options.mockCancel)) ? 0 : 7;
#endif
}
