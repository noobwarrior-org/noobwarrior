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
// File: ProjectWizardIntroPage.cpp
// Started by: Hattozo
// Started on: 2/2/2024
// Description:
#include "ProjectWizardIntroPage.h"

using namespace NoobWarrior;

ProjectWizardIntroPage::ProjectWizardIntroPage(QWidget *parent) : QWizardPage(parent) {
    setTitle("Select Project Template");
    setSubTitle("What type of project would you like to create?");
    setPixmap(QWizard::WizardPixmap::LogoPixmap, QPixmap(":/images/paper_64x64.png"));

    mListWidget = new QListWidget();
    mListWidget->setViewMode(QListWidget::IconMode);
    mListWidget->setWordWrap(true);

    auto *description = new QTextEdit(this);
    description->setReadOnly(true);
    description->setPlainText("No template selected");

    connect(mListWidget, &QListWidget::currentItemChanged, [description, this](QListWidgetItem *current, QListWidgetItem *previous) {
        // ye ik this sucks dont judge me it works for now
        description->setPlainText(current->data(Qt::ItemDataRole::UserRole).toString());
        completeChanged();
    });

    auto *layout = new QVBoxLayout();
    layout->addWidget(mListWidget);
    layout->addWidget(description);
    setLayout(layout);
}

void ProjectWizardIntroPage::AddTemplate(ProjectWizard::PageId id, TemplatePage* page) {
    auto *dbItem = new QListWidgetItem(page->GetIcon(), page->GetName(), mListWidget);
    dbItem->setData(Qt::ItemDataRole::UserRole, page->GetDescription());
    dbItem->setData(Qt::ItemDataRole::UserRole + 1, static_cast<int>(id));
    dynamic_cast<ProjectWizard*>(wizard())->SetPage(id, page);
}

int ProjectWizardIntroPage::nextId() const {
    if (mListWidget->currentItem() == nullptr)
        return static_cast<int>(ProjectWizard::PageId::None);
    return mListWidget->currentItem()->data(Qt::ItemDataRole::UserRole + 1).toInt();
}

bool ProjectWizardIntroPage::isComplete() const {
    return mListWidget->currentItem() != nullptr;
}