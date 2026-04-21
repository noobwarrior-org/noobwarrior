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

#include <QRegularExpressionValidator>

using namespace NoobWarrior;

ItemDialog::ItemDialog(EmuDb* db, ItemType type, std::optional<int64_t> id, QWidget *parent) :
    QDialog(parent),
    mDb(db),
    mType(type),
    mId(id),
    mAsset_AssetTypeInput(nullptr)
{
    setWindowTitle("Item Editor");
    RegenWidgets();

    assert(dynamic_cast<Sdk*>(parent) != nullptr && "ItemDialog must be parented to Sdk");
}

void ItemDialog::RegenWidgets() {
    auto *db = GetDatabase();
    if (db == nullptr) {
        QMessageBox::critical(this, "Error", "Database is null");
        close();
        return;
    }

    auto *sdk = dynamic_cast<Sdk*>(this->parent());

    std::string tableName = GetTableNameFromItemType(mType);
    setWindowTitle(tr("Configure %1").arg(QString::fromStdString(tableName)));

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

    std::vector<unsigned char> data;

    QImage image;

    if (mId.has_value())
        data = std::move(db->RetrieveImageData(tableName, mId.has_value() ? mId.value() : -1));
    else
        data.assign(g_icon_content_deleted, g_icon_content_deleted + g_icon_content_deleted_size);

    image.loadFromData(data);
    mIcon->setPixmap(QPixmap::fromImage(image).scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    mUploadImageButton = new QPushButton("Upload Image");
    mUseExistingImageButton = new QPushButton("Use Existing Image");
    mSidebarLayout->addWidget(mUploadImageButton);
    mSidebarLayout->addWidget(mUseExistingImageButton);

    connect(mUploadImageButton, &QPushButton::clicked, [this, db]() {
        QString filePath = QFileDialog::getOpenFileName(
            this,
            "Change Icon",
            QString(),
            "Image File (*.png *.jpg *.jpeg *.bmp *.gif)"
        );
        if (!filePath.isEmpty()) {
            if (!std::filesystem::exists(filePath.toStdString())) {
                QMessageBox::critical(this, "Error", "Unable to open file");
                return;
            }

            std::filesystem::path stdFilePath = std::filesystem::path(filePath.toStdString());

            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> distrib(0, 2147483647);

            int64_t id = distrib(gen);
            if (db->DoesItemExist(ItemType::Asset, id)) {
                QMessageBox::critical(this, "Weird Error", "Tried generating a random ID for image and it somehow conflicted with another ID. Try again.");
                return;
            }
            db->AddItem(ItemType::Asset, {
                {"Id", id},
                {"Name", "ImageAsset"},
                {"Type", static_cast<int>(Roblox::AssetType::Image)}
            });

            std::string hash;
            SqlDb::Response blobRes = db->AddBlob(stdFilePath, &hash);
            if (blobRes != SqlDb::Response::Success && blobRes != SqlDb::Response::DidNothing) {
                QMessageBox::critical(this, "Error", "Could not attach image because adding the blob failed.");
                return;
            }
            SqlDb::Response attachRes = db->AttachBlobHashToAsset(id, 1, hash);
            if (attachRes != SqlDb::Response::Success) {
                QMessageBox::critical(this, "Error", "Could not attach image because attaching the blob to the image asset failed.");
                return;
            }

            mImageIdInput->setText(QString::number(id));

            QImage newImage(filePath);
            QPixmap newPixmap = QPixmap::fromImage(newImage);
            mIcon->setPixmap(newPixmap.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    });

    connect(mUseExistingImageButton, &QPushButton::clicked, [this]() {
        int64_t id = ItemOpenSaveDialog::GetOpenId(this, GetDatabase(), ItemType::Asset, Roblox::AssetType::Image, true);
    });

    mUploadImageButton->setVisible(!(mType == ItemType::Asset || mType == ItemType::User || mType == ItemType::Universe));
    mUseExistingImageButton->setVisible(!(mType == ItemType::Asset || mType == ItemType::User || mType == ItemType::Universe));

    mSidebarLayout->addStretch();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, 2147483647);

    mIdInput = new QLineEdit(QString::number(distrib(gen)));
    mIdInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mIdInput));
    mContentLayout->addRow("Id", mIdInput);

    mImageIdInput = new QLineEdit("0");
    mImageIdInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mImageIdInput));
    mContentLayout->addRow("Image Id", mImageIdInput);
    mContentLayout->setRowVisible(mImageIdInput, !(mType == ItemType::Asset || mType == ItemType::User || mType == ItemType::Universe));

    mNameInput = new QLineEdit();
    mNameInput->setPlaceholderText("Cool Name");
    mContentLayout->addRow("Name", mNameInput);

    if (mId.has_value()) {
        // deserialization
        Statement stmt = db->PrepareStatement(std::format("SELECT Id, Name FROM {} WHERE Id = ?;", GetTableNameFromItemType(mType)));
        stmt.Bind(1, mId.value());
        int stepResult = stmt.Step();
        if (stepResult == SQLITE_ROW) {
            int64_t id = stmt.GetInt64FromColumnIndex(0);
            std::string name = stmt.GetStringFromColumnIndex(1);

            mIdInput->setText(QString::number(id));
            mNameInput->setText(QString::fromStdString(name));
        } else {
            QMessageBox::critical(this, "Cannot Retrieve Item", QString("Selecting columns from the table failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
        }
    }

    switch (mType) {
    default:
        QMessageBox::warning(
            this,
            "Warning",
            "The item type you are trying to configure does not have a custom implementation for this screen. Your changes will not be saved."
        );
        break;
    case ItemType::Asset:
        Asset_AddFields();
        break;
    case ItemType::Universe:
        Universe_AddFields();
        break;
    case ItemType::User:
        User_AddFields();
        break;
    }

    mButtonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Save, this);
    mContentLayout->addWidget(mButtonBox);

    connect(mButtonBox, &QDialogButtonBox::accepted, this, &ItemDialog::OnSave);

    connect(mButtonBox, &QDialogButtonBox::rejected, this, [&]() {
        close();
    });
}

