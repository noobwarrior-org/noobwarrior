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
// File: NewProjectWizard.cpp
// Started by: Hattozo
// Started on: 2/2/2024
// Description:
#include "NewProjectWizard.h"

#include <QListWidget>
#include <QLabel>
#include <QTextEdit>
#include <QVBoxLayout>

using namespace NoobWarrior;

NewProjectWizard::NewProjectWizard(QWidget *parent) : QWizard(parent) {
    setWindowTitle("New Project");
    InitWidgets();
}

void NewProjectWizard::InitWidgets() {
    addPage(NewTypePage());
    addPage(CreateFilePage());
}

QWizardPage* NewProjectWizard::NewTypePage() {
    auto *page = new QWizardPage(this);
    page->setTitle("Select Project Type");
    page->setSubTitle("What type of project would you like to create?");
    page->setPixmap(WizardPixmap::LogoPixmap, QPixmap(":/images/paper_64x64.png"));

    auto *listWidget = new QListWidget();
    listWidget->setViewMode(QListWidget::IconMode);

    auto *dbItem = new QListWidgetItem(QIcon(":/images/db_96x96.png"), "Database", listWidget);
    auto *pluginItem = new QListWidgetItem(QIcon(":/images/plugin_96x96.png"), "Plugin", listWidget);
    
    auto *description = new QTextEdit(this);
    description->setReadOnly(true);

    connect(listWidget, &QListWidget::currentItemChanged, [=](QListWidgetItem *current, QListWidgetItem *previous) {
        // ye ik this sucks dont judge me it works for now
        if (current == dbItem)
            description->setPlainText("A database stores Roblox items like games and models. It is intended to be either used for retrieving Roblox assets from a server emulator for gameplay consumption, or to serve as an archive for game preservation.\n\nIf you're trying to create a game or model, pick this option.");
        else if (current == pluginItem)
            description->setPlainText("A plugin extends the functionality of noobWarrior by being able to access a powerful Lua API and modify the DataModel of any hosted server.\n\nIf you're trying to modify an existing game to add new functionality, or if you want to modify the functionality of noobWarrior itself, pick this option.");
    });

    listWidget->setCurrentItem(dbItem);

    auto *layout = new QVBoxLayout();
    layout->addWidget(listWidget);
    layout->addWidget(description);
    page->setLayout(layout);

    return page;
}

QWizardPage* NewProjectWizard::CreateFilePage() {
    auto page = new QWizardPage(this);
    page->setTitle("Choose Path");
    page->setSubTitle("Where would you like to create the file and what should its name be?");
    return page;
}