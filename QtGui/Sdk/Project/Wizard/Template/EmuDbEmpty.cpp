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
// File: EmuDbEmpty.cpp
// Started by: Hattozo
// Started on: 2/2/2024
// Description:
#include "EmuDbEmpty.h"
#include "TemplatePage.h"

using namespace NoobWarrior;

EmuDbEmptyIntroPage::EmuDbEmptyIntroPage(QWidget* parent) : TemplatePage(parent) {
    setTitle("Setup Information");
    setSubTitle("Set your database's name, description, and other information here.");

    mMainLayout = new QVBoxLayout(this);
    mFormLayout = new QFormLayout();
    mMainLayout->addLayout(mFormLayout);

    mTitleEdit = new QLineEdit();
    mTitleEdit->setPlaceholderText("My Cool Game");
    mFormLayout->addRow(new QLabel("Title"), mTitleEdit);
}

bool EmuDbEmptyIntroPage::isComplete() const {
    return !mTitleEdit->text().isEmpty();
}

int EmuDbEmptyIntroPage::nextId() const {
    return -1;
}

QString EmuDbEmptyIntroPage::GetName() {
    return "Empty Database";
}

QString EmuDbEmptyIntroPage::GetDescription() {
    return "A database stores Roblox items like games and models. It is intended to be either used for retrieving Roblox assets from a server emulator for gameplay consumption, or to serve as an archive for game preservation.\n\nIf you're trying to create a game or model, pick this option.";
}

QIcon EmuDbEmptyIntroPage::GetIcon() {
    return QIcon(":/images/db_96x96.png");
}