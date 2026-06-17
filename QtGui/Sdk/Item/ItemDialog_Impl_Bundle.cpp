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
// File: ItemDialog_Impl_Bundle.cpp
// Started by: Hattozo
// Started on: 5/30/2026
// Description: Implements the Bundle type for ItemDialog
#include "ItemDialog.h"
#include "ItemOpenSaveDialog.h"

#include <NoobWarrior/Roblox/Api/Bundle.h>

#include <QRegularExpressionValidator>

using namespace NoobWarrior;

void ItemDialog::Bundle_AddFields() {
    auto *db = GetDatabase();

    AddOwnedItemFields();

    Roblox::BundleType type = Roblox::BundleType::BodyParts;
    int64_t price = 0;
    bool isForSale = false;
    bool isNew = false;
    bool isLimited = false;
    bool isLimitedUnique = false;
    int64_t remaining = 0;
    int64_t sales = 0;
    int64_t favorites = 0;
    if (mId.has_value()) {
        Statement stmt = db->PrepareStatement("SELECT Type, PriceInRobux, IsForSale, IsNew, IsLimited, IsLimitedUnique, Remaining, Historical_Sales, Historical_Favorites FROM Bundle WHERE Id = ?;");
        stmt.Bind(1, mId.value());
        if (stmt.Step() == SQLITE_ROW) {
            type = static_cast<Roblox::BundleType>(stmt.GetIntFromColumnIndex(0));
            price = stmt.GetInt64FromColumnIndex(1);
            isForSale = stmt.GetIntFromColumnIndex(2);
            isNew = stmt.GetIntFromColumnIndex(3);
            isLimited = stmt.GetIntFromColumnIndex(4);
            isLimitedUnique = stmt.GetIntFromColumnIndex(5);
            remaining = stmt.GetInt64FromColumnIndex(6);
            sales = stmt.GetInt64FromColumnIndex(7);
            favorites = stmt.GetInt64FromColumnIndex(8);
        }
    }

    mBundle_TypeInput = new QComboBox();
    mBundle_TypeInput->addItem("Body Parts", static_cast<int>(Roblox::BundleType::BodyParts));
    mBundle_TypeInput->addItem("Animations", static_cast<int>(Roblox::BundleType::Animations));
    mBundle_TypeInput->addItem("Shoes", static_cast<int>(Roblox::BundleType::Shoes));
    mBundle_TypeInput->addItem("Dynamic Head", static_cast<int>(Roblox::BundleType::DynamicHead));
    mBundle_TypeInput->addItem("Dynamic Head Avatar", static_cast<int>(Roblox::BundleType::DynamicHeadAvatar));
    int typeIndex = mBundle_TypeInput->findData(static_cast<int>(type));
    if (typeIndex != -1)
        mBundle_TypeInput->setCurrentIndex(typeIndex);
    mContentLayout->addRow("Type", mBundle_TypeInput);

    mBundle_PriceInput = new QLineEdit(QString::number(price));
    mBundle_PriceInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mBundle_PriceInput));
    mContentLayout->addRow("Price (Robux)", mBundle_PriceInput);

    mBundle_IsForSaleInput = new QCheckBox();
    mBundle_IsForSaleInput->setChecked(isForSale);
    mContentLayout->addRow("For Sale", mBundle_IsForSaleInput);

    mBundle_IsNewInput = new QCheckBox();
    mBundle_IsNewInput->setChecked(isNew);
    mContentLayout->addRow("New", mBundle_IsNewInput);

    mBundle_IsLimitedInput = new QCheckBox();
    mBundle_IsLimitedInput->setChecked(isLimited);
    mContentLayout->addRow("Limited", mBundle_IsLimitedInput);

    mBundle_IsLimitedUniqueInput = new QCheckBox();
    mBundle_IsLimitedUniqueInput->setChecked(isLimitedUnique);
    mContentLayout->addRow("Limited Unique", mBundle_IsLimitedUniqueInput);

    mBundle_RemainingInput = new QLineEdit(QString::number(remaining));
    mBundle_RemainingInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mBundle_RemainingInput));
    mContentLayout->addRow("Remaining", mBundle_RemainingInput);

    mBundle_SalesInput = new QLineEdit(QString::number(sales));
    mBundle_SalesInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mBundle_SalesInput));
    mContentLayout->addRow("Sales", mBundle_SalesInput);

    mBundle_FavoritesInput = new QLineEdit(QString::number(favorites));
    mBundle_FavoritesInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mBundle_FavoritesInput));
    mContentLayout->addRow("Favorites", mBundle_FavoritesInput);

    // The assets that make up this bundle (BundleAsset table).
    auto *assetFrame = new QFrame();
    auto *assetFrameLayout = new QVBoxLayout(assetFrame);

    mBundle_AssetList = new ItemListWidget(nullptr, db);
    mBundle_AssetList->SetOnContextMenuShown([this](QMenu* menu, ItemWidget* item) {
        QAction* removeAction = menu->addAction(QIcon(":/images/silk/cross.png"), "Remove From List");
        connect(removeAction, &QAction::triggered, [this, item]() {
            int64_t assetId = item->GetId();
            mBundle_AssetList->Remove(item->GetType(), assetId);
            mBundle_PendingDeleteAssets.push_back(assetId);
            mBundle_PendingAssets.removeAll(assetId);
        });
    });

    if (mId.has_value()) {
        Statement assetsStmt = db->PrepareStatement("SELECT AssetId FROM BundleAsset WHERE Id = ?;");
        assetsStmt.Bind(1, mId.value());
        while (assetsStmt.Step() == SQLITE_ROW) {
            int64_t assetId = assetsStmt.GetInt64FromColumnIndex(0);
            if (!mBundle_AssetList->Add(ItemType::Asset, assetId))
                QMessageBox::critical(this, "Error", "Failed to load asset into list!");
        }
    }

    mBundle_AddAssetButton = new QPushButton("Add Asset");
    connect(mBundle_AddAssetButton, &QPushButton::clicked, [this]() {
        std::optional<int64_t> id = ItemOpenSaveDialog::GetOpenId(this, GetDatabase(), ItemType::Asset);
        if (id.has_value()) {
            if (mBundle_AssetList->IsItemInList(ItemType::Asset, id.value())) {
                QMessageBox::critical(this, "Asset Already Exists", "You already added this asset to the list.");
                return;
            }
            if (!mBundle_AssetList->Add(ItemType::Asset, id.value())) {
                QMessageBox::critical(this, "Error", "Failed to add asset");
                return;
            }
            mBundle_PendingAssets.push_back(id.value());
            mBundle_PendingDeleteAssets.removeAll(id.value());
        }
    });

    assetFrameLayout->addWidget(mBundle_AssetList);
    assetFrameLayout->addWidget(mBundle_AddAssetButton);
    mContentLayout->addRow("Assets", assetFrame);
}

