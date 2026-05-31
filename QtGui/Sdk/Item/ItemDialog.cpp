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

    // Root layout: a body row (sidebar + scrollable content) above a fixed button box.
    mRootLayout = new QVBoxLayout(this);
    mLayout = new QHBoxLayout();
    mRootLayout->addLayout(mLayout, 1);

    mSidebarLayout = new QVBoxLayout();
    mLayout->addLayout(mSidebarLayout);

    mContentLayout = new QFormLayout();
    mContentLayout->setLabelAlignment(Qt::AlignRight);
    mContentLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    mContentLayout->setHorizontalSpacing(14);
    mContentLayout->setVerticalSpacing(6);
    mContentLayout->setContentsMargins(14, 6, 14, 6);

    // Editors can get quite tall (a Place asset in particular), so keep the form scrollable
    // while the icon sidebar and the Save/Cancel buttons stay pinned in place.
    auto *contentWidget = new QWidget();
    contentWidget->setLayout(mContentLayout);
    auto *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setWidget(contentWidget);
    mLayout->addWidget(scrollArea, 1);

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
        data = std::move(db->RetrieveImageData(mType, mId.has_value() ? mId.value() : -1));
    else
        data.assign(g_icon_content_deleted, g_icon_content_deleted + g_icon_content_deleted_size);

    image.loadFromData(data);
    mIcon->setPixmap(QPixmap::fromImage(image).scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    mUploadImageButton = new QPushButton("Upload Image");
    mUseExistingImageButton = new QPushButton("Use Existing Image");
    mSidebarLayout->addWidget(mUploadImageButton);
    mSidebarLayout->addWidget(mUseExistingImageButton);

    connect(mUploadImageButton, &QPushButton::clicked, [this]() {
        QString filePath = QFileDialog::getOpenFileName(
            this,
            "Change Icon",
            QString(),
            "Image File (*.png *.jpg *.jpeg *.bmp *.gif)"
        );
        if (!filePath.isEmpty()) {
            std::optional<int64_t> id = CreateImageAssetFromFile(filePath);
            if (!id.has_value())
                return;

            mImageIdInput->setText(QString::number(id.value()));

            QImage newImage(filePath);
            QPixmap newPixmap = QPixmap::fromImage(newImage);
            mIcon->setPixmap(newPixmap.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    });

    connect(mUseExistingImageButton, &QPushButton::clicked, [this]() {
        std::optional<int64_t> id = ItemOpenSaveDialog::GetOpenId(this, GetDatabase(), ItemType::Asset, Roblox::AssetType::Image, true);
        if (id.has_value()) {
            mImageIdInput->setText(QString::number(id.value()));

            std::vector<unsigned char> data = GetDatabase()->RetrieveImageData(ItemType::Asset, id.value());
            QImage image;
            if (image.loadFromData(data))
                mIcon->setPixmap(QPixmap::fromImage(image).scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    });

    // These types either don't have an ImageId column (Bundle, Outfit, Universe, User) or
    // manage their icon through a dedicated widget (Asset's Place thumbnails), so the generic
    // sidebar image controls don't apply to them.
    bool hasSidebarImage = !(mType == ItemType::Asset || mType == ItemType::Bundle ||
        mType == ItemType::Outfit || mType == ItemType::Universe || mType == ItemType::User);
    mUploadImageButton->setVisible(hasSidebarImage);
    mUseExistingImageButton->setVisible(hasSidebarImage);

    mSidebarLayout->addStretch();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, 2147483647);

    AddSectionHeader("General");

    mIdInput = new QLineEdit(QString::number(distrib(gen)));
    mIdInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mIdInput));
    mContentLayout->addRow("Id", mIdInput);

    mImageIdInput = new QLineEdit("0");
    mImageIdInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mImageIdInput));
    mContentLayout->addRow("Image Id", mImageIdInput);
    mContentLayout->setRowVisible(mImageIdInput, hasSidebarImage);

    mNameInput = new QLineEdit();
    mNameInput->setPlaceholderText("Cool Name");
    mContentLayout->addRow("Name", mNameInput);

    if (mId.has_value()) {
        // deserialization
        Statement stmt = db->PrepareStatement(std::format("SELECT Id, Name FROM \"{}\" WHERE Id = ?;", GetTableNameFromItemType(mType)));
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
    case ItemType::Badge:
        Badge_AddFields();
        break;
    case ItemType::Bundle:
        Bundle_AddFields();
        break;
    case ItemType::DevProduct:
        DevProduct_AddFields();
        break;
    case ItemType::Group:
        Group_AddFields();
        break;
    case ItemType::Outfit:
        Outfit_AddFields();
        break;
    case ItemType::Pass:
        Pass_AddFields();
        break;
    case ItemType::Set:
        Set_AddFields();
        break;
    case ItemType::Universe:
        Universe_AddFields();
        break;
    case ItemType::User:
        User_AddFields();
        break;
    }

    mButtonBox = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Save, this);
    mButtonBox->setContentsMargins(14, 0, 14, 8);
    mRootLayout->addWidget(mButtonBox);

    connect(mButtonBox, &QDialogButtonBox::accepted, this, &ItemDialog::OnSave);

    connect(mButtonBox, &QDialogButtonBox::rejected, this, [&]() {
        close();
    });

    setMinimumWidth(560);
    resize(620, 760);
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
        Statement checkStmt = db->PrepareStatement(std::format("SELECT COUNT(*) FROM \"{}\" WHERE Id = ?;", tableName));
        checkStmt.Bind(1, newId);
        if (checkStmt.Step() == SQLITE_ROW && checkStmt.GetIntFromColumnIndex(0) > 0) {
            QMessageBox::critical(this, "Cannot Save",
                QString("An item with ID %1 already exists.").arg(newId));
            return;
        }

        Statement renameStmt = db->PrepareStatement(std::format("UPDATE \"{}\" SET Id = ? WHERE Id = ?;", tableName));
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
    case ItemType::Badge:
        if (!Badge_OnSave()) return;
        break;
    case ItemType::Bundle:
        if (!Bundle_OnSave()) return;
        break;
    case ItemType::DevProduct:
        if (!DevProduct_OnSave()) return;
        break;
    case ItemType::Group:
        if (!Group_OnSave()) return;
        break;
    case ItemType::Outfit:
        if (!Outfit_OnSave()) return;
        break;
    case ItemType::Pass:
        if (!Pass_OnSave()) return;
        break;
    case ItemType::Set:
        if (!Set_OnSave()) return;
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

