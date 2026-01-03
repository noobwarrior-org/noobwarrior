// === noobWarrior ===
// File: PluginPage.cpp
// Started by: Hattozo
// Started on: 12/3/2025
// Description:
#include "PluginPage.h"

#include "../Application.h"

#include <QGroupBox>
#include <QComboBox>
#include <QLabel>

using namespace NoobWarrior;

PluginPage::PluginPage(QWidget *parent) : SettingsPage(parent) {
    Init();
    InitWidgets();
}

void PluginPage::InitWidgets() {
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

const QString PluginPage::GetTitle() {
    return "Plugins";
}

const QString PluginPage::GetDescription() {
    return "Configure the priority of plugins, and decide whether they are enabled or not.";
}

const QIcon PluginPage::GetIcon() {
    return QIcon(":/images/silk/plugin.png");
}
