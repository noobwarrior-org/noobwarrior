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
// File: ItemDialog_Impl_Asset.cpp
// Started by: Hattozo
// Started on: 3/25/2026
// Description:
#include "ItemDialog.h"
#include "Sdk/CreatorInfoWidget.h"

using namespace NoobWarrior;

void ItemDialog::Asset_AddFields() {
    AddOwnedItemFields();
    Asset_AddAssetTypeWidgets();

    auto *dataFrame = new QFrame();
    auto *dataLayout = new QVBoxLayout(dataFrame);

    auto *dataLabel = new QLabel("No data attached");
    dataLabel->setWordWrap(true);

    auto *dataButton = new QPushButton("Set Data");

    if (mId.has_value()) {
        Statement dataInfoStmt = GetDatabase()->PrepareStatement("SELECT Version, DataHash FROM AssetData WHERE Id = ? ORDER BY Version DESC LIMIT 1");
        dataInfoStmt.Bind(1, mId.value());
        int res = dataInfoStmt.Step();
        if (res == SQLITE_ROW) {
            int64_t version = dataInfoStmt.GetIntFromColumnIndex(0);
            std::string hash = dataInfoStmt.GetStringFromColumnIndex(1);

            dataLabel->setText(QString("Data Hash: %1\nVersion: %2").arg(QString::fromStdString(hash)).arg(version));
        } else if (res != SQLITE_DONE) {
            dataLabel->setText("Failed to retrieve information about data");
        }
    }

    connect(dataButton, &QPushButton::clicked, [this]() {
        QString filePath = QFileDialog::getOpenFileName(this, "Set Data", QString(), "Anything (*.*)");
        if (!filePath.isEmpty()) {
            std::ifstream file(filePath.toStdString());

            if (!file.is_open()) {
                QMessageBox::critical(this, "Error", "Unable to open file");
                return;
            }

            std::vector<unsigned char> data;
            std::vector<unsigned char> buf(1024);
            while (file.read(reinterpret_cast<char*>(buf.data()), buf.size())) {
                data.insert(data.end(), buf.begin(), buf.end());
            }
            SqlDb::Response res = GetDatabase()->AddBlob(data);
            if (res != SqlDb::Response::Success && res != SqlDb::Response::DidNothing) {
                QString errMsg;
                switch (res) {
                default: errMsg = "The reason is unknown."; break;
                case SqlDb::Response::Busy: errMsg = "The database is busy."; break;
                case SqlDb::Response::Misuse: errMsg = "The SQLite API was misused."; break;
                case SqlDb::Response::ConstraintViolation: errMsg = "A constraint was violated."; break;
                }
                QMessageBox::critical(this, "Error", "Unable to add file to database\n" + errMsg);
                return;
            }
        }
    });

    dataLayout->addWidget(dataLabel);
    dataLayout->addWidget(dataButton);

    mContentLayout->addRow("Data", dataFrame);
}

void ItemDialog::Asset_AddAssetTypeWidgets() {
    auto *db = GetDatabase();

    Roblox::AssetType type = Roblox::AssetType::Model;

    mAsset_AssetTypeInput = new QComboBox();
    for (int i = 0; i < Roblox::AssetTypeCount; i++) {
        mAsset_AssetTypeInput->addItem(QString::fromStdString(Roblox::AssetTypeAsTranslatableString(static_cast<Roblox::AssetType>(i))));
    }
    mContentLayout->addRow("Type", mAsset_AssetTypeInput);

    if (mId.has_value()) {
        Statement stmt = db->PrepareStatement("SELECT Type FROM Asset WHERE Id = ?;");
        stmt.Bind(1, mId.value());
        if (stmt.Step() == SQLITE_ROW)
            type = static_cast<Roblox::AssetType>(stmt.GetIntFromColumnIndex(0));
    }

    if (type == Roblox::AssetType::Model) {

    }
}

bool ItemDialog::Asset_OnSave() {
    auto *db = GetDatabase();

    int64_t id = mIdInput->text().toInt();
    std::string name = mNameInput->text().toStdString();
    std::string description = mOwned_DescriptionInput->text().toStdString();

    Statement stmt = db->PrepareStatement(R"(
        INSERT INTO Asset (Id, Name, Description, Created, Updated, Type) VALUES (?, ?, ?, ?, ?, ?)
        ON CONFLICT (Id) DO UPDATE SET LastRecorded = (unixepoch()), Name = excluded.Name, Description = excluded.Description, Created = excluded.Created, Updated = excluded.Updated, Type = excluded.Type;
    )");
    stmt.Bind(1, id);
    stmt.Bind(2, name);
    stmt.Bind(3, description);
    stmt.Bind(4, static_cast<int64_t>(mOwned_CreatedInput->dateTime().toSecsSinceEpoch()));
    stmt.Bind(5, static_cast<int64_t>(mOwned_UpdatedInput->dateTime().toSecsSinceEpoch()));
    stmt.Bind(6, static_cast<int>(mAsset_AssetTypeInput->currentIndex()));

    if (stmt.Step() != SQLITE_DONE) {
        QMessageBox::critical(this, "Failed to Save Changes", QString("Saving changes to the database failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
        return false;
    }
    return true;
}
