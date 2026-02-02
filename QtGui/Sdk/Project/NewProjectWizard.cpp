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

#include <QLabel>
#include <QVBoxLayout>

using namespace NoobWarrior;

NewProjectWizard::NewProjectWizard(QWidget *parent) : QWizard(parent) {
    setWindowTitle("Create New Project");
}

void NewProjectWizard::InitWidgets() {
    addPage(NewTypePage());
    addPage(CreateFilePage());
}

QWizardPage* NewProjectWizard::NewTypePage() {
    auto page = new QWizardPage(this);
    page->setTitle("Select Project Type");
    page->setSubTitle("What type of project would you like to create?");

    QLabel *label = new QLabel("This wizard will help you register your copy "
                               "of Super Product Two.");
    label->setWordWrap(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(label);
    page->setLayout(layout);

    return page;
}

QWizardPage* NewProjectWizard::CreateFilePage() {
    auto page = new QWizardPage(this);
    page->setTitle("Choose Path");
    page->setSubTitle("Where would you like to create the file and what should its name be?");
    return page;
}