void ItemDialog::OnSave() {
    auto *db = GetDatabase();
    std::string tableName = GetTableNameFromItemType(mType);

    if (mNameInput->text().isEmpty()) {
        QMessageBox::critical(
            this,
            "Cannot Save Changes",
            "Please enter a name that is not empty."
        );
        return;
    }

    bool ok;
    int64_t newId = mIdInput->text().toLongLong(&ok);
    if (!ok) {
        QMessageBox::critical(this, "Cannot Save", "ID is not a valid number.");
        return;
    }
    int64_t oldId = mId.has_value() ? mId.value() : newId;
    bool idChanged = mId.has_value() && newId != oldId;

    if (idChanged) {
        Statement checkStmt = db->PrepareStatement(std::format("SELECT COUNT(*) FROM {} WHERE Id = ?;", tableName));
        checkStmt.Bind(1, newId);
        if (checkStmt.Step() == SQLITE_ROW && checkStmt.GetIntFromColumnIndex(0) > 0) {
            QMessageBox::critical(this, "Cannot Save",
                QString("An item with ID %1 already exists.").arg(newId));
            return;
        }

        Statement renameStmt = db->PrepareStatement(std::format("UPDATE {} SET Id = ? WHERE Id = ?;", tableName));
        renameStmt.Bind(1, newId);
        renameStmt.Bind(2, oldId);
        if (renameStmt.Step() != SQLITE_DONE) {
            QMessageBox::critical(this, "Cannot Save",
                QString("Failed to change ID.\nLast error: %1")
                    .arg(QString::fromStdString(db->GetLastErrorMsg())));
            return;
        }
    }

    switch (mType) {
    default:
        QMessageBox::warning(
            this,
            "Cannot Save Changes",
            "No save implementation has been made for this item type. Your changes have not been saved."
        );
        return;
    case ItemType::Asset:
        if (!Asset_OnSave()) return;
        break;
    case ItemType::Universe:
        if (!Universe_OnSave()) return;
        break;
    case ItemType::User:
        if (!User_OnSave()) return;
        break;
    }
    
    db->MarkDirty();
    close();

    Sdk* sdk = dynamic_cast<Sdk*>(this->parent());
    if (sdk != nullptr) {
        sdk->Refresh();
    }
}

void ItemDialog::AddOwnedItemFields() {
    auto *db = GetDatabase();

    mOwned_DescriptionInput = new QLineEdit();
    mOwned_DescriptionInput->setPlaceholderText("Describe your item here");
    mContentLayout->addRow("Description", mOwned_DescriptionInput);

    mOwned_CreatedInput = new QDateTimeEdit();
    mOwned_CreatedInput->setDateTime(QDateTime::currentDateTime());
    mContentLayout->addRow("Created", mOwned_CreatedInput);

    mOwned_UpdatedInput = new QDateTimeEdit();
    mOwned_UpdatedInput->setDateTime(QDateTime::currentDateTime());
    mContentLayout->addRow("Updated", mOwned_UpdatedInput);

    mOwned_CreatorInfoWidget = new CreatorInfoWidget();
    mContentLayout->addRow("Creator", mOwned_CreatorInfoWidget);

    if (mId.has_value()) {
        // deserialization
        Statement stmt = db->PrepareStatement(std::format("SELECT Description, Created, Updated FROM {} WHERE Id = ?;", GetTableNameFromItemType(mType)));
        stmt.Bind(1, mId.value());
        if (stmt.Step() == SQLITE_ROW) {
            std::string desc = stmt.GetStringFromColumnIndex(0);

            mOwned_DescriptionInput->setText(QString::fromStdString(desc));
            mOwned_CreatedInput->setDateTime(QDateTime::fromSecsSinceEpoch(stmt.GetIntFromColumnIndex(1)));
            mOwned_UpdatedInput->setDateTime(QDateTime::fromSecsSinceEpoch(stmt.GetIntFromColumnIndex(2)));
        } else {
            QMessageBox::critical(this, "Cannot Retrieve Item", QString("Selecting columns from the table failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
        }
    }
}

EmuDb* ItemDialog::GetDatabase() {
    return mDb;
}