void ItemDialog::AddSectionHeader(const QString &title) {
    auto *header = new QLabel(title.toUpper());
    header->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    header->setStyleSheet(
        "font-weight: bold;"
        "letter-spacing: 1px;"
        "color: palette(bright-text);"
        "padding: 8px 0 3px 0;"
        "margin-top: 4px;"
        "border-bottom: 1px solid palette(mid);"
    );
    mContentLayout->addRow(header);
}

void ItemDialog::AddOwnedItemFields() {
    auto *db = GetDatabase();

    AddSectionHeader("Details");

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
        Statement stmt = db->PrepareStatement(std::format("SELECT Description, Created, Updated FROM \"{}\" WHERE Id = ?;", GetTableNameFromItemType(mType)));
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

std::optional<int64_t> ItemDialog::CreateImageAssetFromFile(const QString &filePath) {
    auto *db = GetDatabase();

    if (!std::filesystem::exists(filePath.toStdString())) {
        QMessageBox::critical(this, "Error", "Unable to open file");
        return std::nullopt;
    }

    std::filesystem::path stdFilePath = std::filesystem::path(filePath.toStdString());

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, 2147483647);

    int64_t id = distrib(gen);
    if (db->DoesItemExist(ItemType::Asset, id)) {
        QMessageBox::critical(this, "Weird Error", "Tried generating a random ID for image and it somehow conflicted with another ID. Try again.");
        return std::nullopt;
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
        return std::nullopt;
    }
    SqlDb::Response attachRes = db->AttachBlobHashToAsset(id, 1, hash);
    if (attachRes != SqlDb::Response::Success) {
        QMessageBox::critical(this, "Error", "Could not attach image because attaching the blob to the image asset failed.");
        return std::nullopt;
    }

    return id;
}

EmuDb* ItemDialog::GetDatabase() {
    return mDb;
}