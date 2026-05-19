#include "account_history_combo.h"

#include "account_history_model.h"

#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QFrame>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPushButton>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QTimer>
#include <QtGlobal>

#include <utility>

namespace qtlogin::sdologin {
namespace {

QString imageUrl(const QString& path)
{
    return QStringLiteral("url(\"%1\")").arg(QDir::fromNativeSeparators(path));
}

QString editStyle(const QString& normalImage, const QString& focusImage)
{
    return QStringLiteral(
        "QLineEdit{border:0;border-image:%1;color:#ffeddc;background:transparent;"
        "font:12px 'Microsoft YaHei';padding-left:10px;padding-right:30px;selection-background-color:#7b4b23;}"
        "QLineEdit:focus{border-image:%2;}"
        "QLineEdit::placeholder{color:#ffd1a4;}")
        .arg(imageUrl(normalImage), imageUrl(focusImage));
}

void setButtonImages(QPushButton* button, const QString& normal, const QString& hover, const QString& pressed)
{
    QString style = QStringLiteral("QPushButton{border:0;border-image:%1;background:transparent;}").arg(imageUrl(normal));
    if (!hover.isEmpty()) {
        style += QStringLiteral("QPushButton:hover{border-image:%1;}").arg(imageUrl(hover));
    }
    if (!pressed.isEmpty()) {
        style += QStringLiteral("QPushButton:pressed{border-image:%1;}").arg(imageUrl(pressed));
    }
    button->setStyleSheet(style);
}

}

AccountHistoryCombo::AccountHistoryCombo(Options options, QWidget* parent)
    : QWidget(parent)
    , options_(std::move(options))
{
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QStringLiteral("background:transparent;"));

    edit_ = new QLineEdit(this);
    edit_->setObjectName(QStringLiteral("accountEdit"));
    edit_->setPlaceholderText(options_.placeholder);
    edit_->setStyleSheet(editStyle(
        skinPath(QStringLiteral("combobox/combo_n.png")),
        skinPath(QStringLiteral("combobox/combo_o.png"))));

    dropButton_ = new QPushButton(this);
    dropButton_->setCursor(Qt::PointingHandCursor);
    dropButton_->setFocusPolicy(Qt::NoFocus);
    dropButton_->setStyleSheet(QStringLiteral("QPushButton{border:0;background:transparent;}"));
    connect(dropButton_, &QPushButton::clicked, this, [this]() {
        togglePopup();
    });

    layoutChildren();
}

QString AccountHistoryCombo::text() const
{
    return edit_ ? edit_->text() : QString();
}

void AccountHistoryCombo::setTextSilently(const QString& text)
{
    if (!edit_) {
        return;
    }
    const QSignalBlocker blocker(edit_);
    edit_->setText(text);
}

void AccountHistoryCombo::setAccounts(std::vector<qtlogin::config::ConfigManager::UserAccountEntry> accounts)
{
    options_.accounts = std::move(accounts);
    if (popup_ && popup_->isVisible()) {
        schedulePopupRefresh();
    }
}

void AccountHistoryCombo::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    layoutChildren();
}

bool AccountHistoryCombo::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == popup_ && event->type() == QEvent::MouseButtonPress) {
        auto* mouse = static_cast<QMouseEvent*>(event);
        if (!popup_->rect().contains(mouse->pos())) {
            hidePopup();
        }
    }
    return QWidget::eventFilter(watched, event);
}

QString AccountHistoryCombo::skinPath(const QString& relative) const
{
    return QDir(options_.skinRoot).filePath(relative);
}

QString AccountHistoryCombo::resolveSkinPath(const QString& preferred, const QString& fallback) const
{
    const QString preferredPath = skinPath(preferred);
    return QFileInfo::exists(preferredPath) ? preferredPath : skinPath(fallback);
}

void AccountHistoryCombo::layoutChildren()
{
    if (edit_) {
        edit_->setGeometry(rect());
    }
    if (dropButton_) {
        const int buttonWidth = qMax(28, width() / 5);
        dropButton_->setGeometry(width() - buttonWidth, 0, buttonWidth, height());
        dropButton_->raise();
    }
}

void AccountHistoryCombo::togglePopup()
{
    if (popup_ && popup_->isVisible()) {
        hidePopup();
        return;
    }
    showPopup();
}

void AccountHistoryCombo::showPopup()
{
    if (options_.accounts.empty()) {
        return;
    }
    rebuildPopup();
    if (!popup_) {
        return;
    }

    QWidget* host = window();
    const QPoint topLeft = mapTo(host, QPoint(0, height() - 1));
    popup_->move(topLeft);
    popup_->show();
    popup_->raise();
}

