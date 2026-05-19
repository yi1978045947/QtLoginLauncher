#include "virtual_keyboard_dialog.h"

#include <QDateTime>
#include <QDir>
#include <QFont>
#include <QLabel>
#include <QPainter>
#include <QShowEvent>

#include "dpi_scaler.h"
#include "virtual_keyboard_model.h"

namespace qtlogin::sdologin {
namespace {

QString fileUrl(const QString& path)
{
    return QStringLiteral("url(\"%1\")").arg(QDir::fromNativeSeparators(path));
}

QRect toQtRect(const LegacyRect& rect)
{
    return QRect(rect.x, rect.y, rect.width, rect.height);
}

QString scaleStyleSheetPixels(const QString& styleSheet, double scaleFactor)
{
    return QString::fromStdString(scaleLegacyStyleSheetPixels(styleSheet.toStdString(), scaleFactor));
}

}

VirtualKeyboardDialog::VirtualKeyboardDialog(const QString& skinRoot, QWidget* parent)
    : VirtualKeyboardDialog(skinRoot, 1.0, parent)
{
}

VirtualKeyboardDialog::VirtualKeyboardDialog(const QString& skinRoot, double legacyDpiScaleFactor, QWidget* parent)
    : QWidget(parent),
      skinRoot_(skinRoot),
      background_(skinPath(QStringLiteral("keyboard/bg.png"))),
      legacyDpiScaleFactor_(legacyDpiScaleFactor > 0.0 ? legacyDpiScaleFactor : 1.0)
{
    const LegacyRect size = legacyKeyboardWindowRect(legacyDpiScaleFactor_);
    setFixedSize(size.width, size.height);
    QFont scaledFont = font();
    scaledFont.setPixelSize(scaleLegacyPixel(12, legacyDpiScaleFactor_));
    setFont(scaledFont);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAutoFillBackground(false);
    buildUi();
}

void VirtualKeyboardDialog::setCharacterHandler(std::function<void(QChar)> handler)
{
    characterHandler_ = std::move(handler);
}

void VirtualKeyboardDialog::setBackspaceHandler(std::function<void()> handler)
{
    backspaceHandler_ = std::move(handler);
}

void VirtualKeyboardDialog::setEnterHandler(std::function<void()> handler)
{
    enterHandler_ = std::move(handler);
}

QString VirtualKeyboardDialog::skinPath(const QString& relative) const
{
    return QDir(skinRoot_).filePath(relative);
}

void VirtualKeyboardDialog::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    if (!background_.isNull()) {
        painter.drawPixmap(rect(), background_);
    } else {
        painter.fillRect(rect(), QColor(42, 25, 15, 245));
    }
}

void VirtualKeyboardDialog::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    refreshLayout();
}

void VirtualKeyboardDialog::buildUi()
{
    const QString keyNormal = skinPath(QStringLiteral("keyboard/key_n.png"));
    const QString keyHover = skinPath(QStringLiteral("keyboard/key_o.png"));
    const QString keyPressed = skinPath(QStringLiteral("keyboard/key_c.png"));

    for (size_t i = 0; i < legacyKeyboardKeys().size(); ++i) {
        auto* key = new QPushButton(this);
        key->setCursor(Qt::PointingHandCursor);
        applyButtonSkin(key, keyNormal, keyHover, keyPressed);
        keyButtons_.push_back(key);
        connect(key, &QPushButton::clicked, this, [this, key]() {
            if (characterHandler_) {
                characterHandler_(key->text().isEmpty() ? QChar() : key->text().at(0));
            }
        });
    }

    auto* backspace = new QPushButton(QStringLiteral("Back"), this);
    backspace->setGeometry(toQtRect(legacyKeyboardControlRect(LegacyKeyboardControl::Backspace, legacyDpiScaleFactor_)));
    applyButtonSkin(backspace, keyNormal, keyHover, keyPressed);
    connect(backspace, &QPushButton::clicked, this, [this]() {
        if (backspaceHandler_) {
            backspaceHandler_();
        }
    });

    auto* shift = new QPushButton(QStringLiteral("Shift"), this);
    shift->setGeometry(toQtRect(legacyKeyboardControlRect(LegacyKeyboardControl::Shift, legacyDpiScaleFactor_)));
    shift->setCheckable(true);
    applyButtonSkin(shift, keyNormal, keyHover, keyPressed);
    connect(shift, &QPushButton::clicked, this, [this, shift]() {
        shifted_ = shift->isChecked();
        refreshLayout();
    });

    auto* enter = new QPushButton(QStringLiteral("Enter"), this);
    enter->setGeometry(toQtRect(legacyKeyboardControlRect(LegacyKeyboardControl::Enter, legacyDpiScaleFactor_)));
    applyButtonSkin(enter, keyNormal, keyHover, keyPressed);
    connect(enter, &QPushButton::clicked, this, [this]() {
        if (enterHandler_) {
            enterHandler_();
        }
    });

    auto* close = new QPushButton(QStringLiteral("X"), this);
    close->setGeometry(toQtRect(legacyKeyboardControlRect(LegacyKeyboardControl::Close, legacyDpiScaleFactor_)));
    applyButtonSkin(close, keyNormal, keyHover, keyPressed);
    connect(close, &QPushButton::clicked, this, &QWidget::hide);

    auto* hint = new QLabel(QStringLiteral("建议使用键盘和软键盘混合输入密码"), this);
    hint->setGeometry(toQtRect(legacyKeyboardControlRect(LegacyKeyboardControl::Hint, legacyDpiScaleFactor_)));
    hint->setStyleSheet(scaleStyleSheetPixels(
        QStringLiteral("QLabel{color:#f6e6bf;background:transparent;font:12px 'Microsoft YaHei';}"),
        legacyDpiScaleFactor_));
}

void VirtualKeyboardDialog::applyButtonSkin(QPushButton* button, const QString& normal, const QString& hover, const QString& pressed)
{
    QString style = QStringLiteral(
        "QPushButton{border:0;border-image:%1;color:#f6e6bf;background:transparent;font:bold 12px 'Microsoft YaHei';}"
        "QPushButton:hover{border-image:%2;}"
        "QPushButton:pressed{border-image:%3;}"
        "QPushButton:checked{border-image:%3;color:#ffffff;}")
        .arg(fileUrl(normal), fileUrl(hover), fileUrl(pressed));
    button->setStyleSheet(scaleStyleSheetPixels(style, legacyDpiScaleFactor_));
}

void VirtualKeyboardDialog::refreshLayout()
{
    const auto order = randomizedLegacyKeyboardOrder(static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch()));
    const auto& keys = legacyKeyboardKeys();
    for (size_t position = 0; position < keyButtons_.size() && position < order.size(); ++position) {
        const auto& key = keys[order[position]];
        keyButtons_[position]->setGeometry(toQtRect(legacyKeyboardKeyRectForPosition(position, legacyDpiScaleFactor_)));
        keyButtons_[position]->setText(QString(QChar::fromLatin1(shifted_ ? key.shifted : key.normal)));
    }
}

}
