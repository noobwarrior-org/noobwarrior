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
// File: ItemDialog_Impl_Outfit.cpp
// Started by: Hattozo
// Started on: 5/30/2026
// Description: Implements the Outfit type for ItemDialog
#include "ItemDialog.h"
#include "ItemOpenSaveDialog.h"

#include <QRegularExpressionValidator>
#include <QDoubleValidator>

using namespace NoobWarrior;

void ItemDialog::Outfit_AddFields() {
    auto *db = GetDatabase();

    // Outfit has no Description or Updated-only owned-item layout, so build fields by hand.
    int64_t created = 0;
    int64_t updated = 0;
    int64_t userId = 0;
    int bodyType = 0;
    double width = 1.0;
    double height = 1.0;
    double head = 1.0;
    double proportions = 0.0;
    if (mId.has_value()) {
        Statement stmt = db->PrepareStatement("SELECT Created, Updated, UserId, BodyType, Width, Height, Head, Proportions FROM Outfit WHERE Id = ?;");
        stmt.Bind(1, mId.value());
        if (stmt.Step() == SQLITE_ROW) {
            created = stmt.GetInt64FromColumnIndex(0);
            updated = stmt.GetInt64FromColumnIndex(1);
            userId = stmt.GetInt64FromColumnIndex(2);
            bodyType = stmt.GetIntFromColumnIndex(3);
            width = stmt.GetDoubleFromColumnIndex(4);
            height = stmt.GetDoubleFromColumnIndex(5);
            head = stmt.GetDoubleFromColumnIndex(6);
            proportions = stmt.GetDoubleFromColumnIndex(7);
        }
    }

    mOwned_CreatedInput = new QDateTimeEdit();
    mOwned_CreatedInput->setDateTime(mId.has_value() ? QDateTime::fromSecsSinceEpoch(created) : QDateTime::currentDateTime());
    mContentLayout->addRow("Created", mOwned_CreatedInput);

    mOwned_UpdatedInput = new QDateTimeEdit();
    mOwned_UpdatedInput->setDateTime(mId.has_value() ? QDateTime::fromSecsSinceEpoch(updated) : QDateTime::currentDateTime());
    mContentLayout->addRow("Updated", mOwned_UpdatedInput);

    // UserId references the owning User.
    auto *userFrame = new QFrame();
    auto *userLayout = new QHBoxLayout(userFrame);
    userLayout->setContentsMargins(0, 0, 0, 0);
    mOutfit_UserIdInput = new QLineEdit(QString::number(userId));
    mOutfit_UserIdInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mOutfit_UserIdInput));
    auto *userBrowse = new QPushButton("Browse...");
    connect(userBrowse, &QPushButton::clicked, [this]() {
        std::optional<int64_t> id = ItemOpenSaveDialog::GetOpenId(this, GetDatabase(), ItemType::User);
        if (id.has_value())
            mOutfit_UserIdInput->setText(QString::number(id.value()));
    });
    userLayout->addWidget(mOutfit_UserIdInput);
    userLayout->addWidget(userBrowse);
    mContentLayout->addRow("User Id", userFrame);

    mOutfit_BodyTypeInput = new QLineEdit(QString::number(bodyType));
    mOutfit_BodyTypeInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mOutfit_BodyTypeInput));
    mContentLayout->addRow("Body Type", mOutfit_BodyTypeInput);

    mOutfit_WidthInput = new QLineEdit(QString::number(width));
    mOutfit_WidthInput->setValidator(new QDoubleValidator(mOutfit_WidthInput));
    mContentLayout->addRow("Width", mOutfit_WidthInput);

    mOutfit_HeightInput = new QLineEdit(QString::number(height));
    mOutfit_HeightInput->setValidator(new QDoubleValidator(mOutfit_HeightInput));
    mContentLayout->addRow("Height", mOutfit_HeightInput);

    mOutfit_HeadInput = new QLineEdit(QString::number(head));
    mOutfit_HeadInput->setValidator(new QDoubleValidator(mOutfit_HeadInput));
    mContentLayout->addRow("Head", mOutfit_HeadInput);

    mOutfit_ProportionsInput = new QLineEdit(QString::number(proportions));
    mOutfit_ProportionsInput->setValidator(new QDoubleValidator(mOutfit_ProportionsInput));
    mContentLayout->addRow("Proportions", mOutfit_ProportionsInput);

    // The assets worn by this outfit (OutfitItem table).
    auto *itemFrame = new QFrame();
    auto *itemFrameLayout = new QVBoxLayout(itemFrame);

    mOutfit_ItemList = new ItemListWidget(nullptr, db);
    mOutfit_ItemList->SetOnContextMenuShown([this](QMenu* menu, ItemWidget* item) {
        QAction* removeAction = menu->addAction(QIcon(":/images/silk/cross.png"), "Remove From List");
        connect(removeAction, &QAction::triggered, [this, item]() {
            int64_t assetId = item->GetId();
            mOutfit_ItemList->Remove(item->GetType(), assetId);
            mOutfit_PendingDeleteItems.push_back(assetId);
            mOutfit_PendingItems.removeAll(assetId);
        });
    });

    if (mId.has_value()) {
        Statement itemsStmt = db->PrepareStatement("SELECT AssetId FROM OutfitItem WHERE Id = ?;");
        itemsStmt.Bind(1, mId.value());
        while (itemsStmt.Step() == SQLITE_ROW) {
            int64_t assetId = itemsStmt.GetInt64FromColumnIndex(0);
            if (!mOutfit_ItemList->Add(ItemType::Asset, assetId))
                QMessageBox::critical(this, "Error", "Failed to load asset into list!");
        }
    }

    mOutfit_AddItemButton = new QPushButton("Add Asset");
    connect(mOutfit_AddItemButton, &QPushButton::clicked, [this]() {
        std::optional<int64_t> id = ItemOpenSaveDialog::GetOpenId(this, GetDatabase(), ItemType::Asset);
        if (id.has_value()) {
            if (mOutfit_ItemList->IsItemInList(ItemType::Asset, id.value())) {
                QMessageBox::critical(this, "Asset Already Exists", "You already added this asset to the list.");
                return;
            }
            if (!mOutfit_ItemList->Add(ItemType::Asset, id.value())) {
                QMessageBox::critical(this, "Error", "Failed to add asset");
                return;
            }
            mOutfit_PendingItems.push_back(id.value());
            mOutfit_PendingDeleteItems.removeAll(id.value());
        }
    });

    itemFrameLayout->addWidget(mOutfit_ItemList);
    itemFrameLayout->addWidget(mOutfit_AddItemButton);
    mContentLayout->addRow("Assets", itemFrame);
}

