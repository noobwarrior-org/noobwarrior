/*
 * Copyright (C) 2026 Hattozo
 *
 * This file is part of noobWarrior.
 *
 * noobWarrior is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * noobWarrior is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with noobWarrior; if not, see
 * <https://www.gnu.org/licenses/>.
 */
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

static void SetTheme() {

}

GeneralPage::GeneralPage(QWidget *parent) : SettingsPage(parent) {
    Init();
    InitWidgets();
}

void GeneralPage::InitWidgets() {
    mWipLabel = new QLabel("Right now, this page is currently a work in progress. Expect a setting to change the language and UI in here soon.");
    mWipLabel->setWordWrap(true);
    Layout->addWidget(mWipLabel);
    /*
    auto uiBox = new QGroupBox("User Interface");
    auto uiLayout = new QFormLayout(uiBox);
    uiBox->setLayout(uiLayout);

    mLanguage = new QComboBox;
    mTheme = new QComboBox;
    mTheme->addItem("Darcula");
    mTheme->addItem("System");

    uiLayout->addRow(new QLabel("Language"), mLanguage);
    uiLayout->addRow(new QLabel("Theme"), mTheme);

    Layout->addWidget(uiBox);
    */
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

void GeneralPage::Deserialize(Registry* reg) {
    std::optional<std::string> theme = reg->GetKeyValue<std::string>("gui.theme");
    if (theme.has_value()) {

    }
}

void GeneralPage::Serialize(Registry* reg) {

}
