// === noobWarrior ===
// File: DatabasePage.cpp
// Started by: Hattozo
// Started on: 7/2/2025
// Description:
#include "DatabasePage.h"

#include "../Application.h"

#include <QGroupBox>
#include <QComboBox>
#include <QLabel>

using namespace NoobWarrior;

DatabasePage::DatabasePage(QWidget *parent) : SettingsPage(parent) {
    Init();
    InitWidgets();
}

void DatabasePage::InitWidgets() {
    mGridLayout = new QGridLayout();

    mAvailableFrame = new QFrame();
    mAvailableLayout = new QVBoxLayout(mAvailableFrame);
    mAvailableLabel = new QLabel("Available");
    mAvailableList = new QListWidget();

    mAvailableFrame->setAutoFillBackground(true);

    mSelectedFrame = new QFrame();
    mSelectedLayout = new QVBoxLayout(mSelectedFrame);
    mSelectedLabel = new QLabel("Selected");
    mSelectedList = new QListWidget();

    mSelectedFrame->setAutoFillBackground(true);

    gApp->GetCore()->GetPluginManager()->GetPlugins();

    mGridLayout->addWidget(mAvailableFrame, 0, 0);
    mGridLayout->addWidget(mSelectedFrame, 0, 1);
    Layout->addLayout(mGridLayout);
}

const QString DatabasePage::GetTitle() {
    return "Databases";
}

const QString DatabasePage::GetDescription() {
    return "Configure the priority of databases, and decide whether they are mounted or not.";
}

const QIcon DatabasePage::GetIcon() {
    return QIcon(":/images/silk/database.png");
}
