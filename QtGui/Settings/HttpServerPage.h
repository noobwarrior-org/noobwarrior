// === noobWarrior ===
// File: HttpServerPage.h
// Started by: Hattozo
// Started on: 7/24/2025
// Description:
#pragma once
#include "SettingsPage.h"

namespace NoobWarrior {
class HttpServerPage : public SettingsPage {
public:
    HttpServerPage(QWidget *parent = nullptr);
    const QString GetTitle() override;
    const QString GetDescription() override;
    const QIcon GetIcon() override;
};
}
