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
// File: SettingsDialog.cpp
// Started by: Hattozo
// Started on: 3/22/2025
// Description: Settings menu to configure user preferences
#include "SettingsDialog.h"
#include "../Application.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QFrame>
#include <QLineEdit>
#include <qdialogbuttonbox.h>

#include "GeneralPage.h"
#include "HttpServerPage.h"
#include "InstallationPage.h"
#include "AccountPage.h"
#include "SettingsPage.h"

// #define PAGE(name, icon) { auto *page = new QFrame(tab); auto *layout = new QVBoxLayout(page); page->setLayout(layout); tab->addTab(page, QIcon(icon), name); }
// #define INPUT(name, configProp, category, pageName) { QWidget *page = tab->findChild<QWidget*>(pageName); auto *w = new QLineEdit(page); w->setText(QString::fromStdString(configProp)); page->layout()->addWidget(w); }

using namespace NoobWarrior;

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent),
    ListWidget(nullptr),
    StackedWidget(nullptr),
    ButtonBox(nullptr)
{
    setWindowTitle("noobWarrior Settings");
    InitWidgets();
}

void SettingsDialog::InitWidgets() {
    auto *mainLayout = new QHBoxLayout(this);

    ListWidget = new QListWidget();
    ListWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    ListWidget->setFixedWidth(160);

    StackedWidget = new QStackedWidget();

    ButtonBox = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Save | QDialogButtonBox::Discard, this);

    InitPages();

    // Pages.push_back()

    // PAGE("General", ":/images/silk/cog.png")
    // PAGE("Internet", ":/images/silk/world.png")

    // INPUT("Asset Delivery Api", gApp->GetCore()->GetConfig()->Api_AssetDownload, "Roblox API URLs", "Internet")

    mainLayout->addWidget(ListWidget);
    mainLayout->addWidget(StackedWidget);
    mainLayout->addWidget(ButtonBox);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void SettingsDialog::InitPages() {
    AddPage(new GeneralPage());
    AddPage(new HttpServerPage());
    AddPage(new InstallationPage());
    AddPage(new AccountPage());
}

void SettingsDialog::AddPage(SettingsPage *page) {
    auto button = new QListWidgetItem(page->GetIcon(), page->GetTitle());
    int index = StackedWidget->addWidget(page);
    ListWidget->addItem(button);
    connect(ListWidget, &QListWidget::currentItemChanged, [this, button, index](QListWidgetItem *current, QListWidgetItem *previous) {
        if (current == button)
            StackedWidget->setCurrentIndex(index);

        // Page specific code for anything that needs to be lazily loaded.
        auto installationPage = dynamic_cast<InstallationPage*>(StackedWidget->currentWidget());
        if (installationPage != nullptr) {
            installationPage->Refresh();
        }
    });
}
