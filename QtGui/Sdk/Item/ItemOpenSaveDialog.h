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
    static int GetOpenId(QWidget *parent, ItemType itemType = ItemType::Asset, Roblox::AssetType assetType = Roblox::AssetType::None, bool enforce = false);

    ItemOpenSaveDialog(Mode mode = Mode::Open, QWidget *parent = nullptr);
    void ToggleItemTypeDropdown(bool val);
    void ToggleAssetTypeDropdown(bool val);
private:
    void InitWidgets();
    QVBoxLayout* mLayout;

    QComboBox* mItemTypeDropdown;
    QComboBox* mAssetTypeDropdown;
    ItemListWidget* mList;
};
}