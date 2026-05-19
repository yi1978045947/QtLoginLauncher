#include "account_history_combo.h"

#include "account_history_model.h"

#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QFrame>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPushButton>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QTimer>
#include <QVector>
#include <QtGlobal>

#include <functional>
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

QVector<QString> accountTextList(const std::vector<qtlogin::config::ConfigManager::UserAccountEntry>& accounts)
{
    QVector<QString> result;
    result.reserve(static_cast<int>(accounts.size()));
    for (const auto& account : accounts) {
        const QString text = QString::fromStdWString(account.name).trimmed();
        if (!text.isEmpty()) {
            result.push_back(text);
        }
    }
    return result;
}

void removeAccountText(QVector<QString>* accounts, const QString& account)
{
    if (!accounts) {
        return;
    }
    const QString normalized = account.trimmed();
    for (int i = accounts->size() - 1; i >= 0; --i) {
        if (accounts->at(i).compare(normalized, Qt::CaseInsensitive) == 0) {
            accounts->removeAt(i);
        }
    }
}

class AccountHistoryPopup final : public QFrame {
public:
    struct Item {
        QString text;
        QString account;
    };

    explicit AccountHistoryPopup(QWidget* parent = nullptr)
        : QFrame(parent)
    {
        setMouseTracking(true);
        setAttribute(Qt::WA_StyledBackground, false);
        setStyleSheet(QStringLiteral("background:transparent;border:0;"));
    }

    void configure(
        QVector<Item> items,
        const QString& backgroundPath,
        const QString& selectedPath,
        const QString& deleteNormalPath,
        const QString& deleteHoverPath,
        const QString& deletePressedPath,
        const QFont& popupFont,
        int rowHeight,
        std::function<void(const QString&)> selectHandler,
        std::function<void(const QString&)> removeHandler)
    {
        items_ = std::move(items);
        background_ = QPixmap(backgroundPath);
        selected_ = QPixmap(selectedPath);
        deleteNormal_ = QPixmap(deleteNormalPath);
        deleteHover_ = QPixmap(deleteHoverPath);
        deletePressed_ = QPixmap(deletePressedPath);
        popupFont_ = popupFont;
        rowHeight_ = qMax(20, rowHeight);
        selectHandler_ = std::move(selectHandler);
        removeHandler_ = std::move(removeHandler);
        hoveredRow_ = -1;
        pressedRow_ = -1;
        hoveredDelete_ = false;
        pressedDelete_ = false;
        update();
    }

    void detachHandlers()
    {
        selectHandler_ = nullptr;
        removeHandler_ = nullptr;
        items_.clear();
        hoveredRow_ = -1;
        pressedRow_ = -1;
        hoveredDelete_ = false;
        pressedDelete_ = false;
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        if (!background_.isNull()) {
            painter.drawPixmap(rect(), background_);
        } else {
            painter.fillRect(rect(), QColor(105, 55, 16));
        }

        painter.setFont(popupFont_);
        QFontMetrics metrics(popupFont_);
        for (int i = 0; i < static_cast<int>(items_.size()); ++i) {
            const QRect row = rowRect(i);
            const bool highlighted = i == hoveredRow_ || i == pressedRow_;
            if (highlighted) {
                if (!selected_.isNull()) {
                    painter.drawPixmap(row, selected_);
                } else {
                    painter.fillRect(row, QColor(120, 62, 18, 190));
                }
            }

            painter.setPen(highlighted ? QColor(255, 255, 255) : QColor(255, 237, 220));
            const QRect textRect = row.adjusted(10, 0, -rowHeight_, 0);
            painter.drawText(
                textRect,
                Qt::AlignVCenter | Qt::AlignLeft,
                metrics.elidedText(items_[i].text, Qt::ElideRight, textRect.width()));

            const QRect removeRect = deleteRect(i);
            const QPixmap& removeIcon = (i == pressedRow_ && pressedDelete_)
                ? deletePressed_
                : ((i == hoveredRow_ && hoveredDelete_) ? deleteHover_ : deleteNormal_);
            if (!removeIcon.isNull()) {
                painter.drawPixmap(removeRect, removeIcon);
            }
        }
    }

    void mouseMoveEvent(QMouseEvent* event) override
    {
        const int row = rowAt(event->pos());
        const bool overDelete = row >= 0 && deleteRect(row).contains(event->pos());
        if (hoveredRow_ != row || hoveredDelete_ != overDelete) {
            hoveredRow_ = row;
            hoveredDelete_ = overDelete;
            update();
        }
        setCursor(row >= 0 ? Qt::PointingHandCursor : Qt::ArrowCursor);
        QFrame::mouseMoveEvent(event);
    }

    void leaveEvent(QEvent* event) override
    {
        hoveredRow_ = -1;
        hoveredDelete_ = false;
        pressedRow_ = -1;
        pressedDelete_ = false;
        unsetCursor();
        update();
        QFrame::leaveEvent(event);
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton) {
            pressedRow_ = rowAt(event->pos());
            pressedDelete_ = pressedRow_ >= 0 && deleteRect(pressedRow_).contains(event->pos());
            update();
            event->accept();
            return;
        }
        QFrame::mousePressEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (event->button() != Qt::LeftButton) {
            QFrame::mouseReleaseEvent(event);
            return;
        }

        const int row = rowAt(event->pos());
        const bool removeClick = row >= 0 && row == pressedRow_ && pressedDelete_ && deleteRect(row).contains(event->pos());
        const bool selectClick = row >= 0 && row == pressedRow_ && !pressedDelete_;
        const QString selectedText = (row >= 0 && row < items_.size())
            ? items_[row].text
            : QString();
        const QString selectedAccount = (row >= 0 && row < items_.size())
            ? items_[row].account
            : QString();

