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
// File: ItemDialog_Impl_DevProduct.cpp
// Started by: Hattozo
// Started on: 5/30/2026
// Description: Implements the DevProduct type for ItemDialog
#include "ItemDialog.h"
#include "ItemOpenSaveDialog.h"

#include <NoobWarrior/Roblox/Api/Asset.h>

#include <QRegularExpressionValidator>

using namespace NoobWarrior;

void ItemDialog::DevProduct_AddFields() {
    auto *db = GetDatabase();

    AddOwnedItemFields();

    Roblox::CurrencyType currencyType = Roblox::CurrencyType::Robux;
    int64_t price = 0;
    int64_t imageId = 0;
    int64_t universeId = 0;
    if (mId.has_value()) {
        Statement stmt = db->PrepareStatement("SELECT CurrencyType, Price, ImageId, UniverseId FROM DevProduct WHERE Id = ?;");
        stmt.Bind(1, mId.value());
        if (stmt.Step() == SQLITE_ROW) {
            currencyType = static_cast<Roblox::CurrencyType>(stmt.GetIntFromColumnIndex(0));
            price = stmt.GetInt64FromColumnIndex(1);
            imageId = stmt.GetInt64FromColumnIndex(2);
            universeId = stmt.GetInt64FromColumnIndex(3);
        }
    }
    mImageIdInput->setText(QString::number(imageId));

    mDevProduct_CurrencyTypeInput = new QComboBox();
    for (int i = 0; i <= static_cast<int>(Roblox::CurrencyType::Tix); i++) {
        Roblox::CurrencyType type = static_cast<Roblox::CurrencyType>(i);
        mDevProduct_CurrencyTypeInput->addItem(Roblox::CurrencyTypeAsTranslatableString(type), i);
    }
    int currencyIndex = mDevProduct_CurrencyTypeInput->findData(static_cast<int>(currencyType));
    if (currencyIndex != -1)
        mDevProduct_CurrencyTypeInput->setCurrentIndex(currencyIndex);
    mContentLayout->addRow("Currency", mDevProduct_CurrencyTypeInput);

    mDevProduct_PriceInput = new QLineEdit(QString::number(price));
    mDevProduct_PriceInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mDevProduct_PriceInput));
    mContentLayout->addRow("Price", mDevProduct_PriceInput);

    // UniverseId is NOT NULL in the schema; every developer product belongs to a universe.
    auto *universeFrame = new QFrame();
    auto *universeLayout = new QHBoxLayout(universeFrame);
    universeLayout->setContentsMargins(0, 0, 0, 0);
    mDevProduct_UniverseIdInput = new QLineEdit(QString::number(universeId));
    mDevProduct_UniverseIdInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mDevProduct_UniverseIdInput));
    auto *universeBrowse = new QPushButton("Browse...");
    connect(universeBrowse, &QPushButton::clicked, [this]() {
        std::optional<int64_t> id = ItemOpenSaveDialog::GetOpenId(this, GetDatabase(), ItemType::Universe);
        if (id.has_value())
            mDevProduct_UniverseIdInput->setText(QString::number(id.value()));
    });
    universeLayout->addWidget(mDevProduct_UniverseIdInput);
    universeLayout->addWidget(universeBrowse);
    mContentLayout->addRow("Universe Id", universeFrame);
}

bool ItemDialog::DevProduct_OnSave() {
    auto *db = GetDatabase();

    int64_t id = mIdInput->text().toLongLong();
    std::string name = mNameInput->text().toStdString();
    std::string description = mOwned_DescriptionInput->toPlainText().toStdString();
    int currencyType = mDevProduct_CurrencyTypeInput->currentData().toInt();
    int64_t price = mDevProduct_PriceInput->text().toLongLong();
    int64_t imageId = mImageIdInput->text().toLongLong();
    int64_t universeId = mDevProduct_UniverseIdInput->text().toLongLong();

    Statement stmt = db->PrepareStatement(R"(
        INSERT INTO DevProduct (Id, Name, Description, Created, Updated, CurrencyType, Price, ImageId, UniverseId) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT (Id) DO UPDATE SET
            LastRecorded = (unixepoch()),
            Name = excluded.Name,
            Description = excluded.Description,
            Created = excluded.Created,
            Updated = excluded.Updated,
            CurrencyType = excluded.CurrencyType,
            Price = excluded.Price,
            ImageId = excluded.ImageId,
            UniverseId = excluded.UniverseId;
    )");
    stmt.Bind(1, id);
    stmt.Bind(2, name);
    stmt.Bind(3, description);
    stmt.Bind(4, static_cast<int64_t>(mOwned_CreatedInput->dateTime().toSecsSinceEpoch()));
    stmt.Bind(5, static_cast<int64_t>(mOwned_UpdatedInput->dateTime().toSecsSinceEpoch()));
    stmt.Bind(6, currencyType);
    stmt.Bind(7, price);
    stmt.Bind(8, imageId);
    stmt.Bind(9, universeId);

    if (stmt.Step() != SQLITE_DONE) {
        QMessageBox::critical(this, "Failed to Save Changes", QString("Saving changes to the database failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
        return false;
    }
    return true;
}
