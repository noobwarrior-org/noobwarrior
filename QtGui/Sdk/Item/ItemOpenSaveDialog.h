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
// File: ItemOpenSaveDialog.h
// Started by: Hattozo
// Started on: 2/14/2026
// Description:
#pragma once
#include "Sdk/Item/ItemListWidget.h"

#include <NoobWarrior/EmuDb/ItemType.h>
#include <NoobWarrior/Roblox/Api/Asset.h>

#include <QDialog>
#include <QVBoxLayout>
#include <QComboBox>

namespace NoobWarrior {
class ItemOpenSaveDialog : public QDialog {
    Q_OBJECT
public:
    enum class Mode {
        Open,
        Save
    };

    // if enforce is true then itemType and assetType parameters will be forced. otherwise they are just defaults.\
    // assetType does nothing if itemType is set to anything but Asset
    static std::optional<int64_t> GetOpenId(QWidget *parent, EmuDb* db, ItemType itemType = ItemType::Asset, Roblox::AssetType assetType = Roblox::AssetType::None, bool enforce = false);

    // Opens an Asset restricted to the given types: the item-type dropdown is locked to Asset and
    // the asset-type dropdown lists only these, preceded by an "All" entry that shows every
    // allowed type at once.
    static std::optional<int64_t> GetOpenAssetId(QWidget *parent, EmuDb* db,
                                                 const std::vector<Roblox::AssetType>& allowedTypes);

    ItemOpenSaveDialog(EmuDb* db, Mode mode = Mode::Open, ItemType defaultItemType = ItemType::Asset, Roblox::AssetType defaultAssetType = Roblox::AssetType::None, QWidget *parent = nullptr);
    void ToggleItemTypeDropdown(bool val);
    void ToggleAssetTypeDropdown(bool val);
    void RestrictToAssetTypes(const std::vector<Roblox::AssetType>& types);
protected:
    void InitWidgets();
    void Repopulate();
private:
    std::optional<int64_t> mSelectedId;
    ItemType mSelectedItemType { ItemType::Asset }; // the table the selection came from

    EmuDb* mDb;
    ItemType mItemType;
    Roblox::AssetType mAssetType;
    std::vector<Roblox::AssetType> mAllowedAssetTypes; // empty = unrestricted

    QVBoxLayout* mLayout;

    QComboBox* mItemTypeDropdown;
    QComboBox* mAssetTypeDropdown;
    ItemListWidget* mList;
};
}