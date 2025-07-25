// === noobWarrior ===
// File: HttpServerPage.cpp
// Started by: Hattozo
// Started on: 7/24/2025
// Description:
#include "HttpServerPage.h"

using namespace NoobWarrior;

HttpServerPage::HttpServerPage(QWidget *parent) : SettingsPage(parent) {
    Init();
}

const QString HttpServerPage::GetTitle() {
    return "HTTP Server";
}

const QString HttpServerPage::GetDescription() {
    return "Anything that is related to your HTTP server (and only your HTTP server) goes in here.";
}

const QIcon HttpServerPage::GetIcon() {
    return QIcon(":/images/silk/drive_web.png");
}

