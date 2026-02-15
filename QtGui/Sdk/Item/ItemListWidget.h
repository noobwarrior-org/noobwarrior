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
// File: ItemBrowserPage.h
// Started by: Hattozo
// Started on: 11/30/2025
// Description:
#pragma once
#include <NoobWarrior/EmuDb/ItemType.h>
#include <NoobWarrior/Roblox/Api/Asset.h>

#include <QListWidget>
#include <QAbstractListModel>

#include <string>

namespace NoobWarrior {
class ItemListWidget : public QListWidget {
public:
    struct PopulateOptions {
        /* ItemType and AssetType have no effect if EnforceType is set to false */
        ItemType ItemType { ItemType::Asset };
        Roblox::AssetType AssetType { Roblox::AssetType::None };
        /* Offset and Limit have no effect if EnforceLimit is set to false */
        int Offset { 0 };
        int Limit { 100 };
        bool EnforceType { false };
        bool EnforceLimit { false };
        /* Query has no effect if it is set to blank */
        std::string Query { "" };
    };

    ItemListWidget(QWidget *parent = nullptr);
    virtual void Refresh();
    void AddItem(ItemType type, int id);
    void Populate(const PopulateOptions options);
protected:
    void InitWidgets();
};
}