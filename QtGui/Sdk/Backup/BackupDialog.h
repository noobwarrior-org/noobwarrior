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
// File: BackupDialog.h
// Started by: Hattozo
// Started on: 9/2/2025
// Description:
#include <NoobWarrior/Backup.h>
#include <NoobWarrior/EmuDb/ItemType.h>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QSizePolicy>

namespace NoobWarrior {
class BackupDialog : public QDialog {
public:
    BackupDialog(QWidget *parent = nullptr);
protected:
    void InitWidgets();
    void UpdateWidgets();

    bool StartBackup();
    EmuDb* GetDatabase();
private:
    // Local-file backup is removed for the time being, so every backup is an online item.
    Backup::ItemSource mSource;
    ItemType mItemType;

    QVBoxLayout* mMainLayout;

    QLabel* mItemTypeCaption;
    QComboBox* mItemTypeDropdown;

    QLabel* mIdCaption;
    QLineEdit* mIdField;

    QDialogButtonBox* mButtons;
};
}