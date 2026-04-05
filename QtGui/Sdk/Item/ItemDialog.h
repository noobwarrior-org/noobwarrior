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
#include <QTreeView>
#include <QStandardItemModel>

#include <NoobWarrior/EmuDb/EmuDb.h>
#include <NoobWarrior/EmuDb/ItemType.h>

#include <memory>
#include <optional>
#include <fstream>
#include <qlineedit.h>

#include "Sdk/Sdk.h"
#include "Sdk/Project/EmuDb/EmuDbProject.h"
#include "Sdk/CreatorInfoWidget.h"

namespace NoobWarrior {
class ItemDialog : public QDialog {
public:
    ItemDialog(EmuDb* db, ItemType type, std::optional<int64_t> id = std::nullopt, QWidget *parent = nullptr);
protected:
    void RegenWidgets();
    void OnSave();

    void AddOwnedItemFields();

    void Asset_AddFields();
    void Asset_AddAssetTypeWidgets();
    bool Asset_OnSave();

    void User_AddFields();
    bool User_OnSave();

    EmuDb* GetDatabase();

    EmuDb* mDb;
    ItemType mType;

    std::optional<int64_t> mId;

    QLabel* mIcon;

    QHBoxLayout* mLayout;
    QVBoxLayout* mSidebarLayout;
    QFormLayout* mContentLayout;

    QLineEdit* mIdInput;
    QLineEdit* mNameInput;

    QLineEdit* mOwned_DescriptionInput;
    QDateTimeEdit* mOwned_CreatedInput;
    QDateTimeEdit* mOwned_UpdatedInput;
    CreatorInfoWidget* mOwned_CreatorInfoWidget;

    QComboBox* mAsset_AssetTypeInput;
    QTreeView* mAsset_DataView;
    QStandardItemModel* mAsset_DataModel;
    QList<QString> mAsset_DataPendingFiles;
    QList<int> mAsset_DataPendingDeleteVersions;

    QLineEdit* mUser_DisplayNameInput;
    QLineEdit* mUser_StatusInput;
    QLineEdit* mUser_BioInput;
    QDateTimeEdit* mUser_JoinDateInput;
    QDateTimeEdit* mUser_LastOnlineInput;

    QDialogButtonBox* mButtonBox;
};
}