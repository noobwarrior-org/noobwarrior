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

int ItemOpenSaveDialog::GetOpenId(QWidget *parent, EmuDb* db, ItemType itemType, Roblox::AssetType assetType, bool enforce) {
    ItemOpenSaveDialog dialog(db, ItemOpenSaveDialog::Mode::Open, parent);
    dialog.ToggleItemTypeDropdown(!enforce);
    dialog.ToggleAssetTypeDropdown(!enforce);
    dialog.exec();
    return 0;
}

ItemOpenSaveDialog::ItemOpenSaveDialog(EmuDb* db, Mode mode, QWidget *parent) : QDialog(parent),
    mDb(db),
    mLayout(new QVBoxLayout(this))
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

    mList = new ItemListWidget(mDb);

    mLayout->addWidget(mItemTypeDropdown);
    mLayout->addWidget(mAssetTypeDropdown);
}