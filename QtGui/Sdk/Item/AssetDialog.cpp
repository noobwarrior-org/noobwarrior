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
// File: AssetDialog.cpp
// Started by: Hattozo
// Started on: 11/30/2025
// Description:
#include "AssetDialog.h"
#include "NoobWarrior/EmuDb/ContentImages.h"
#include "Sdk/CreatorInfoWidget.h"
#include "Sdk/Item/Browser/ItemBrowserWidget.h"

#include <NoobWarrior/Roblox/Api/Asset.h>

#include <QMessageBox>
#include <QDateTimeEdit>
#include <QComboBox>

#include <cstdlib>

using namespace NoobWarrior;

AssetDialog::AssetDialog(QWidget *parent, std::optional<int> id) : ItemDialog(parent, id) {
    RegenWidgets();
}

void AssetDialog::AddCustomWidgets() {
    auto *db = GetDatabase();

    std::vector<unsigned char> data;

    QImage image;

    if (mId.has_value())
        data = std::move(db->RetrieveImageData("Asset", mId.has_value() ? mId.value() : -1));
    else
        data.assign(g_icon_content_deleted, g_icon_content_deleted + g_icon_content_deleted_size);

    image.loadFromData(data);
    mIcon->setPixmap(QPixmap::fromImage(image).scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    CreatorInfoWidget* info = new CreatorInfoWidget();
    mContentLayout->addRow("Creator", info);

    mAssetTypeInput = new QComboBox();
    for (int i = 0; i < Roblox::AssetTypeCount; i++) {
        mAssetTypeInput->addItem(QString::fromStdString(Roblox::AssetTypeAsTranslatableString(static_cast<Roblox::AssetType>(i))));
    }
    mContentLayout->addRow("Type", mAssetTypeInput);

    if (mId.has_value()) {
        Statement stmt = db->PrepareStatement("SELECT * FROM Asset WHERE Id = ?;");
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

    AddTypeWidgets();

    auto *dataFrame = new QFrame();
    auto *dataLayout = new QVBoxLayout(dataFrame);

    auto *dataLabel = new QLabel("No data attached");
    auto *dataButton = new QPushButton("Set Data");

    connect(dataButton, &QPushButton::clicked, [this]() {
        QString filePath = QFileDialog::getOpenFileName(this, "Set Data", QString(), "Anything (*.*)");
        if (!filePath.isEmpty()) {
            std::ifstream file(filePath.toStdString());

            if (!file.is_open()) {
                QMessageBox::critical(this, "Error", "Unable to open file");
                return;
            }

            std::vector<unsigned char> data = {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
            SqlDb::Response res = GetDatabase()->AddBlob(data);
            if (res != SqlDb::Response::Success && res != SqlDb::Response::DidNothing) {
                QMessageBox::critical(this, "Error", "Unable to add file to database");
                return;
            }
        }
    });

    dataLayout->addWidget(dataLabel);
    dataLayout->addWidget(dataButton);

    mContentLayout->addRow("Data", dataFrame);
}

void AssetDialog::AddTypeWidgets() {
    auto *db = GetDatabase();

    Roblox::AssetType type = Roblox::AssetType::Model;

    if (mId.has_value()) {
        Statement stmt = db->PrepareStatement("SELECT Type FROM Asset WHERE Id = ?;");
        stmt.Bind(1, mId.value());
        if (stmt.Step() == SQLITE_ROW)
            type = static_cast<Roblox::AssetType>(stmt.GetIntFromColumnIndex(0));
    }

    if (type == Roblox::AssetType::Model) {

    }
}

void AssetDialog::OnSave() {
    auto *db = GetDatabase();

    int64_t id = mIdInput->text().toInt();
    std::string name = mNameInput->text().toStdString();
    std::string description = mDescriptionInput->text().toStdString();

    Statement stmt = db->PrepareStatement(R"(
        INSERT INTO Asset (Id, Name, Description, Created, Updated, Type) VALUES (?, ?, ?, ?, ?, ?)
        ON CONFLICT (Id) DO UPDATE SET LastRecorded = (unixepoch()), Name = excluded.Name, Description = excluded.Description, Created = excluded.Created, Updated = excluded.Updated, Type = excluded.Type;
    )");
    stmt.Bind(1, id);
    stmt.Bind(2, name);
    stmt.Bind(3, description);
    stmt.Bind(4, static_cast<int64_t>(mCreatedInput->dateTime().toSecsSinceEpoch()));
    stmt.Bind(5, static_cast<int64_t>(mUpdatedInput->dateTime().toSecsSinceEpoch()));
    stmt.Bind(6, static_cast<int>(mAssetTypeInput->currentIndex()));

    if (stmt.Step() != SQLITE_DONE) {
        QMessageBox::critical(this, "Failed to Save Changes", QString("Saving changes to the database failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
        return;
    }
    db->MarkDirty();
    close();
    mSdk->GetItemBrowser()->Refresh();
}