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
#include "ProjectWizard.h"
#include "Sdk/Project/Wizard/ProjectWizardIntroPage.h"
#include "Sdk/Project/Wizard/Template/EmuDbEmpty.h"
#include "Sdk/Project/Wizard/Template/EmuDbGameBackup.h"
#include "Sdk/Project/Wizard/Template/PluginEmpty.h"

#include <QListWidget>
#include <QLabel>
#include <QTextEdit>
#include <QVBoxLayout>
#include <qnamespace.h>

using namespace NoobWarrior;

ProjectWizard::ProjectWizard(QWidget *parent) : QWizard(parent) {
    setWindowTitle("New Project");
    InitWidgets();
}

void ProjectWizard::InitWidgets() {
    auto *intro = new ProjectWizardIntroPage();
    SetPage(PageId::Intro, intro);
    intro->AddTemplate(PageId::EmuDbEmptyIntro, new EmuDbEmptyIntroPage);
    intro->AddTemplate(PageId::EmuDbGameBackupIntro, new EmuDbGameBackupIntroPage);
    intro->AddTemplate(PageId::PluginEmptyIntro, new PluginEmptyIntroPage);
}

void ProjectWizard::SetPage(ProjectWizard::PageId id, QWizardPage* page) {
    setPage(static_cast<int>(id), page);
}
