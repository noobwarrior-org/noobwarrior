// === noobWarrior ===
// File: AssetDialog.h
// Started by: Hattozo
// Started on: 11/30/2025
// Description:
#pragma once
#include "ItemDialog.h"
#include <NoobWarrior/Database/Item/Asset.h>

namespace NoobWarrior {
class AssetDialog : public ItemDialog<Asset> {
    Q_OBJECT
public:
    AssetDialog(QWidget *parent = nullptr, Asset asset = {});
    void RegenWidgets() override;
};
}