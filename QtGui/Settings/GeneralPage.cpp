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
