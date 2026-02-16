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
// File: ItemDialog.h
// Started by: Hattozo
// Started on: 10/27/2025
// Description: Dialog window that allows you to edit or create an item.
#pragma once
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QDateTimeEdit>
#include <QMessageBox>
#include <QComboBox>

#include <NoobWarrior/EmuDb/EmuDb.h>
#include <NoobWarrior/EmuDb/ItemType.h>

#include <memory>
#include <optional>
#include <fstream>
#include <qlineedit.h>

#include "Sdk/Sdk.h"
#include "Sdk/Project/EmuDb/EmuDbProject.h"

namespace NoobWarrior {
class ItemDialog : public QDialog {
public:
    ItemDialog(EmuDb* db, ItemType type, std::optional<int> id = std::nullopt, QWidget *parent = nullptr);

    void RegenWidgets();

    EmuDb* GetDatabase();
protected:
    void AddAssetWidgets();
    void AddAssetTypeWidgets();
    void OnSave();

    EmuDb* mDb;
    ItemType mType;

    std::optional<int> mId;

    QLabel* mIcon;

    QHBoxLayout* mLayout;
    QVBoxLayout* mSidebarLayout;
    QFormLayout* mContentLayout;

    QLineEdit* mIdInput;
    QLineEdit* mNameInput;
    QLineEdit* mDescriptionInput;
    QDateTimeEdit* mCreatedInput;
    QDateTimeEdit* mUpdatedInput;

    QComboBox* mAssetTypeInput;

    QDialogButtonBox* mButtonBox;
};
}