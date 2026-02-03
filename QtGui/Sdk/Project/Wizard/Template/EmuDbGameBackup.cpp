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
// File: EmuDbGameBackup.cpp
// Started by: Hattozo
// Started on: 2/2/2024
// Description:
#include "EmuDbGameBackup.h"
#include "TemplatePage.h"

using namespace NoobWarrior;

EmuDbGameBackupIntroPage::EmuDbGameBackupIntroPage(QWidget* parent) : TemplatePage(parent) {
    setTitle("Roblox Game");
    setSubTitle("Set your database's name, description, and other information here.");

    mMainLayout = new QVBoxLayout(this);
    mFormLayout = new QFormLayout();
    mMainLayout->addLayout(mFormLayout);

    mTitleEdit = new QLineEdit();
    mTitleEdit->setPlaceholderText("My Cool Game");
    mFormLayout->addRow(new QLabel("Title"), mTitleEdit);
}

bool EmuDbGameBackupIntroPage::isComplete() const {
    return !mTitleEdit->text().isEmpty();
}

int EmuDbGameBackupIntroPage::nextId() const {
    return -1;
}

QString EmuDbGameBackupIntroPage::GetName() {
    return "Game Backup";
}

QString EmuDbGameBackupIntroPage::GetDescription() {
    return "This template backs up your game, either online or local, and scrapes whatever it can into a database.\n\nIf you are attempting to download assets or other data, logging into a Roblox account is required. Please use a burner account if possible.";
}

QIcon EmuDbGameBackupIntroPage::GetIcon() {
    return QIcon(":/images/db_backupgame_96x96.png");
}