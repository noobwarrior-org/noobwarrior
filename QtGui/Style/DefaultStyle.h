// === noobWarrior ===
// File: DefaultStyle.h
// Started by: Hattozo
// Started on: 7/11/2025
// Description: My style that I like for this program.
#pragma once
#include <QProxyStyle>

namespace NoobWarrior {
class DefaultStyle : public QProxyStyle {
public:
    DefaultStyle();
    void polish(QPalette &pal) override;
    void polish(QWidget *widget) override;
};
}
