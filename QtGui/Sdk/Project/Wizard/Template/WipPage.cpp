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
// File: WipPage.cpp
// Started by: Hattozo
// Started on: 8/28/2026
// Description:
#include "WipPage.h"

using namespace NoobWarrior;

bool WipPage::isComplete() const {
    return false;
}

int WipPage::nextId() const {
    return -1;
}

QString WipPage::GetName() {
    return "Empty Plugin";
}

QString WipPage::GetDescription() {
    return "A plugin extends the functionality of noobWarrior by being able to access a powerful Lua API and modify the DataModel of any hosted server.\n\nIf you're trying to modify an existing game to add new functionality, or if you want to modify the functionality of noobWarrior itself, pick this option.";
}

QIcon WipPage::GetIcon() {
    return QIcon(":/images/plugin_96x96.png");
}

WipPage::WipPage(QWidget* parent) : TemplatePage(parent) {
    setTitle("A Work in Progress Feature");
    setSubTitle("Woah, you've encountered a WIP feature!");

    mMainLayout = new QVBoxLayout(this);
    mLabel = new QLabel("Sorry, being able to edit plugins through the SDK is currently a work in progress feature. For now, create and edit plugins through your computer's file explorer.");
    mLabel->setWordWrap(true);

    mMainLayout->addWidget(mLabel);
}