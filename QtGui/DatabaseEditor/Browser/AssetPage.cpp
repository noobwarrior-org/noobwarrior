// === noobWarrior ===
// File: AssetPage.cpp
// Started by: Hattozo
// Started on: 11/30/2025
// Description:
#include "AssetPage.h"
#include "ItemBrowserWidget.h"
#include "BrowserItem.h"
#include <NoobWarrior/Database/Database.h>

using namespace NoobWarrior;

AssetPage::AssetPage(ItemBrowserWidget *browser) : ItemBrowserPage(browser), mBrowser(browser) {}

void AssetPage::Refresh() {
    SearchOptions opt {};
    opt.Offset = 0;
    opt.Limit = 100;
    opt.AssetType = mType;

    Database* db = mBrowser->GetDatabase();

    std::vector<Asset> list = db->GetAssetRepository().List();
    for (auto &item : list) {
        new BrowserItem<Asset>(item, db, this);
    }
}

void AssetPage::SetType(Roblox::AssetType type) {
    mType = type;
}