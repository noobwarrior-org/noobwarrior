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

    std::string tableName = GetTableNameFromItemType(mType);

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

    std::vector<unsigned char> data;

    QImage image;

    if (mId.has_value())
        data = std::move(db->RetrieveImageData(tableName, mId.has_value() ? mId.value() : -1));
    else
        data.assign(g_icon_content_deleted, g_icon_content_deleted + g_icon_content_deleted_size);

    image.loadFromData(data);
    mIcon->setPixmap(QPixmap::fromImage(image).scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    if (!(mType == ItemType::Asset || mType == ItemType::User)) {
        auto *uploadImage = new QPushButton("Upload Image");
        auto *useExistingImage = new QPushButton("Use Existing Image");
        mSidebarLayout->addWidget(uploadImage);
        mSidebarLayout->addWidget(useExistingImage);

        connect(uploadImage, &QPushButton::clicked, [this]() {
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
        });

        connect(useExistingImage, &QPushButton::clicked, [this]() {
            // TODO: Add ItemOpenSaveDialog here
            int id = ItemOpenSaveDialog::GetOpenId(this, GetDatabase(), ItemType::Asset, Roblox::AssetType::Image, true);
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

    if (mNameInput->text().isEmpty()) {
        QMessageBox::critical(
            this,
            "Cannot Save Changes",
            "Please enter a name that is not empty."
        );
        return;
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
        Statement stmt = db->PrepareStatement(std::format("SELECT Id, Name, Description, Created, Updated FROM {} WHERE Id = ?;", GetTableNameFromItemType(mType)));
        stmt.Bind(1, mId.value());
        if (stmt.Step() == SQLITE_ROW) {
            int id = stmt.GetIntFromColumnIndex(0);
            std::string name = stmt.GetStringFromColumnIndex(1);
            std::string desc = stmt.GetStringFromColumnIndex(2);

            mIdInput->setText(QString::number(id));
            mNameInput->setText(QString::fromStdString(name));
            mOwned_DescriptionInput->setText(QString::fromStdString(desc));
            mOwned_CreatedInput->setDateTime(QDateTime::fromSecsSinceEpoch(stmt.GetIntFromColumnIndex(3)));
            mOwned_UpdatedInput->setDateTime(QDateTime::fromSecsSinceEpoch(stmt.GetIntFromColumnIndex(4)));
        } else {
            QMessageBox::critical(this, "Cannot Retrieve Item", QString("Selecting columns from the table failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
        }
    }
}

EmuDb* ItemDialog::GetDatabase() {
    return mDb;
}