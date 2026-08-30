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
    // Ids overlap between tables; a selection from a switched dropdown must not be returned.
    if (dialog.mSelectedId.has_value() && dialog.mSelectedItemType != itemType)
        return std::nullopt;
    return dialog.mSelectedId;
}

std::optional<int64_t> ItemOpenSaveDialog::GetOpenAssetId(QWidget *parent, EmuDb* db,
                                                          const std::vector<Roblox::AssetType>& allowedTypes) {
    ItemOpenSaveDialog dialog(db, ItemOpenSaveDialog::Mode::Open, ItemType::Asset,
                              Roblox::AssetType::None, parent);
    dialog.RestrictToAssetTypes(allowedTypes);
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
        mItemTypeDropdown->addItem(QString::fromStdString(GetTableNameFromItemType(static_cast<ItemType>(i))), i);
    }
    mItemTypeDropdown->setCurrentIndex(static_cast<int>(mItemType));
    connect(mItemTypeDropdown, &QComboBox::currentIndexChanged, [this](int index) {
        mItemType = static_cast<ItemType>(mItemTypeDropdown->itemData(index).toInt());
        // Asset types only mean something for assets.
        mAssetTypeDropdown->setEnabled(mItemType == ItemType::Asset);
        Repopulate();
    });

    mAssetTypeDropdown = new QComboBox();
    for (int i = 0; i < Roblox::AssetTypeCount; i++) {
        const std::string name = Roblox::AssetTypeAsTranslatableString(static_cast<Roblox::AssetType>(i));
        // The enum is gapped; non-members all stringify to "None" (only value 0 really is it).
        if (i != 0 && name == "None")
            continue;
        mAssetTypeDropdown->addItem(QString::fromStdString(name), i);
    }
    mAssetTypeDropdown->setCurrentIndex(std::max(0, mAssetTypeDropdown->findData(static_cast<int>(mAssetType))));
    mAssetTypeDropdown->setEnabled(mItemType == ItemType::Asset);
    connect(mAssetTypeDropdown, &QComboBox::currentIndexChanged, [this](int index) {
        mAssetType = static_cast<Roblox::AssetType>(mAssetTypeDropdown->itemData(index).toInt());
        Repopulate();
    });

    mList = new ItemListWidget(nullptr);
    mList->SetOnDoubleClick([this](ItemWidget* item) {
        mSelectedId = item->GetId();
        mSelectedItemType = mItemType;
        close();
    });
    Repopulate();

    mLayout->addWidget(mItemTypeDropdown);
    mLayout->addWidget(mAssetTypeDropdown);
    mLayout->addWidget(mList);
}

void ItemOpenSaveDialog::Repopulate() {
    ItemListWidget::PopulateOptions options;
    options.Database = mDb;
    options.ItemType = mItemType;
    options.AssetType = mAssetType;
    // In a restricted dialog "All" means "every allowed type", never truly everything.
    if (!mAllowedAssetTypes.empty() && mAssetType == Roblox::AssetType::None)
        options.AssetTypes = mAllowedAssetTypes;
    mList->Populate(options);
}

void ItemOpenSaveDialog::RestrictToAssetTypes(const std::vector<Roblox::AssetType>& types) {
    mAllowedAssetTypes = types;
    if (types.empty())
        return;
    mItemType = ItemType::Asset;
    ToggleItemTypeDropdown(false);

    mAssetTypeDropdown->blockSignals(true);
    mAssetTypeDropdown->clear();
    mAssetTypeDropdown->addItem("All", static_cast<int>(Roblox::AssetType::None));
    for (Roblox::AssetType type : types)
        mAssetTypeDropdown->addItem(
            QString::fromStdString(Roblox::AssetTypeAsTranslatableString(type)), static_cast<int>(type));
    mAssetTypeDropdown->setCurrentIndex(0);
    mAssetTypeDropdown->setEnabled(true);
    mAssetTypeDropdown->blockSignals(false);

    mAssetType = Roblox::AssetType::None;
    Repopulate();
}
