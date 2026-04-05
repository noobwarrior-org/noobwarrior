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
// Description: An unholy abomination that implements the Asset type for ItemDialog
#include "ItemDialog.h"
#include "Sdk/CreatorInfoWidget.h"

using namespace NoobWarrior;

void ItemDialog::Asset_AddFields() {
    AddOwnedItemFields();
    Asset_AddAssetTypeWidgets();

    auto *dataFrame = new QFrame();
    auto *dataLayout = new QVBoxLayout(dataFrame);

    mAsset_DataView = new QTreeView();
    mAsset_DataView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mAsset_DataView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    mAsset_DataModel = new QStandardItemModel(mAsset_DataView);
    mAsset_DataModel->setColumnCount(4);
    mAsset_DataModel->setHorizontalHeaderLabels({"", "Version", "Hash", "Size"});
    mAsset_DataView->setModel(mAsset_DataModel);
    mAsset_DataView->setColumnHidden(0, true); // The first column cannot be reordered, so we just make it blank and hide it.

    mAsset_DataView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(mAsset_DataView, &QTreeView::customContextMenuRequested, [this](const QPoint &point) {
        QModelIndex index = mAsset_DataView->indexAt(point);
        if (!index.isValid())
            return;

        QPoint globalPos = mAsset_DataView->viewport()->mapToGlobal(point);

        QMenu myMenu;
        QAction* del = myMenu.addAction(QIcon(":/images/silk/cross.png"), "Delete Item");

        connect(del, &QAction::triggered, [this, index]() {
            QMessageBox::StandardButton button = QMessageBox::warning(this, "Delete Item", "Are you sure you want to delete this item?", QMessageBox::Yes | QMessageBox::No);
            if (button != QMessageBox::Yes)
                return;

            int row = index.row();
            QStandardItem* versionColumn = mAsset_DataModel->item(row, 1);
            QStandardItem* hashColumn = mAsset_DataModel->item(row, 2);
            int version = versionColumn->text().toInt();
            QString hash = hashColumn->text();

            // Some data like PendingAdd and file paths are located in the data of the Version field.
            bool isPendingAdd = versionColumn->data(Qt::UserRole).toBool();
            if (isPendingAdd) {
                // find and remove from mAsset_DataPendingFiles by matching the filepath
                // we stored filepath in Qt::UserRole+1 on the version column when you create the row
                QString filePath = versionColumn->data(Qt::UserRole + 1).toString();
                auto it = std::find(mAsset_DataPendingFiles.begin(), mAsset_DataPendingFiles.end(), filePath);
                if (it != mAsset_DataPendingFiles.end())
                    mAsset_DataPendingFiles.erase(it);
            } else {
                mAsset_DataPendingDeleteVersions.push_back(version);
            }
            mAsset_DataModel->removeRow(row);
        });

        myMenu.exec(globalPos);
    });

    if (mId.has_value()) {
        Statement dataInfoStmt = GetDatabase()->PrepareStatement("SELECT Version, DataHash FROM AssetData WHERE Id = ? ORDER BY Version DESC;");
        dataInfoStmt.Bind(1, mId.value());
        int res = dataInfoStmt.Step();
        if (res != SQLITE_DONE && res != SQLITE_ROW) {
            auto *label = new QLabel("Failed to retrieve information about data");
            dataLayout->addWidget(label);
        }
        while (res == SQLITE_ROW) {
            int version = dataInfoStmt.GetIntFromColumnIndex(0);
            std::string hash = dataInfoStmt.GetStringFromColumnIndex(1);

            auto *versionItem = new QStandardItem(QString::number(version));
            versionItem->setData(false, Qt::UserRole); // false = not a pending-add
            QList<QStandardItem*> row;
            row
                << new QStandardItem("")
                << versionItem
                << new QStandardItem(QString::fromStdString(hash))
                << new QStandardItem("N/A");
            mAsset_DataModel->appendRow(row);
            res = dataInfoStmt.Step();
        }
    }

    auto *dataButton = new QPushButton("Add Data");

    connect(dataButton, &QPushButton::clicked, [this]() {
        QString filePath = QFileDialog::getOpenFileName(this, "Add Data", QString(), "Anything (*.*)");
        if (!filePath.isEmpty()) {
            mAsset_DataPendingFiles.push_back(filePath); // Push onto a list of pending files so that we can add them to the database when the user clicks Save.

            // Placeholder row to visualize what it will look like to the user before clicking Save
            int previewVersion = mAsset_DataModel->rowCount() + 1;
            auto *versionItem = new QStandardItem("(Pending) " + QString::number(previewVersion));
            versionItem->setData(true, Qt::UserRole); // true = pending-add
            versionItem->setData(filePath, Qt::UserRole + 1); // also include file path lol
            QList<QStandardItem*> row;
            row
                << new QStandardItem("")
                << versionItem
                << new QStandardItem("N/A")
                << new QStandardItem("N/A");
            mAsset_DataModel->appendRow(row);
        }
    });

    dataLayout->addWidget(mAsset_DataView);
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

    int id = mIdInput->text().toInt();
    std::string name = mNameInput->text().toStdString();
    std::string description = mOwned_DescriptionInput->text().toStdString();

    Statement stmt = db->PrepareStatement(R"(
        INSERT INTO Asset (Id, Name, Description, Created, Updated, Type) VALUES (?, ?, ?, ?, ?, ?)
        ON CONFLICT (Id) DO UPDATE SET LastRecorded = (unixepoch()), Name = excluded.Name, Description = excluded.Description, Created = excluded.Created, Updated = excluded.Updated, Type = excluded.Type;
    )");
    stmt.Bind(1, id);
    stmt.Bind(2, name);
    stmt.Bind(3, description);
    stmt.Bind(4, mOwned_CreatedInput->dateTime().toSecsSinceEpoch());
    stmt.Bind(5, mOwned_UpdatedInput->dateTime().toSecsSinceEpoch());
    stmt.Bind(6, mAsset_AssetTypeInput->currentIndex());
    if (stmt.Step() != SQLITE_DONE) {
        QMessageBox::critical(this, "Failed to Save Changes", QString("Saving changes to the database failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
        return false;
    }

    for (QString filePath : mAsset_DataPendingFiles) {
        std::ifstream file(filePath.toStdString(), std::ios::binary);
        if (!file.is_open()) {
            QMessageBox::critical(this, "Error", "Unable to open file");
            return false;
        }

        std::string hashOutput;
        std::vector<unsigned char> data {
            std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>()
        };
        SqlDb::Response res = GetDatabase()->AddBlob(data, &hashOutput);
        if (res != SqlDb::Response::Success && res != SqlDb::Response::DidNothing) {
            QString errMsg;
            switch (res) {
                default: errMsg = "The reason is unknown."; break;
                case SqlDb::Response::Busy: errMsg = "The database is busy."; break;
                case SqlDb::Response::Misuse: errMsg = "The SQLite API was misused."; break;
                case SqlDb::Response::ConstraintViolation: errMsg = "A constraint was violated."; break;
            }
            QMessageBox::critical(this, "Error", "Unable to add file to database\n" + errMsg);
            return false;
        }
        Statement dataStmt = db->PrepareStatement(
            "INSERT INTO AssetData (Id, Version, DataHash) VALUES (?, (SELECT COALESCE(MAX(Version), 0) + 1 FROM AssetData WHERE Id = ?), ?);"
        );
        dataStmt.Bind(1, id);
        dataStmt.Bind(2, id); // subquery also needs the id
        dataStmt.Bind(3, hashOutput);
        if (dataStmt.Step() != SQLITE_DONE) {
            QMessageBox::critical(this, "Failed to Save Changes",
                QString("Failed to add data.\nLast error: %1")
                    .arg(QString::fromStdString(db->GetLastErrorMsg())));
            return false;
        }
    }

    for (int version : mAsset_DataPendingDeleteVersions) {
        Statement delStmt = db->PrepareStatement("DELETE FROM AssetData WHERE Id = ? AND Version = ?;");
        delStmt.Bind(1, id);
        delStmt.Bind(2, version);
        if (delStmt.Step() != SQLITE_DONE) {
            QMessageBox::critical(this, "Failed to Save Changes",
                QString("Failed to delete version %1.\nLast error: %2")
                    .arg(version)
                    .arg(QString::fromStdString(db->GetLastErrorMsg())));
            return false;
        }
    }
    return true;
}
