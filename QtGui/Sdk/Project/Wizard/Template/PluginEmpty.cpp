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
// File: PluginEmpty.cpp
// Started by: Hattozo
// Started on: 2/2/2024
// Description:
#include "PluginEmpty.h"
#include "../ProjectWizard.h"

using namespace NoobWarrior;

PluginEmptyIntroPage::PluginEmptyIntroPage(QWidget *parent) : TemplatePage(parent) {
    setTitle("Setup Information");
    setSubTitle("Set your plugin's name, identifier, and other information here.");

    mMainLayout = new QVBoxLayout(this);
    mFormLayout = new QFormLayout();
    mMainLayout->addLayout(mFormLayout);

    mIdentifierEdit = new QLineEdit();
    mIdentifierEdit->setPlaceholderText("plugin@example.com");
    mFormLayout->addRow(new QLabel("Identifier"), mIdentifierEdit);

    mTitleEdit = new QLineEdit();
    mTitleEdit->setPlaceholderText("Really Cool Plugin");
    mFormLayout->addRow(new QLabel("Title"), mTitleEdit);
};

bool PluginEmptyIntroPage::isComplete() const {
    return !mIdentifierEdit->text().isEmpty() && !mTitleEdit->text().isEmpty();
}

int PluginEmptyIntroPage::nextId() const {
    return static_cast<int>(ProjectWizard::PageId::Intro);
}

QString PluginEmptyIntroPage::GetName() {
    return "Empty Plugin";
}

QString PluginEmptyIntroPage::GetDescription() {
    return "A plugin extends the functionality of noobWarrior by being able to access a powerful Lua API and modify the DataModel of any hosted server.\n\nIf you're trying to modify an existing game to add new functionality, or if you want to modify the functionality of noobWarrior itself, pick this option.";
}

QIcon PluginEmptyIntroPage::GetIcon() {
    return QIcon(":/images/plugin_96x96.png");
}
