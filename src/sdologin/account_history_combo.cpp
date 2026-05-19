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

class AccountHistoryPopup final : public QFrame {
public:
    struct Item {
        QString text;
        std::wstring account;
    };

    explicit AccountHistoryPopup(QWidget* parent = nullptr)
        : QFrame(parent)
    {
        setMouseTracking(true);
        setAttribute(Qt::WA_StyledBackground, false);
        setStyleSheet(QStringLiteral("background:transparent;border:0;"));
    }

    void configure(
        std::vector<Item> items,
        const QString& backgroundPath,
        const QString& selectedPath,
        const QString& deleteNormalPath,
        const QString& deleteHoverPath,
        const QString& deletePressedPath,
        const QFont& popupFont,
        int rowHeight,
        std::function<void(const QString&)> selectHandler,
        std::function<void(const std::wstring&)> removeHandler)
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
                metrics.elidedText(items_[static_cast<size_t>(i)].text, Qt::ElideRight, textRect.width()));

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
        const QString selectedText = (row >= 0 && row < static_cast<int>(items_.size()))
            ? items_[static_cast<size_t>(row)].text
            : QString();
        const std::wstring selectedAccount = (row >= 0 && row < static_cast<int>(items_.size()))
            ? items_[static_cast<size_t>(row)].account
            : std::wstring();

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
        return row >= 0 && row < static_cast<int>(items_.size()) && rowRect(row).contains(pos) ? row : -1;
    }

    std::vector<Item> items_;
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
    std::function<void(const std::wstring&)> removeHandler_;
};

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

AccountHistoryCombo::~AccountHistoryCombo()
{
    if (popup_) {
        popup_->removeEventFilter(this);
        delete popup_.data();
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
        popup_ = new AccountHistoryPopup(host);
        popup_->setObjectName(QStringLiteral("accountHistoryPopup"));
        popup_->setAttribute(Qt::WA_StyledBackground, true);
        popup_->installEventFilter(this);
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

    std::vector<AccountHistoryPopup::Item> popupItems;
    popupItems.reserve(options_.accounts.size());
    for (const auto& account : options_.accounts) {
        popupItems.push_back({QString::fromStdWString(account.name), account.name});
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
        [this](const std::wstring& account) {
            removeAccount(account);
        });
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

    QPointer<AccountHistoryCombo> guard(this);
    QTimer::singleShot(0, this, [guard, removeHandler, account]() {
        if (guard) {
            removeHandler(account);
        }
    });
}

}
