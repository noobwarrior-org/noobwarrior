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
