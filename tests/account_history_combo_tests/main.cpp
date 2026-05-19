#include <cassert>
#include <vector>

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFrame>
#include <QMouseEvent>
#include <QPushButton>
#include <QWidget>

#include "account_history_combo.h"

namespace {

void sendLeftClick(QWidget* widget, const QPoint& position)
{
    QMouseEvent press(
        QEvent::MouseButtonPress,
        position,
        widget->mapToGlobal(position),
        Qt::LeftButton,
        Qt::LeftButton,
        Qt::NoModifier);
    QApplication::sendEvent(widget, &press);

    QMouseEvent release(
        QEvent::MouseButtonRelease,
        position,
        widget->mapToGlobal(position),
        Qt::LeftButton,
        Qt::NoButton,
        Qt::NoModifier);
    QApplication::sendEvent(widget, &release);
    QApplication::processEvents();
}

}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    QWidget host;
    host.resize(240, 120);

    qtlogin::sdologin::AccountHistoryCombo* combo = nullptr;
    bool removed = false;

    qtlogin::sdologin::AccountHistoryCombo::Options options;
    options.skinRoot = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("skin"));
    options.placeholder = QStringLiteral("account");
    options.popupFontPixelSize = 18;
    options.accounts = {
        {L"18070557376", 0},
    };
    options.removeHandler = [&](const std::wstring& account) {
        removed = account == L"18070557376";
        combo->setAccounts({});
    };

    combo = new qtlogin::sdologin::AccountHistoryCombo(std::move(options), &host);
    combo->setGeometry(0, 0, 177, 29);
    host.show();
    QApplication::processEvents();

    const auto buttons = combo->findChildren<QPushButton*>();
    assert(!buttons.empty());
    sendLeftClick(buttons.front(), buttons.front()->rect().center());

    auto* popup = host.findChild<QFrame*>(QStringLiteral("accountHistoryPopup"));
    assert(popup);
    assert(popup->isVisible());
    const int rowHeight = popup->height() - 4;
    const QPoint removePoint(popup->width() - rowHeight / 2, 2 + rowHeight / 2);
    sendLeftClick(popup, removePoint);
    QApplication::processEvents();

    assert(removed);
    assert(!popup->isVisible());

    delete combo;
    QApplication::processEvents();
    return 0;
}
