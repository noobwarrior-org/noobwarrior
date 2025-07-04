// === noobWarrior ===
// File: SettingsDialog.h
// Started by: Hattozo
// Started on: 3/22/2025
// Description:
#pragma once

#include <NoobWarrior/Config.h>

#include <QDialog>

namespace NoobWarrior {
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    SettingsDialog(QWidget *parent = nullptr);
};
}