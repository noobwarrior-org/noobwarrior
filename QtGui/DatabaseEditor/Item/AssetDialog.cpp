// === noobWarrior ===
// File: AssetDialog.h
// Started by: Hattozo
// Started on: 11/30/2025
// Description:
#include "AssetDialog.h"

using namespace NoobWarrior;

AssetDialog::AssetDialog(QWidget *parent, Asset asset) : ItemDialog<Asset>(parent, asset) {

}

void AssetDialog::RegenWidgets() {
    ItemDialog<Asset>::RegenWidgets();
}