static constexpr const char* sOutfitChildTables[] = {
    "OutfitItem",
    "OutfitBodyColor"
};

bool ItemDialog::Outfit_OnSave() {
    auto *db = GetDatabase();

    int64_t newId = mIdInput->text().toLongLong();
    int64_t oldId = mId.has_value() ? mId.value() : newId;
    bool idChanged = mId.has_value() && oldId != newId;

    if (idChanged) {
        for (int i = 0; i < std::size(sOutfitChildTables); i++) {
            Statement updateStmt = db->PrepareStatement(std::format("UPDATE {} SET Id = ? WHERE Id = ?;", sOutfitChildTables[i]));
            updateStmt.Bind(1, newId);
            updateStmt.Bind(2, oldId);
            if (updateStmt.Step() != SQLITE_DONE) {
                QMessageBox::critical(this, "Cannot Save",
                    QString("Failed to update Id field in %1 table.\nLast error: %2")
                    .arg(sOutfitChildTables[i])
                    .arg(QString::fromStdString(db->GetLastErrorMsg())));
                return false;
            }
        }
    }

    std::string name = mNameInput->text().toStdString();
    int64_t created = mOwned_CreatedInput->dateTime().toSecsSinceEpoch();
    int64_t updated = mOwned_UpdatedInput->dateTime().toSecsSinceEpoch();
    int64_t userId = mOutfit_UserIdInput->text().toLongLong();
    int bodyType = mOutfit_BodyTypeInput->text().toInt();
    double width = mOutfit_WidthInput->text().toDouble();
    double height = mOutfit_HeightInput->text().toDouble();
    double head = mOutfit_HeadInput->text().toDouble();
    double proportions = mOutfit_ProportionsInput->text().toDouble();

    Statement stmt = db->PrepareStatement(R"(
        INSERT INTO Outfit (Id, Name, Created, Updated, UserId, BodyType, Width, Height, Head, Proportions) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT (Id) DO UPDATE SET
            LastRecorded = (unixepoch()),
            Name = excluded.Name,
            Created = excluded.Created,
            Updated = excluded.Updated,
            UserId = excluded.UserId,
            BodyType = excluded.BodyType,
            Width = excluded.Width,
            Height = excluded.Height,
            Head = excluded.Head,
            Proportions = excluded.Proportions;
    )");
    stmt.Bind(1, newId);
    stmt.Bind(2, name);
    stmt.Bind(3, created);
    stmt.Bind(4, updated);
    stmt.Bind(5, userId);
    stmt.Bind(6, bodyType);
    stmt.Bind(7, width);
    stmt.Bind(8, height);
    stmt.Bind(9, head);
    stmt.Bind(10, proportions);

    if (stmt.Step() != SQLITE_DONE) {
        QMessageBox::critical(this, "Failed to Save Changes", QString("Saving changes to the database failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
        return false;
    }

    for (int64_t assetId : mOutfit_PendingDeleteItems) {
        Statement del = db->PrepareStatement("DELETE FROM OutfitItem WHERE Id = ? AND AssetId = ?;");
        del.Bind(1, newId);
        del.Bind(2, assetId);
        if (del.Step() != SQLITE_DONE) {
            QMessageBox::critical(this, "Failed to Save Changes", QString("Saving changes to the database failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
            return false;
        }
    }

    for (int64_t assetId : mOutfit_PendingItems) {
        Statement ins = db->PrepareStatement("INSERT OR IGNORE INTO OutfitItem (Id, AssetId) VALUES (?, ?);");
        ins.Bind(1, newId);
        ins.Bind(2, assetId);
        if (ins.Step() != SQLITE_DONE) {
            QMessageBox::critical(this, "Failed to Save Changes", QString("Saving changes to the database failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
            return false;
        }
    }
    return true;
}
