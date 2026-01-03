// === noobWarrior ===
// File: AssetPage.h
// Started by: Hattozo
// Started on: 11/30/2025
// Description:
#pragma once
#include "ItemBrowserPage.h"
#include <NoobWarrior/Database/Item/Asset.h>

namespace NoobWarrior {
class ItemBrowserWidget;
class AssetPage : public ItemBrowserPage {
public:
    AssetPage(ItemBrowserWidget *browser);
    void Refresh() override;
    void SetType(Roblox::AssetType);
private:
    ItemBrowserWidget *mBrowser;
    Roblox::AssetType mType { Roblox::AssetType::None };
};
}