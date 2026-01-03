// === noobWarrior ===
// File: DatabaseDialog.h
// Started by: Hattozo
// Started on: 12/8/2025
// Description:
#pragma once
#include <QDialog>

namespace NoobWarrior {
class DatabaseDialog : public QDialog {
    Q_OBJECT
public:
    DatabaseDialog(QWidget *parent = nullptr);
    void InitWidgets();
};
}