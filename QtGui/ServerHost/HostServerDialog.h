// === noobWarrior ===
// File: HostServerDialog.h
// Started by: Hattozo
// Started on: 11/6/2025
// Description: Dialog that allows for starting a Roblox game server
#pragma once
#include <QDialog>
#include <QHBoxLayout>
#include <QPushButton>

namespace NoobWarrior {
class HostServerDialog : public QDialog {
    Q_OBJECT
public:
    HostServerDialog(QWidget* parent = nullptr);
private:
    void InitWidgets();
    QHBoxLayout *mMainLayout;
    QPushButton *mStartServer;
};
}