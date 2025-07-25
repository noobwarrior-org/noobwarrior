// === noobWarrior ===
// File: DatabasePage.cpp
// Started by: Hattozo
// Started on: 7/2/2025
// Description:
#include "DatabasePage.h"

using namespace NoobWarrior;

DatabasePage::DatabasePage(QWidget *parent) : SettingsPage(parent) { Init(); }

const QString DatabasePage::GetTitle() {
    return "Database";
}

const QString DatabasePage::GetDescription() {
    return "Configure the priority of databases, and decide whether they are mounted or not.";
}

const QIcon DatabasePage::GetIcon() {
    return QIcon(":/images/silk/database.png");
}
