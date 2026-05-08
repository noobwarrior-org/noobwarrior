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
// File: ItemOpenSaveDialog.cpp
// Started by: Hattozo
// Started on: 2/14/2026
// Description:
#include "ItemOpenSaveDialog.h"

using namespace NoobWarrior;

std::optional<int64_t> ItemOpenSaveDialog::GetOpenId(QWidget *parent, EmuDb* db, ItemType itemType, Roblox::AssetType assetType, bool enforce) {
    ItemOpenSaveDialog dialog(db, ItemOpenSaveDialog::Mode::Open, itemType, assetType, parent);
    dialog.ToggleItemTypeDropdown(!enforce);
    dialog.ToggleAssetTypeDropdown(!enforce);
    dialog.exec();
    return dialog.mSelectedId;
}

ItemOpenSaveDialog::ItemOpenSaveDialog(EmuDb* db, Mode mode, ItemType defaultItemType, Roblox::AssetType defaultAssetType, QWidget *parent) : QDialog(parent),
    mSelectedId(std::nullopt),
    mDb(db),
    mLayout(new QVBoxLayout(this)),
    mItemType(defaultItemType),
    mAssetType(defaultAssetType)
{
    setWindowTitle(mode == Mode::Open ? "Open Item" : "Save Item");
    InitWidgets();
}

void ItemOpenSaveDialog::ToggleItemTypeDropdown(bool val) {
    mItemTypeDropdown->setVisible(val);
}

void ItemOpenSaveDialog::ToggleAssetTypeDropdown(bool val) {
    mAssetTypeDropdown->setVisible(val);
}

void ItemOpenSaveDialog::InitWidgets() {
    mItemTypeDropdown = new QComboBox();
    for (int i = 0; i < ItemTypeCount; i++) {
        mItemTypeDropdown->addItem(QString::fromStdString(GetTableNameFromItemType(static_cast<ItemType>(i))));
    }

    mAssetTypeDropdown = new QComboBox();
    for (int i = 0; i < Roblox::AssetTypeCount; i++) {
        mAssetTypeDropdown->addItem(QString::fromStdString(Roblox::AssetTypeAsTranslatableString(static_cast<Roblox::AssetType>(i))));
    }

    mList = new ItemListWidget(nullptr);
    mList->SetOnDoubleClick([this](ItemWidget* item) {
        mSelectedId = item->GetId();
        close();
    });
    mList->Populate({
        .Database = mDb,
        .ItemType = mItemType,
        .AssetType = mAssetType
    });

    mLayout->addWidget(mItemTypeDropdown);
    mLayout->addWidget(mAssetTypeDropdown);
    mLayout->addWidget(mList);
}
