// === noobWarrior ===
// File: PluginDialog.h
// Started by: Hattozo
// Started on: 12/8/2025
// Description:
#pragma once
#include <QDialog>

namespace NoobWarrior {
class PluginDialog : public QDialog {
    Q_OBJECT
public:
    PluginDialog(QWidget *parent = nullptr);
    void InitWidgets();
};
}