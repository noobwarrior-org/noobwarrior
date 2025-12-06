// === noobWarrior ===
// File: PluginsPage.cpp
// Started by: Hattozo
// Started on: 12/3/2025
// Description:
#include "PluginsPage.h"

#include "../Application.h"

#include <QGroupBox>
#include <QComboBox>
#include <QLabel>

using namespace NoobWarrior;

PluginsPage::PluginsPage(QWidget *parent) : SettingsPage(parent) {
    Init();
    InitWidgets();
}

void PluginsPage::InitWidgets() {
    mGridLayout = new QGridLayout();

    mAvailableFrame = new QFrame();
    mAvailableLayout = new QVBoxLayout(mAvailableFrame);
    mAvailableLabel = new QLabel("Available");
    mAvailableList = new QListWidget();

    mSelectedFrame = new QFrame();
    mSelectedLayout = new QVBoxLayout(mSelectedFrame);
    mSelectedLabel = new QLabel("Selected");
    mSelectedList = new QListWidget();

    gApp->GetCore()->GetPluginManager()->GetPlugins();

    mGridLayout->addWidget(mAvailableFrame, 0, 0);
    mGridLayout->addWidget(mSelectedFrame, 0, 1);
    Layout->addLayout(mGridLayout);
}

const QString PluginsPage::GetTitle() {
    return "Plugins";
}

const QString PluginsPage::GetDescription() {
    return "Configure the priority of plugins, and decide whether they are enabled or not.";
}

const QIcon PluginsPage::GetIcon() {
    return QIcon(":/images/silk/plugin.png");
}
