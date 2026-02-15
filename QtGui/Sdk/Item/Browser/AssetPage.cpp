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
// File: AssetPage.cpp
// Started by: Hattozo
// Started on: 11/30/2025
// Description:
#include "AssetPage.h"
#include "Sdk/Item/ItemListWidget.h"
#include "BrowserItem.h"
#include "ItemBrowserWidget.h"
#include <NoobWarrior/EmuDb/EmuDb.h>

using namespace NoobWarrior;

AssetPage::AssetPage(ItemBrowserWidget *browser) : ItemListWidget(browser), mBrowser(browser) {}

void AssetPage::Refresh() {
    SearchOptions opt {};
    opt.Offset = 0;
    opt.Limit = 100;
    opt.AssetType = mType;

    EmuDb* db = mBrowser->GetDatabase();

    Statement stmt = db->PrepareStatement("SELECT Id FROM Asset");
    while (stmt.Step() == SQLITE_ROW) {
        int id = stmt.GetIntFromColumnIndex(0);
        new BrowserItem(db, "Asset", id, this);
    }

    /*
    std::vector<Asset> list = db->GetAssetRepository()->List();
    for (auto &item : list) {
        new BrowserItem<Asset>(item, db, this);
    }
    */
}

void AssetPage::SetType(Roblox::AssetType type) {
    mType = type;
}