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
// File: UniverseDialog.cpp
// Started by: Hattozo
// Started on: 2/8/2026
// Description:
#include "UniverseDialog.h"
#include "Sdk/CreatorInfoWidget.h"
#include "Sdk/Browser/ItemBrowserWidget.h"

#include <NoobWarrior/Roblox/Api/Asset.h>

#include <QMessageBox>
#include <QDateTimeEdit>
#include <QComboBox>

#include <cstdlib>

using namespace NoobWarrior;

UniverseDialog::UniverseDialog(QWidget *parent, std::optional<int64_t> id, std::optional<int64_t> snapshot) : ItemDialog(parent, id, snapshot) {
    RegenWidgets();
}

void UniverseDialog::AddCustomWidgets() {
    auto *db = GetDatabase();

    mAssetTypeInput = new QComboBox();
    for (int i = 0; i < Roblox::AssetTypeCount; i++) {
        mAssetTypeInput->addItem(QString::fromStdString(Roblox::AssetTypeAsTranslatableString(static_cast<Roblox::AssetType>(i))));
    }
    mContentLayout->addRow("Type", mAssetTypeInput);

    CreatorInfoWidget* info = new CreatorInfoWidget();
    mContentLayout->addRow("Creator", info);

    if (mId != std::nullopt) {
        std::string stmtStr = std::format("SELECT * FROM Universe WHERE Id = ? {};", mSnapshot.has_value() ? "AND Snapshot = ?" : "ORDER BY Snapshot DESC LIMIT 1");
        Statement stmt = db->PrepareStatement(stmtStr);
        stmt.Bind(1, mId.value());
        if (stmt.Step() == SQLITE_ROW) {
            std::map<std::string, SqlValue> columns = stmt.GetColumnMap();
            
            int64_t id = std::get<int64_t>(columns["Id"]);
            std::string name = std::get<std::string>(columns["Name"]);
            std::string description = std::get<std::string>(columns["Description"]);

            mIdInput->setText(QString::number(id));
        } else {
            QMessageBox::critical(this, "Cannot Retrieve Item", QString("Selecting columns from the table failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
        }
    }
}

void UniverseDialog::OnSave() {
    auto *db = GetDatabase();

    int64_t id = mIdInput->text().toInt();
    int snapshot = 1;
    std::string name = mNameInput->text().toStdString();
    std::string description = mDescriptionInput->text().toStdString();

    Statement stmt = db->PrepareStatement(R"(
        INSERT INTO Universe (Id, Snapshot, Name, Description, Created, Updated, Type) VALUES (?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT (Id, Snapshot) DO UPDATE SET LastRecorded = (unixepoch()), Name = excluded.Name, Description = excluded.Description, Created = excluded.Created, Updated = excluded.Updated, Type = excluded.Type;
    )");
    stmt.Bind(1, id);
    stmt.Bind(2, snapshot);
    stmt.Bind(3, name);
    stmt.Bind(4, description);
    stmt.Bind(5, static_cast<int64_t>(mCreatedInput->dateTime().toSecsSinceEpoch()));
    stmt.Bind(6, static_cast<int64_t>(mUpdatedInput->dateTime().toSecsSinceEpoch()));
    stmt.Bind(7, static_cast<int>(mAssetTypeInput->currentIndex()));

    if (stmt.Step() != SQLITE_DONE) {
        QMessageBox::critical(this, "Failed to Save Changes", QString("Saving changes to the database failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
        return;
    }
    db->MarkDirty();
    close();
    mSdk->GetItemBrowser()->Refresh();
}