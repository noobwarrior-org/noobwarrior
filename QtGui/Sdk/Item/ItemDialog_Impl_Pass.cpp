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
// File: ItemDialog_Impl_Pass.cpp
// Started by: Hattozo
// Started on: 5/30/2026
// Description: Implements the Pass (game pass) type for ItemDialog
#include "ItemDialog.h"
#include "ItemOpenSaveDialog.h"

#include <QRegularExpressionValidator>

using namespace NoobWarrior;

void ItemDialog::Pass_AddFields() {
    auto *db = GetDatabase();

    AddOwnedItemFields();

    int64_t imageId = 0;
    int64_t universeId = 0;
    int64_t price = 0;
    bool isForSale = false;
    int64_t likes = 0;
    int64_t dislikes = 0;
    if (mId.has_value()) {
        Statement stmt = db->PrepareStatement("SELECT ImageId, UniverseId, PriceInRobux, IsForSale, Historical_Likes, Historical_Dislikes FROM Pass WHERE Id = ?;");
        stmt.Bind(1, mId.value());
        if (stmt.Step() == SQLITE_ROW) {
            imageId = stmt.GetInt64FromColumnIndex(0);
            universeId = stmt.GetInt64FromColumnIndex(1);
            price = stmt.GetInt64FromColumnIndex(2);
            isForSale = stmt.GetIntFromColumnIndex(3);
            likes = stmt.GetInt64FromColumnIndex(4);
            dislikes = stmt.GetInt64FromColumnIndex(5);
        }
    }
    mImageIdInput->setText(QString::number(imageId));

    // UniverseId is NOT NULL in the schema; every pass belongs to a universe.
    auto *universeFrame = new QFrame();
    auto *universeLayout = new QHBoxLayout(universeFrame);
    universeLayout->setContentsMargins(0, 0, 0, 0);
    mPass_UniverseIdInput = new QLineEdit(QString::number(universeId));
    mPass_UniverseIdInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mPass_UniverseIdInput));
    auto *universeBrowse = new QPushButton("Browse...");
    connect(universeBrowse, &QPushButton::clicked, [this]() {
        std::optional<int64_t> id = ItemOpenSaveDialog::GetOpenId(this, GetDatabase(), ItemType::Universe);
        if (id.has_value())
            mPass_UniverseIdInput->setText(QString::number(id.value()));
    });
    universeLayout->addWidget(mPass_UniverseIdInput);
    universeLayout->addWidget(universeBrowse);
    mContentLayout->addRow("Universe Id", universeFrame);

    mPass_PriceInput = new QLineEdit(QString::number(price));
    mPass_PriceInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mPass_PriceInput));
    mContentLayout->addRow("Price (Robux)", mPass_PriceInput);

    mPass_IsForSaleInput = new QCheckBox();
    mPass_IsForSaleInput->setChecked(isForSale);
    mContentLayout->addRow("For Sale", mPass_IsForSaleInput);

    mPass_LikesInput = new QLineEdit(QString::number(likes));
    mPass_LikesInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mPass_LikesInput));
    mContentLayout->addRow("Likes", mPass_LikesInput);

    mPass_DislikesInput = new QLineEdit(QString::number(dislikes));
    mPass_DislikesInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mPass_DislikesInput));
    mContentLayout->addRow("Dislikes", mPass_DislikesInput);
}

bool ItemDialog::Pass_OnSave() {
    auto *db = GetDatabase();

    int64_t id = mIdInput->text().toLongLong();
    std::string name = mNameInput->text().toStdString();
    std::string description = mOwned_DescriptionInput->toPlainText().toStdString();
    int64_t imageId = mImageIdInput->text().toLongLong();
    int64_t universeId = mPass_UniverseIdInput->text().toLongLong();
    int64_t price = mPass_PriceInput->text().toLongLong();
    bool isForSale = mPass_IsForSaleInput->isChecked();
    int64_t likes = mPass_LikesInput->text().toLongLong();
    int64_t dislikes = mPass_DislikesInput->text().toLongLong();

    Statement stmt = db->PrepareStatement(R"(
        INSERT INTO Pass (Id, Name, Description, Created, Updated, ImageId, UniverseId, PriceInRobux, IsForSale, Historical_Likes, Historical_Dislikes) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT (Id) DO UPDATE SET
            LastRecorded = (unixepoch()),
            Name = excluded.Name,
            Description = excluded.Description,
            Created = excluded.Created,
            Updated = excluded.Updated,
            ImageId = excluded.ImageId,
            UniverseId = excluded.UniverseId,
            PriceInRobux = excluded.PriceInRobux,
            IsForSale = excluded.IsForSale,
            Historical_Likes = excluded.Historical_Likes,
            Historical_Dislikes = excluded.Historical_Dislikes;
    )");
    stmt.Bind(1, id);
    stmt.Bind(2, name);
    stmt.Bind(3, description);
    stmt.Bind(4, static_cast<int64_t>(mOwned_CreatedInput->dateTime().toSecsSinceEpoch()));
    stmt.Bind(5, static_cast<int64_t>(mOwned_UpdatedInput->dateTime().toSecsSinceEpoch()));
    stmt.Bind(6, imageId);
    stmt.Bind(7, universeId);
    stmt.Bind(8, price);
    stmt.Bind(9, isForSale);
    stmt.Bind(10, likes);
    stmt.Bind(11, dislikes);

    if (stmt.Step() != SQLITE_DONE) {
        QMessageBox::critical(this, "Failed to Save Changes", QString("Saving changes to the database failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
        return false;
    }
    return true;
}
