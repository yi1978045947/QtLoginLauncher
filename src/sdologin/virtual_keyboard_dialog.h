#pragma once

#include <functional>
#include <vector>

#include <QPixmap>
#include <QPushButton>
#include <QWidget>

namespace qtlogin::sdologin {

class VirtualKeyboardDialog final : public QWidget {
public:
    explicit VirtualKeyboardDialog(const QString& skinRoot, QWidget* parent = nullptr);
    VirtualKeyboardDialog(const QString& skinRoot, double legacyDpiScaleFactor, QWidget* parent = nullptr);

    void setCharacterHandler(std::function<void(QChar)> handler);
    void setBackspaceHandler(std::function<void()> handler);
    void setEnterHandler(std::function<void()> handler);
    void refreshLayout();

protected:
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    QString skinPath(const QString& relative) const;
    void buildUi();
    void applyButtonSkin(QPushButton* button, const QString& normal, const QString& hover, const QString& pressed);

    QString skinRoot_;
    QPixmap background_;
    std::vector<QPushButton*> keyButtons_;
    double legacyDpiScaleFactor_ = 1.0;
    bool shifted_ = false;
    std::function<void(QChar)> characterHandler_;
    std::function<void()> backspaceHandler_;
    std::function<void()> enterHandler_;
};

}
