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
// File: ItemDialog_Impl_Set.cpp
// Started by: Hattozo
// Started on: 5/30/2026
// Description: Implements the Set type for ItemDialog
#include "ItemDialog.h"
#include "ItemOpenSaveDialog.h"

#include <QRegularExpressionValidator>

using namespace NoobWarrior;

void ItemDialog::Set_AddFields() {
    auto *db = GetDatabase();

    AddOwnedItemFields();

    int64_t imageId = 0;
    int64_t subscribers = 0;
    if (mId.has_value()) {
        Statement stmt = db->PrepareStatement("SELECT ImageId, Historical_Subscribers FROM \"Set\" WHERE Id = ?;");
        stmt.Bind(1, mId.value());
        if (stmt.Step() == SQLITE_ROW) {
            imageId = stmt.GetInt64FromColumnIndex(0);
            subscribers = stmt.GetInt64FromColumnIndex(1);
        }
    }
    mImageIdInput->setText(QString::number(imageId));

    mSet_SubscribersInput = new QLineEdit(QString::number(subscribers));
    mSet_SubscribersInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mSet_SubscribersInput));
    mContentLayout->addRow("Subscribers", mSet_SubscribersInput);

    // The assets that make up this set (SetAsset table).
    auto *assetFrame = new QFrame();
    auto *assetFrameLayout = new QVBoxLayout(assetFrame);

    mSet_AssetList = new ItemListWidget(nullptr, db);
    mSet_AssetList->SetOnContextMenuShown([this](QMenu* menu, ItemWidget* item) {
        QAction* removeAction = menu->addAction(QIcon(":/images/silk/cross.png"), "Remove From List");
        connect(removeAction, &QAction::triggered, [this, item]() {
            int64_t assetId = item->GetId();
            mSet_AssetList->Remove(item->GetType(), assetId);
            mSet_PendingDeleteAssets.push_back(assetId);
            mSet_PendingAssets.removeAll(assetId);
        });
    });

    if (mId.has_value()) {
        Statement assetsStmt = db->PrepareStatement("SELECT AssetId FROM SetAsset WHERE Id = ?;");
        assetsStmt.Bind(1, mId.value());
        while (assetsStmt.Step() == SQLITE_ROW) {
            int64_t assetId = assetsStmt.GetInt64FromColumnIndex(0);
            if (!mSet_AssetList->Add(ItemType::Asset, assetId))
                QMessageBox::critical(this, "Error", "Failed to load asset into list!");
        }
    }

    mSet_AddAssetButton = new QPushButton("Add Asset");
    connect(mSet_AddAssetButton, &QPushButton::clicked, [this]() {
        std::optional<int64_t> id = ItemOpenSaveDialog::GetOpenId(this, GetDatabase(), ItemType::Asset);
        if (id.has_value()) {
            if (mSet_AssetList->IsItemInList(ItemType::Asset, id.value())) {
                QMessageBox::critical(this, "Asset Already Exists", "You already added this asset to the list.");
                return;
            }
            if (!mSet_AssetList->Add(ItemType::Asset, id.value())) {
                QMessageBox::critical(this, "Error", "Failed to add asset");
                return;
            }
            mSet_PendingAssets.push_back(id.value());
            mSet_PendingDeleteAssets.removeAll(id.value());
        }
    });

    assetFrameLayout->addWidget(mSet_AssetList);
    assetFrameLayout->addWidget(mSet_AddAssetButton);
    mContentLayout->addRow("Assets", assetFrame);
}

bool ItemDialog::Set_OnSave() {
    auto *db = GetDatabase();

    int64_t id = mIdInput->text().toLongLong();
    std::string name = mNameInput->text().toStdString();
    std::string description = mOwned_DescriptionInput->toPlainText().toStdString();
    int64_t imageId = mImageIdInput->text().toLongLong();
    int64_t subscribers = mSet_SubscribersInput->text().toLongLong();

    // If the Id changed, the main row was already renamed by ItemDialog::OnSave; bring the SetAsset rows along.
    int64_t oldId = mId.has_value() ? mId.value() : id;
    if (mId.has_value() && oldId != id) {
        Statement updateStmt = db->PrepareStatement("UPDATE SetAsset SET Id = ? WHERE Id = ?;");
        updateStmt.Bind(1, id);
        updateStmt.Bind(2, oldId);
        if (updateStmt.Step() != SQLITE_DONE) {
            QMessageBox::critical(this, "Cannot Save", QString("Failed to update Id field in SetAsset table.\nLast error: %1").arg(QString::fromStdString(db->GetLastErrorMsg())));
            return false;
        }
    }

    Statement stmt = db->PrepareStatement(R"(
        INSERT INTO "Set" (Id, Name, Description, Created, Updated, ImageId, Historical_Subscribers) VALUES (?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT (Id) DO UPDATE SET
            LastRecorded = (unixepoch()),
            Name = excluded.Name,
            Description = excluded.Description,
            Created = excluded.Created,
            Updated = excluded.Updated,
            ImageId = excluded.ImageId,
            Historical_Subscribers = excluded.Historical_Subscribers;
    )");
    stmt.Bind(1, id);
    stmt.Bind(2, name);
    stmt.Bind(3, description);
    stmt.Bind(4, static_cast<int64_t>(mOwned_CreatedInput->dateTime().toSecsSinceEpoch()));
    stmt.Bind(5, static_cast<int64_t>(mOwned_UpdatedInput->dateTime().toSecsSinceEpoch()));
    stmt.Bind(6, imageId);
    stmt.Bind(7, subscribers);

    if (stmt.Step() != SQLITE_DONE) {
        QMessageBox::critical(this, "Failed to Save Changes", QString("Saving changes to the database failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
        return false;
    }

    for (int64_t assetId : mSet_PendingDeleteAssets) {
        Statement del = db->PrepareStatement("DELETE FROM SetAsset WHERE Id = ? AND AssetId = ?;");
        del.Bind(1, id);
        del.Bind(2, assetId);
        if (del.Step() != SQLITE_DONE) {
            QMessageBox::critical(this, "Failed to Save Changes", QString("Saving changes to the database failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
            return false;
        }
    }

    for (int64_t assetId : mSet_PendingAssets) {
        Statement ins = db->PrepareStatement("INSERT OR IGNORE INTO SetAsset (Id, AssetId) VALUES (?, ?);");
        ins.Bind(1, id);
        ins.Bind(2, assetId);
        if (ins.Step() != SQLITE_DONE) {
            QMessageBox::critical(this, "Failed to Save Changes", QString("Saving changes to the database failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
            return false;
        }
    }
    return true;
}
