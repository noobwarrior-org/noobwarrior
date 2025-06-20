// === noobWarrior ===
// File: Settings.h
// Started by: Hattozo
// Started on: 3/22/2025
// Description:
#pragma once

#include <NoobWarrior/Config.h>

#include <QDialog>

namespace NoobWarrior {
class Settings : public QDialog {
    Q_OBJECT
public:
    Settings(QWidget *parent = nullptr);
};
}