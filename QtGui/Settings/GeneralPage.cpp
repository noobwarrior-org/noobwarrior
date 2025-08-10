// === noobWarrior ===
// File: GeneralPage.h
// Started by: Hattozo
// Started on: 7/24/2025
// Description:
#include "GeneralPage.h"

#include "../Application.h"

#include <QGroupBox>
#include <QComboBox>
#include <QLabel>

using namespace NoobWarrior;

GeneralPage::GeneralPage(QWidget *parent) : SettingsPage(parent) {
    Init();
    InitWidgets();
}

void GeneralPage::InitWidgets() {
    auto uiBox = new QGroupBox("User Interface");
    auto uiLayout = new QFormLayout(uiBox);
    uiBox->setLayout(uiLayout);

    gApp->GetCore()->GetConfig()->GetKeyValue<std::string>("language");

    uiLayout->addRow(new QLabel("Language"), new QComboBox());
    uiLayout->addRow(new QLabel("Theme"), new QComboBox());

    Layout->addWidget(uiBox);
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
