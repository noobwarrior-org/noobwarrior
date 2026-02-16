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
// File: ItemDialog.cpp
// Started by: Hattozo
// Started on: 2/8/2026
// Description: Dialog window that allows you to edit or create an item.
#include "ItemDialog.h"
#include "ItemOpenSaveDialog.h"
#include "NoobWarrior/EmuDb/ItemType.h"
#include "Sdk/CreatorInfoWidget.h"

#include <random>

using namespace NoobWarrior;

ItemDialog::ItemDialog(EmuDb* db, ItemType type, std::optional<int> id, QWidget *parent) :
    QDialog(parent),
    mDb(db),
    mType(type),
    mId(id),
    mAssetTypeInput(nullptr)
{
    setWindowTitle("Item Editor");
    RegenWidgets();
}

void ItemDialog::RegenWidgets() {
    auto *db = GetDatabase();
    if (db == nullptr) {
        QMessageBox::critical(this, "Error", "Database is null");
        close();
        return;
    }

    setWindowTitle(tr("Configure %1").arg(QString::fromStdString(Item::TypeName)));

    qDeleteAll(findChildren<QWidget*>("", Qt::FindDirectChildrenOnly));
    mLayout = new QHBoxLayout(this);
    mSidebarLayout = new QVBoxLayout();
    mContentLayout = new QFormLayout();

    mLayout->addLayout(mSidebarLayout);
    mLayout->addLayout(mContentLayout);

    ////////////////////////////////////////////////////////////////////////
    /// icon
    ////////////////////////////////////////////////////////////////////////
    mIcon = new QLabel();
    mIcon->setAlignment(Qt::AlignLeft);
    mSidebarLayout->addWidget(mIcon);
    // mSidebarLayout->addStretch();

    if (!(Item::TypeName.compare("Asset") == 0 || Item::TypeName.compare("User") == 0)) {
        auto *changeIcon = new QPushButton("Change Icon");
        mSidebarLayout->addWidget(changeIcon);
        connect(changeIcon, &QPushButton::clicked, [this]() {
            // TODO: Add ItemOpenSaveDialog here
            int id = ItemOpenSaveDialog::GetOpenId(this, GetDatabase(), ItemType::Asset, Roblox::AssetType::Image, true);
            /*
            QString filePath = QFileDialog::getOpenFileName(
                this,
                "Change Icon",
                QString(),
                "Image File (*.png *.jpg *.jpeg *.bmp *.gif)"
            );
            if (!filePath.isEmpty()) {
                std::ifstream file(filePath.toStdString());

                if (!file.is_open()) {
                    QMessageBox::critical(this, "Error", "Unable to open file");
                    return;
                }

                QImage newImage(filePath);
                QPixmap newPixmap = QPixmap::fromImage(newImage);
                mIcon->setPixmap(newPixmap.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
            */
        });
    }

    mSidebarLayout->addStretch();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, 2147483647);

    mIdInput = new QLineEdit(QString::number(distrib(gen)));
    mContentLayout->addRow("Id", mIdInput);

    mNameInput = new QLineEdit();
    mNameInput->setPlaceholderText("Cool Name");
    mContentLayout->addRow("Name", mNameInput);

    mDescriptionInput = new QLineEdit();
    mDescriptionInput->setPlaceholderText("Describe your item here");
    mContentLayout->addRow("Description", mDescriptionInput);

    mCreatedInput = new QDateTimeEdit();
    mCreatedInput->setDate(QDate::currentDate());
    mContentLayout->addRow("Created", mCreatedInput);

    mUpdatedInput = new QDateTimeEdit();
    mUpdatedInput->setDate(QDate::currentDate());
    mContentLayout->addRow("Updated", mUpdatedInput);

    if (mType == ItemType::Asset) {
        AddAssetWidgets();
    }

    std::vector<unsigned char> data;

    QImage image;

    if (mId.has_value())
        data = std::move(db->RetrieveImageData("Asset", mId.has_value() ? mId.value() : -1));
    else
        data.assign(g_icon_content_deleted, g_icon_content_deleted + g_icon_content_deleted_size);

    image.loadFromData(data);
    mIcon->setPixmap(QPixmap::fromImage(image).scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    if (mId.has_value()) {
        // deserialization
        Statement stmt = db->PrepareStatement("SELECT Id, Name, Description, Created, Updated FROM Asset WHERE Id = ?;");
        stmt.Bind(1, mId.value());
        if (stmt.Step() == SQLITE_ROW) {
            int id = stmt.GetIntFromColumnIndex(0);
            std::string name = stmt.GetStringFromColumnIndex(1);
            std::string desc = stmt.GetStringFromColumnIndex(2);

            mIdInput->setText(QString::number(id));
            mNameInput->setText(QString::fromStdString(name));
            mDescriptionInput->setText(QString::fromStdString(desc));
            mCreatedInput->setDateTime(QDateTime::fromSecsSinceEpoch(stmt.GetIntFromColumnIndex(3)));
            mUpdatedInput->setDateTime(QDateTime::fromSecsSinceEpoch(stmt.GetIntFromColumnIndex(4)));
        } else {
            QMessageBox::critical(this, "Cannot Retrieve Item", QString("Selecting columns from the table failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
        }
    }

    mButtonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Save, this);
    mContentLayout->addWidget(mButtonBox);

    connect(mButtonBox, &QDialogButtonBox::accepted, this, &ItemDialog::OnSave);

    connect(mButtonBox, &QDialogButtonBox::rejected, this, [&]() {
        close();
    });
}

void ItemDialog::AddAssetWidgets() {
    CreatorInfoWidget* info = new CreatorInfoWidget();
    mContentLayout->addRow("Creator", info);

    AddAssetTypeWidgets();

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

void ItemDialog::AddAssetTypeWidgets() {
    auto *db = GetDatabase();

    Roblox::AssetType type = Roblox::AssetType::Model;

    mAssetTypeInput = new QComboBox();
    for (int i = 0; i < Roblox::AssetTypeCount; i++) {
        mAssetTypeInput->addItem(QString::fromStdString(Roblox::AssetTypeAsTranslatableString(static_cast<Roblox::AssetType>(i))));
    }
    mContentLayout->addRow("Type", mAssetTypeInput);

    if (mId.has_value()) {
        Statement stmt = db->PrepareStatement("SELECT Type FROM Asset WHERE Id = ?;");
        stmt.Bind(1, mId.value());
        if (stmt.Step() == SQLITE_ROW)
            type = static_cast<Roblox::AssetType>(stmt.GetIntFromColumnIndex(0));
    }

    if (type == Roblox::AssetType::Model) {

    }
}

void ItemDialog::OnSave() {
    auto *db = GetDatabase();

    int64_t id = mIdInput->text().toInt();
    std::string name = mNameInput->text().toStdString();
    std::string description = mDescriptionInput->text().toStdString();

    if (mType == ItemType::Asset) {
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
    }
    
    db->MarkDirty();
    close();
}

EmuDb* ItemDialog::GetDatabase() {
    return mDb;
}