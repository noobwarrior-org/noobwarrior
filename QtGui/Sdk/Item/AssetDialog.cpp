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
// File: AssetDialog.h
// Started by: Hattozo
// Started on: 11/30/2025
// Description:
#include "AssetDialog.h"
#include "Sdk/CreatorInfoWidget.h"

#include <NoobWarrior/Roblox/Api/Asset.h>

#include <QMessageBox>
#include <QDateTimeEdit>
#include <QComboBox>

#include <cstdlib>

#define RAND_MAX 2147483647

using namespace NoobWarrior;

AssetDialog::AssetDialog(QWidget *parent, std::optional<int64_t> id) : ItemDialog<Asset>(parent, id) {
    RegenWidgets();
}

void AssetDialog::AddCustomWidgets() {
    auto *db = GetDatabase();

    mAssetTypeInput = new QComboBox();
    for (int i = 0; i < Roblox::AssetTypeCount; i++) {
        mAssetTypeInput->addItem(QString::fromStdString(Roblox::AssetTypeAsTranslatableString(static_cast<Roblox::AssetType>(i))));
    }
    mContentLayout->addRow("Type", mAssetTypeInput);

    CreatorInfoWidget* info = new CreatorInfoWidget();
    mContentLayout->addRow("Creator", info);

    if (mId != std::nullopt) {
        Statement stmt = db->PrepareStatement("SELECT * FROM Asset WHERE Id = ? ORDER BY Snapshot DESC LIMIT 1;");
        stmt.Bind(1, 1);
        if (stmt.Step() == SQLITE_ROW) {
            std::map<std::string, SqlValue> columns = stmt.GetColumnMap();
            
            int64_t id = std::get<int64_t>(columns["Id"]);
            std::string name = std::get<std::string>(columns["Name"]);
            std::string description = std::get<std::string>(columns["Description"]);

            mIdInput->setText(QString::number(id));
        } else {
            QMessageBox::critical(this, "Cannot Retrieve Asset", QString("Selecting columns from the asset failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
        }
    } else {
        /* Comment this out because ideally you'd actually want randomized ID's.
           Instead this gets the minimum positive ID in the database that isn't taken,
           which is bad because we have a system in place that allows databases to
           overlay ID's on top of each other. */
        // Statement stmt = db->PrepareStatement("SELECT COUNT(*) FROM Asset;");
        // if (stmt.Step() == SQLITE_ROW) {
        //     SqlValue val = stmt.GetValueFromColumnIndex(0);
        //     idInput->setText(QString::number(std::get<int>(val)));
        // }
    }

    // SqlRows rows = db->QueryTyped("SELECT * FROM Asset WHERE Id = ?;", SqlValue(1));
    // if (!rows.empty()) {
    //     SqlRow row = rows.at(0);
    // }
}

void AssetDialog::OnSave() {
    auto *db = GetDatabase();

    Statement stmt = db->PrepareStatement(R"(
        INSERT INTO Asset (Id, Snapshot, Name, Description, Created, Updated, Type) VALUES (?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT (Id, Snapshot) DO UPDATE SET Name = excluded.Name, Description = excluded.Description, Created = excluded.Created, Updated = excluded.Updated, Type = excluded.Type;
    )");
    stmt.Bind(1, mIdInput->text().toInt());
    stmt.Bind(2, 1);
    stmt.Bind(3, mNameInput->text().toStdString());
    stmt.Bind(4, mDescriptionInput->text().toStdString());
    stmt.Bind(5, static_cast<int64_t>(mCreatedInput->dateTime().toSecsSinceEpoch()));
    stmt.Bind(6, static_cast<int64_t>(mUpdatedInput->dateTime().toSecsSinceEpoch()));
    stmt.Bind(7, static_cast<int>(mAssetTypeInput->currentIndex()));

    if (stmt.Step() != SQLITE_DONE) {
        QMessageBox::critical(this, "Failed to Save Changes", QString("Saving changes to the database failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
        return;
    }
    db->MarkDirty();
    close();
}