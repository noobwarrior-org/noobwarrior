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
// File: EmuDbGameBackup.h
// Started by: Hattozo
// Started on: 2/2/2026
// Description: Wizard page that creates a database and immediately backs a Roblox item into it.
#pragma once
#include "TemplatePage.h"

#include <QWizardPage>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>

namespace NoobWarrior {
class EmuDbGameBackupIntroPage : public TemplatePage {
    Q_OBJECT
public:
    EmuDbGameBackupIntroPage(QWidget* parent = nullptr);

    bool validatePage() override;
    bool isComplete() const override;
    int nextId() const override;

    QString GetName() override;
    QString GetDescription() override;
    QIcon GetIcon() override;
private:
    QVBoxLayout* mMainLayout;
    QFormLayout* mFormLayout;

    QLineEdit* mTitleEdit;
    QLineEdit* mPathEdit;
    QComboBox* mItemTypeDropdown;
    QLineEdit* mItemIdEdit;
    QLabel* mLoginNoticeLabel;
};
}
