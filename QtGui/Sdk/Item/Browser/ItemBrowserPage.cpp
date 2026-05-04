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
// File: ItemBrowserPage.cpp
// Started by: Hattozo
// Started on: 11/30/2025
// Description:
#include "ItemBrowserPage.h"
#include "Sdk/Item/ItemListWidget.h"
#include "../ItemWidget.h"
#include "ItemBrowserWidget.h"
#include <NoobWarrior/EmuDb/EmuDb.h>

using namespace NoobWarrior;

ItemBrowserPage::ItemBrowserPage(ItemBrowserWidget *browser) : ItemListWidget(browser), mBrowser(browser) {}

void ItemBrowserPage::Refresh() {
    SearchOptions opt {};
    opt.Offset = 0;
    opt.Limit = 100;
    opt.AssetType = mAssetType;

    EmuDb* db = mBrowser->GetDatabase();

    Populate({
        .Database = db,
        .ItemType = mType,
        .AssetType = mAssetType,
        .Offset = 0,
        .Limit = 100,
        .EnforceLimit = true,
        .Query = mQuery.toStdString()
    });

    /*
    std::vector<Asset> list = db->GetAssetRepository()->List();
    for (auto &item : list) {
        new ItemWidget<Asset>(item, db, this);
    }
    */
}

void ItemBrowserPage::SetQuery(const QString &query) {
    mQuery = query;
}

void ItemBrowserPage::SetType(ItemType type) {
    mType = type;
}

void ItemBrowserPage::SetAssetType(Roblox::AssetType type) {
    mAssetType = type;
}