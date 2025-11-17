// === noobWarrior ===
// File: HostServerDialog.cpp
// Started by: Hattozo
// Started on: 11/6/2025
// Description: Dialog that allows for starting a Roblox game server
#include "HostServerDialog.h"
#include "../Application.h"

using namespace NoobWarrior;

HostServerDialog::HostServerDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Host Server");
    InitWidgets();
}

void HostServerDialog::InitWidgets() {
    mMainLayout = new QHBoxLayout(this);
    mStartServer = new QPushButton("Start Server");
    mMainLayout->addWidget(mStartServer);

    connect(mStartServer, &QPushButton::clicked, []() {
        gApp->LaunchClient({
            .NoobWarriorVersion = 1,
            .Type = ClientType::Server,
            .Hash = "07b64feec0bd47c1",
            .Version = "0.463.0.417004"
        });
    });
}