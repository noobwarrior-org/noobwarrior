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
// File: TemplatePage.h
// Started by: Hattozo
// Started on: 2/2/2024
// Description: QWizardPage except it 
#pragma once
#include "../ProjectWizard.h"
#include "Sdk/Project/Wizard/ProjectWizard.h"
#include <QWizardPage>

namespace NoobWarrior {
class TemplatePage : public QWizardPage {
    Q_OBJECT
public:
    TemplatePage(QWidget *parent = nullptr);

    virtual QString GetName();
    virtual QString GetDescription();
    virtual QIcon GetIcon();
};
}