        pressedRow_ = -1;
        pressedDelete_ = false;
        update();
        event->accept();

        if (removeClick && removeHandler_) {
            removeHandler_(selectedAccount);
        } else if (selectClick && selectHandler_) {
            selectHandler_(selectedText);
        }
    }

private:
    QRect rowRect(int row) const
    {
        return QRect(2, 2 + row * rowHeight_, qMax(0, width() - 4), rowHeight_);
    }

    QRect deleteRect(int row) const
    {
        return QRect(width() - rowHeight_, 2 + row * rowHeight_ + 3, rowHeight_ - 6, rowHeight_ - 6);
    }

    int rowAt(const QPoint& pos) const
    {
        if (pos.x() < 2 || pos.x() >= width() - 2 || pos.y() < 2) {
            return -1;
        }
        const int row = (pos.y() - 2) / rowHeight_;
        return row >= 0 && row < items_.size() && rowRect(row).contains(pos) ? row : -1;
    }

    QVector<Item> items_;
    QPixmap background_;
    QPixmap selected_;
    QPixmap deleteNormal_;
    QPixmap deleteHover_;
    QPixmap deletePressed_;
    QFont popupFont_;
    int rowHeight_ = 24;
    int hoveredRow_ = -1;
    int pressedRow_ = -1;
    bool hoveredDelete_ = false;
    bool pressedDelete_ = false;
    std::function<void(const QString&)> selectHandler_;
    std::function<void(const QString&)> removeHandler_;
};

}

AccountHistoryCombo::AccountHistoryCombo(Options options, QWidget* parent)
    : QWidget(parent)
    , skinRoot_(std::move(options.skinRoot))
    , placeholder_(std::move(options.placeholder))
    , popupFontPixelSize_(options.popupFontPixelSize)
    , accounts_(accountTextList(options.accounts))
    , removeHandler_(std::move(options.removeHandler))
{
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QStringLiteral("background:transparent;"));

    edit_ = new QLineEdit(this);
    edit_->setObjectName(QStringLiteral("accountEdit"));
    edit_->setPlaceholderText(placeholder_);
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

AccountHistoryCombo::~AccountHistoryCombo()
{
    if (popup_) {
        popup_->removeEventFilter(this);
        static_cast<AccountHistoryPopup*>(popup_.data())->detachHandlers();
        popup_->hide();
        popup_ = nullptr;
    }
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
    const QVector<QString> updatedAccounts = accountTextList(accounts);
    accounts_ = updatedAccounts;
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
    if (watched == popup_.data() && event->type() == QEvent::MouseButtonPress) {
        auto* mouse = static_cast<QMouseEvent*>(event);
        if (!popup_->rect().contains(mouse->pos())) {
            hidePopup();
        }
    }
    return QWidget::eventFilter(watched, event);
}

QString AccountHistoryCombo::skinPath(const QString& relative) const
{
    return QDir(skinRoot_).filePath(relative);
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
    if (accounts_.isEmpty()) {
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
        popup_ = new AccountHistoryPopup(host);
        popup_->setObjectName(QStringLiteral("accountHistoryPopup"));
        popup_->setAttribute(Qt::WA_StyledBackground, true);
        popup_->installEventFilter(this);
    }

    QFont popupFont = edit_ ? edit_->font() : font();
    if (popupFontPixelSize_ > 0) {
        popupFont.setPixelSize(popupFontPixelSize_);
    }

    const QFontMetrics metrics(popupFont);
    const int rowHeight = qMax(qMax(24, height()), metrics.height() + 12);
    const int popupWidth = width();
    const int popupHeight = qMin(accounts_.size() * rowHeight + 4, 10 * rowHeight + 4);
    popup_->resize(popupWidth, popupHeight);
    popup_->setFont(popupFont);

    QVector<AccountHistoryPopup::Item> popupItems;
    popupItems.reserve(accounts_.size());
    for (const QString& accountText : accounts_) {
        popupItems.push_back({accountText, accountText});
    }

    auto* historyPopup = static_cast<AccountHistoryPopup*>(popup_.data());
    historyPopup->configure(
        std::move(popupItems),
        skinPath(QStringLiteral("combobox/bg.png")),
        resolveSkinPath(QStringLiteral("combobox/itemselect.png"), QStringLiteral("combobox/select_item.png")),
        resolveSkinPath(QStringLiteral("login/icon_delete_n.png"), QStringLiteral("combobox/delete_n.png")),
        resolveSkinPath(QStringLiteral("login/icon_delete_o.png"), QStringLiteral("combobox/delete_o.png")),
        resolveSkinPath(QStringLiteral("login/icon_delete_c.png"), QStringLiteral("combobox/delete_c.png")),
        popupFont,
        rowHeight,
        [this](const QString& account) {
            selectAccount(account);
        },
        [this](const QString& account) {
            removeAccount(account);
        });
}

void AccountHistoryCombo::schedulePopupRefresh()
{
    if (!popup_ || !popup_->isVisible()) {
        return;
    }
    if (accounts_.isEmpty()) {
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
        if (accounts_.isEmpty()) {
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

void AccountHistoryCombo::removeAccount(const QString& account)
{
    const auto removeHandler = removeHandler_;
    hidePopup();
    if (!removeHandler) {
        removeAccountText(&accounts_, account);
        return;
    }
    if (!shouldQueueAccountHistoryRemoveFromPopupClick()) {
        removeHandler(account.toStdWString());
        return;
    }

    QPointer<AccountHistoryCombo> guard(this);
    QTimer::singleShot(0, this, [guard, removeHandler, account]() {
        if (guard) {
            removeHandler(account.toStdWString());
        }
    });
}

}
