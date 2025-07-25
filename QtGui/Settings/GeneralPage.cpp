// === noobWarrior ===
// File: GeneralPage.h
// Started by: Hattozo
// Started on: 7/24/2025
// Description:
#include "GeneralPage.h"

using namespace NoobWarrior;

GeneralPage::GeneralPage(QWidget *parent) : SettingsPage(parent) {
    Init();
}

const QString GeneralPage::GetTitle() {
    return "General";
}

const QString GeneralPage::GetDescription() {
    return "Configure the general state of the application.";
}

const QIcon GeneralPage::GetIcon() {
    return QIcon(":/images/silk/cog.png");
}
