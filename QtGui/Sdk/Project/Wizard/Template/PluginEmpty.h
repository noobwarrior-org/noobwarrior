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
// File: PluginEmpty.h
// Started by: Hattozo
// Started on: 2/2/2024
// Description:
#pragma once
#include "TemplatePage.h"

#include <QWizardPage>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QLabel>

namespace NoobWarrior {
class PluginEmptyIntroPage : public TemplatePage {
    Q_OBJECT
public:
    PluginEmptyIntroPage(QWidget *parent = nullptr);

    int nextId() const override;

    QString GetName() override;
    QString GetDescription() override;
    QIcon GetIcon() override;
private:
    QVBoxLayout* mMainLayout;
    QFormLayout* mFormLayout;
    QLineEdit* mTitleEdit;
};
}