bool ItemDialog::Bundle_OnSave() {
    auto *db = GetDatabase();

    int64_t id = mIdInput->text().toLongLong();
    std::string name = mNameInput->text().toStdString();
    std::string description = mOwned_DescriptionInput->toPlainText().toStdString();
    int type = mBundle_TypeInput->currentData().toInt();
    int64_t price = mBundle_PriceInput->text().toLongLong();
    bool isForSale = mBundle_IsForSaleInput->isChecked();
    bool isNew = mBundle_IsNewInput->isChecked();
    bool isLimited = mBundle_IsLimitedInput->isChecked();
    bool isLimitedUnique = mBundle_IsLimitedUniqueInput->isChecked();
    int64_t remaining = mBundle_RemainingInput->text().toLongLong();
    int64_t sales = mBundle_SalesInput->text().toLongLong();
    int64_t favorites = mBundle_FavoritesInput->text().toLongLong();

    // If the Id changed, the main row was already renamed by ItemDialog::OnSave; bring the BundleAsset rows along.
    int64_t oldId = mId.has_value() ? mId.value() : id;
    if (mId.has_value() && oldId != id) {
        Statement updateStmt = db->PrepareStatement("UPDATE BundleAsset SET Id = ? WHERE Id = ?;");
        updateStmt.Bind(1, id);
        updateStmt.Bind(2, oldId);
        if (updateStmt.Step() != SQLITE_DONE) {
            QMessageBox::critical(this, "Cannot Save", QString("Failed to update Id field in BundleAsset table.\nLast error: %1").arg(QString::fromStdString(db->GetLastErrorMsg())));
            return false;
        }
    }

    Statement stmt = db->PrepareStatement(R"(
        INSERT INTO Bundle (Id, Name, Description, Created, Updated, Type, PriceInRobux, IsForSale, IsNew, IsLimited, IsLimitedUnique, Remaining, Historical_Sales, Historical_Favorites) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT (Id) DO UPDATE SET
            LastRecorded = (unixepoch()),
            Name = excluded.Name,
            Description = excluded.Description,
            Created = excluded.Created,
            Updated = excluded.Updated,
            Type = excluded.Type,
            PriceInRobux = excluded.PriceInRobux,
            IsForSale = excluded.IsForSale,
            IsNew = excluded.IsNew,
            IsLimited = excluded.IsLimited,
            IsLimitedUnique = excluded.IsLimitedUnique,
            Remaining = excluded.Remaining,
            Historical_Sales = excluded.Historical_Sales,
            Historical_Favorites = excluded.Historical_Favorites;
    )");
    stmt.Bind(1, id);
    stmt.Bind(2, name);
    stmt.Bind(3, description);
    stmt.Bind(4, static_cast<int64_t>(mOwned_CreatedInput->dateTime().toSecsSinceEpoch()));
    stmt.Bind(5, static_cast<int64_t>(mOwned_UpdatedInput->dateTime().toSecsSinceEpoch()));
    stmt.Bind(6, type);
    stmt.Bind(7, price);
    stmt.Bind(8, isForSale);
    stmt.Bind(9, isNew);
    stmt.Bind(10, isLimited);
    stmt.Bind(11, isLimitedUnique);
    stmt.Bind(12, remaining);
    stmt.Bind(13, sales);
    stmt.Bind(14, favorites);

    if (stmt.Step() != SQLITE_DONE) {
        QMessageBox::critical(this, "Failed to Save Changes", QString("Saving changes to the database failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
        return false;
    }

    for (int64_t assetId : mBundle_PendingDeleteAssets) {
        Statement del = db->PrepareStatement("DELETE FROM BundleAsset WHERE Id = ? AND AssetId = ?;");
        del.Bind(1, id);
        del.Bind(2, assetId);
        if (del.Step() != SQLITE_DONE) {
            QMessageBox::critical(this, "Failed to Save Changes", QString("Saving changes to the database failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
            return false;
        }
    }

    for (int64_t assetId : mBundle_PendingAssets) {
        Statement ins = db->PrepareStatement("INSERT OR IGNORE INTO BundleAsset (Id, AssetId) VALUES (?, ?);");
        ins.Bind(1, id);
        ins.Bind(2, assetId);
        if (ins.Step() != SQLITE_DONE) {
            QMessageBox::critical(this, "Failed to Save Changes", QString("Saving changes to the database failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
            return false;
        }
    }
    return true;
}