void AccountHistoryCombo::hidePopup()
{
    if (popup_) {
        popup_->hide();
    }
}

void AccountHistoryCombo::rebuildPopup()
{
    QWidget* host = window();
    if (!host) {
        return;
    }
    if (!popup_) {
        popup_ = new QFrame(host);
        popup_->setObjectName(QStringLiteral("accountHistoryPopup"));
        popup_->setAttribute(Qt::WA_StyledBackground, true);
        popup_->installEventFilter(this);
    }

    const auto children = popup_->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    for (QWidget* child : children) {
        delete child;
    }

    QFont popupFont = edit_ ? edit_->font() : font();
    if (options_.popupFontPixelSize > 0) {
        popupFont.setPixelSize(options_.popupFontPixelSize);
    }

    const QFontMetrics metrics(popupFont);
    const int rowHeight = qMax(qMax(24, height()), metrics.height() + 12);
    const int popupWidth = width();
    const int popupHeight = qMin(static_cast<int>(options_.accounts.size()) * rowHeight + 4, 10 * rowHeight + 4);
    popup_->resize(popupWidth, popupHeight);
    popup_->setFont(popupFont);
    popup_->setStyleSheet(QStringLiteral(
        "QFrame#accountHistoryPopup{border:0;border-image:%1;background:transparent;}"
        "QPushButton[class=\"accountRow\"]{border:0;color:#ffeddc;background:transparent;text-align:left;padding-left:10px;padding-right:28px;}"
        "QPushButton[class=\"accountRow\"]:hover{border-image:%2;color:#ffffff;}")
        .arg(
            imageUrl(skinPath(QStringLiteral("combobox/bg.png"))),
            imageUrl(resolveSkinPath(QStringLiteral("combobox/itemselect.png"), QStringLiteral("combobox/select_item.png")))));

    for (int i = 0; i < static_cast<int>(options_.accounts.size()); ++i) {
        const auto account = options_.accounts[static_cast<size_t>(i)];
        const int y = 2 + i * rowHeight;
        auto* row = new QPushButton(QString::fromStdWString(account.name), popup_);
        row->setProperty("class", QStringLiteral("accountRow"));
        row->setGeometry(2, y, popupWidth - 4, rowHeight);
        row->setFont(popupFont);
        row->setCursor(Qt::PointingHandCursor);
        connect(row, &QPushButton::clicked, this, [this, row]() {
            selectAccount(row->text());
        });

        auto* remove = new QPushButton(popup_);
        remove->setGeometry(popupWidth - rowHeight, y + 3, rowHeight - 6, rowHeight - 6);
        remove->setCursor(Qt::PointingHandCursor);
        remove->setFocusPolicy(Qt::NoFocus);
        setButtonImages(remove,
            resolveSkinPath(QStringLiteral("login/icon_delete_n.png"), QStringLiteral("combobox/delete_n.png")),
            resolveSkinPath(QStringLiteral("login/icon_delete_o.png"), QStringLiteral("combobox/delete_o.png")),
            resolveSkinPath(QStringLiteral("login/icon_delete_c.png"), QStringLiteral("combobox/delete_c.png")));
        remove->raise();
        connect(remove, &QPushButton::clicked, this, [this, account]() {
            removeAccount(account.name);
        });
    }
}

void AccountHistoryCombo::schedulePopupRefresh()
{
    if (!popup_ || !popup_->isVisible()) {
        return;
    }
    if (options_.accounts.empty()) {
        hidePopup();
        return;
    }
    if (popupRefreshPending_) {
        return;
    }

    popupRefreshPending_ = true;
    QTimer::singleShot(0, this, [this]() {
        popupRefreshPending_ = false;
        if (!popup_ || !popup_->isVisible()) {
            return;
        }
        if (options_.accounts.empty()) {
            hidePopup();
            return;
        }
        rebuildPopup();
    });
}

void AccountHistoryCombo::selectAccount(const QString& account)
{
    if (edit_) {
        edit_->setText(account);
        edit_->setFocus();
        edit_->selectAll();
    }
    hidePopup();
}

void AccountHistoryCombo::removeAccount(const std::wstring& account)
{
    const auto removeHandler = options_.removeHandler;
    options_.accounts = removeAccountHistoryEntry(options_.accounts, account);
    hidePopup();
    if (!removeHandler || !shouldQueueAccountHistoryRemoveFromPopupClick()) {
        return;
    }

    QTimer::singleShot(0, this, [removeHandler, account]() {
        removeHandler(account);
    });
}

}
