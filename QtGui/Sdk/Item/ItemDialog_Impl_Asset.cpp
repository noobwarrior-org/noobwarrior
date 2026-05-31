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
#include "ItemOpenSaveDialog.h"
#include "Sdk/CreatorInfoWidget.h"
#include <NoobWarrior/EmuDb/Item/Asset.h>

#include <QMenu>
#include <QRegularExpressionValidator>

using namespace NoobWarrior;

void ItemDialog::Asset_AddFields() {
    AddOwnedItemFields();
    Asset_AddFields_AssetType();

    bool isPublic = false;
    int64_t minMembership = 0;
    int64_t contentRating = 0;
    if (mId.has_value()) {
        Statement stmt = GetDatabase()->PrepareStatement("SELECT Public, MinimumMembershipLevel, ContentRatingTypeId FROM Asset WHERE Id = ?;");
        stmt.Bind(1, mId.value());
        if (stmt.Step() == SQLITE_ROW) {
            isPublic = stmt.GetIntFromColumnIndex(0);
            minMembership = stmt.GetInt64FromColumnIndex(1);
            contentRating = stmt.GetInt64FromColumnIndex(2);
        }
    }

    mAsset_PublicInput = new QCheckBox();
    mAsset_PublicInput->setChecked(isPublic);
    mContentLayout->addRow("Public", mAsset_PublicInput);

    mAsset_MinMembershipInput = new QLineEdit(QString::number(minMembership));
    mAsset_MinMembershipInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mAsset_MinMembershipInput));
    mContentLayout->addRow("Min. Membership Level", mAsset_MinMembershipInput);

    mAsset_ContentRatingInput = new QLineEdit(QString::number(contentRating));
    mAsset_ContentRatingInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mAsset_ContentRatingInput));
    mContentLayout->addRow("Content Rating Type", mAsset_ContentRatingInput);

    // Asset statistics live in the separate AssetHistorical table.
    bool isNew = false;
    int64_t sales = 0;
    int64_t favorites = 0;
    int64_t likes = 0;
    int64_t dislikes = 0;
    if (mId.has_value()) {
        Statement stmt = GetDatabase()->PrepareStatement("SELECT IsNew, Sales, Favorites, Likes, Dislikes FROM AssetHistorical WHERE Id = ?;");
        stmt.Bind(1, mId.value());
        if (stmt.Step() == SQLITE_ROW) {
            isNew = stmt.GetIntFromColumnIndex(0);
            sales = stmt.GetInt64FromColumnIndex(1);
            favorites = stmt.GetInt64FromColumnIndex(2);
            likes = stmt.GetInt64FromColumnIndex(3);
            dislikes = stmt.GetInt64FromColumnIndex(4);
        }
    }

    mAsset_IsNewInput = new QCheckBox();
    mAsset_IsNewInput->setChecked(isNew);
    mContentLayout->addRow("New", mAsset_IsNewInput);

    mAsset_SalesInput = new QLineEdit(QString::number(sales));
    mAsset_SalesInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mAsset_SalesInput));
    mContentLayout->addRow("Sales", mAsset_SalesInput);

    mAsset_FavoritesInput = new QLineEdit(QString::number(favorites));
    mAsset_FavoritesInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mAsset_FavoritesInput));
    mContentLayout->addRow("Favorites", mAsset_FavoritesInput);

    mAsset_LikesInput = new QLineEdit(QString::number(likes));
    mAsset_LikesInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mAsset_LikesInput));
    mContentLayout->addRow("Likes", mAsset_LikesInput);

    mAsset_DislikesInput = new QLineEdit(QString::number(dislikes));
    mAsset_DislikesInput->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), mAsset_DislikesInput));
    mContentLayout->addRow("Dislikes", mAsset_DislikesInput);

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

            bool ok;
            int version = versionColumn->text().toLongLong(&ok);
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
        Statement stmt = GetDatabase()->PrepareStatement("SELECT ImageId FROM Asset WHERE Id = ?;");
        stmt.Bind(1, mId.value());
        if (stmt.Step() == SQLITE_ROW) {
            int64_t imageId = stmt.GetIntFromColumnIndex(0);
            mImageIdInput->setText(QString::number(imageId));
        }

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

            // Auto set the Asset Type field to the appropriate type according to the file type
            const std::filesystem::path &stdPath = filePath.toStdString();
            Roblox::AssetType type = Asset_GetAssetTypeFromFileType(stdPath);
            if (type != Roblox::AssetType::None) {
                int index = mAsset_AssetTypeInput->findData(QVariant::fromValue(type));
                if (index != -1)
                    mAsset_AssetTypeInput->setCurrentIndex(index);

                if (type == Roblox::AssetType::Image) {
                    QMessageBox::question(this, "Question", "Would you like to create a decal?");
                }
            }
        }
    });

    dataLayout->addWidget(mAsset_DataView);
    dataLayout->addWidget(dataButton);

    mContentLayout->addRow("Data", dataFrame);
}

void ItemDialog::Asset_AddFields_AssetType() {
    auto *db = GetDatabase();

    mAsset_AssetCategoryInput = new QComboBox();
    for (int i = 0; i < AssetCategoryCount; i++) {
        AssetCategory assetCategory = static_cast<AssetCategory>(i);
        std::string assetCategoryStr = AssetCategoryAsTranslatableString(assetCategory);
        mAsset_AssetCategoryInput->addItem(QString::fromStdString(assetCategoryStr));
    }
    mContentLayout->addRow("Category", mAsset_AssetCategoryInput);

    mAsset_AssetTypeInput = new QComboBox();
    connect(mAsset_AssetCategoryInput, &QComboBox::currentIndexChanged, [this](int index) {
        mAsset_AssetTypeInput->clear();
        AssetCategory assetCategory = static_cast<AssetCategory>(index);
        for (int i = 0; i < Roblox::AssetTypeCount; i++) {
            Roblox::AssetType assetType = static_cast<Roblox::AssetType>(i);
            std::string assetTypeStr = Roblox::AssetTypeAsTranslatableString(assetType);
            if (MapAssetTypeToCategory(assetType) == assetCategory && assetTypeStr.compare("None") != 0) {
                mAsset_AssetTypeInput->addItem(QString::fromStdString(assetTypeStr), QVariant::fromValue(assetType));
            }
        }
    });
    connect(mAsset_AssetTypeInput, &QComboBox::currentIndexChanged, [this](int index) {
        Roblox::AssetType assetType = mAsset_AssetTypeInput->itemData(index).value<Roblox::AssetType>();
        Asset_SetVisibilityOfAssetTypeWidgets(assetType);
    });
    mContentLayout->addRow("Type", mAsset_AssetTypeInput);

    Asset_AddFields_Place();

    // in order to fire the events we attached above without manual user intervention
    emit mAsset_AssetCategoryInput->currentIndexChanged(0);
    emit mAsset_AssetTypeInput->currentIndexChanged(0);

    Roblox::AssetType type = Roblox::AssetType::Model;

    // deserialize asset type
    if (mId.has_value()) {
        Statement stmt = db->PrepareStatement("SELECT Type FROM Asset WHERE Id = ?;");
        stmt.Bind(1, mId.value());
        if (stmt.Step() == SQLITE_ROW)
            type = static_cast<Roblox::AssetType>(stmt.GetIntFromColumnIndex(0));
    }

    if (MapAssetTypeToCategory(type) == AssetCategory::AvatarItem) {
        mAsset_AssetCategoryInput->setCurrentIndex(1); // 1 because that's where the avatar item category is
    }

    int index = mAsset_AssetTypeInput->findData(QVariant::fromValue(type));
    if (index != -1)
        mAsset_AssetTypeInput->setCurrentIndex(index);
}

void ItemDialog::Asset_AddFields_Place() {
    mAsset_Place_ThumbnailFrame = new QFrame();
    auto *thumbnailLayout = new QVBoxLayout(mAsset_Place_ThumbnailFrame);

    mAsset_Place_ThumbnailList = new QListWidget();
    mAsset_Place_ThumbnailList->setIconSize(QSize(64, 64));

    // Right-click a thumbnail to remove it.
    mAsset_Place_ThumbnailList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(mAsset_Place_ThumbnailList, &QListWidget::customContextMenuRequested, [this](const QPoint &point) {
        QListWidgetItem* item = mAsset_Place_ThumbnailList->itemAt(point);
        if (item == nullptr)
            return;

        QMenu menu;
        QAction* del = menu.addAction(QIcon(":/images/silk/cross.png"), "Remove Thumbnail");
        connect(del, &QAction::triggered, [this, item]() {
            int64_t thumbnailId = item->data(Qt::UserRole).toLongLong();
            bool isPendingAdd = item->data(Qt::UserRole + 1).toBool();
            if (isPendingAdd) {
                mAsset_Place_DataPendingThumbnails.removeAll(thumbnailId);
            } else {
                mAsset_Place_DataPendingDeleteThumbnails.push_back(thumbnailId);
            }
            delete mAsset_Place_ThumbnailList->takeItem(mAsset_Place_ThumbnailList->row(item));
        });
        menu.exec(mAsset_Place_ThumbnailList->viewport()->mapToGlobal(point));
    });

    // Populate the existing thumbnails for this place.
    if (mId.has_value()) {
        Statement stmt = GetDatabase()->PrepareStatement("SELECT Thumbnail FROM AssetPlaceThumbnail WHERE Id = ?;");
        stmt.Bind(1, mId.value());
        while (stmt.Step() == SQLITE_ROW) {
            if (stmt.IsColumnIndexNull(0))
                continue;
            Asset_AddThumbnailToList(stmt.GetInt64FromColumnIndex(0), false);
        }
    }

    mAsset_Place_UploadThumbnailButton = new QPushButton("Upload Thumbnail");
    connect(mAsset_Place_UploadThumbnailButton, &QPushButton::clicked, [this]() {
        QString filePath = QFileDialog::getOpenFileName(
            this,
            "Upload Thumbnail",
            QString(),
            "Image File (*.png *.jpg *.jpeg *.bmp *.gif)"
        );
        if (filePath.isEmpty())
            return;

        std::optional<int64_t> thumbnailId = CreateImageAssetFromFile(filePath);
        if (!thumbnailId.has_value())
            return;

        mAsset_Place_DataPendingThumbnails.push_back(thumbnailId.value());
        Asset_AddThumbnailToList(thumbnailId.value(), true);
    });

    mAsset_Place_AddThumbnailFromExistingImageButton = new QPushButton("Add Thumbnail From Existing Image");
    connect(mAsset_Place_AddThumbnailFromExistingImageButton, &QPushButton::clicked, [this]() {
        std::optional<int64_t> thumbnailId = ItemOpenSaveDialog::GetOpenId(this, GetDatabase(), ItemType::Asset, Roblox::AssetType::Image, true);
        if (!thumbnailId.has_value())
            return;

        // Don't allow the same image to be added twice (the table's key is (Id, Thumbnail)).
        if (mAsset_Place_DataPendingThumbnails.contains(thumbnailId.value())) {
            QMessageBox::information(this, "Already Added", "That image is already a thumbnail for this place.");
            return;
        }
        for (int i = 0; i < mAsset_Place_ThumbnailList->count(); i++) {
            if (mAsset_Place_ThumbnailList->item(i)->data(Qt::UserRole).toLongLong() == thumbnailId.value()) {
                QMessageBox::information(this, "Already Added", "That image is already a thumbnail for this place.");
                return;
            }
        }

        mAsset_Place_DataPendingThumbnails.push_back(thumbnailId.value());
        // If it was queued for deletion this session, adding it back cancels that.
        mAsset_Place_DataPendingDeleteThumbnails.removeAll(thumbnailId.value());
        Asset_AddThumbnailToList(thumbnailId.value(), true);
    });

    thumbnailLayout->addWidget(mAsset_Place_ThumbnailList);
    thumbnailLayout->addWidget(mAsset_Place_UploadThumbnailButton);
    thumbnailLayout->addWidget(mAsset_Place_AddThumbnailFromExistingImageButton);

    mContentLayout->addRow("Thumbnails", mAsset_Place_ThumbnailFrame);
}

void ItemDialog::Asset_AddThumbnailToList(int64_t thumbnailId, bool pendingAdd) {
    auto *listItem = new QListWidgetItem();
    listItem->setText(pendingAdd
        ? QString("(Pending) Image %1").arg(thumbnailId)
        : QString("Image %1").arg(thumbnailId));
    listItem->setData(Qt::UserRole, static_cast<qlonglong>(thumbnailId));
    listItem->setData(Qt::UserRole + 1, pendingAdd);

    std::vector<unsigned char> data = GetDatabase()->RetrieveImageData(ItemType::Asset, thumbnailId);
    QImage image;
    if (image.loadFromData(data))
        listItem->setIcon(QPixmap::fromImage(image));

    mAsset_Place_ThumbnailList->addItem(listItem);
}

void ItemDialog::Asset_SetVisibilityOfAssetTypeWidgets(Roblox::AssetType type) {
    // mAsset_Place_ThumbnailFrame->setVisible(type == Roblox::AssetType::Place);
    mUploadImageButton->setVisible(type == Roblox::AssetType::Place);
    mUseExistingImageButton->setVisible(type == Roblox::AssetType::Place);
    mContentLayout->setRowVisible(mImageIdInput, type == Roblox::AssetType::Place);
    mContentLayout->setRowVisible(mAsset_Place_ThumbnailFrame, type == Roblox::AssetType::Place);
}

Roblox::AssetType ItemDialog::Asset_GetAssetTypeFromFileType(const std::filesystem::path &path) {
    Roblox::AssetType type = Roblox::AssetType::None;
    std::ifstream ifs(path);
    if (ifs.fail())
        return type;
    std::vector<char> buf(256);
    ifs.read(buf.data(), 256);
    return type;
}

static constexpr const char* sTablesThatNeedToBeUpdated[] = {
    "AssetData",
    "AssetHistorical",
    "AssetMicrotransaction",
    "AssetPlaceAttributes",
    "AssetPlaceGearType",
    "AssetPlaceThumbnail"
};

bool ItemDialog::Asset_OnSave() {
    auto *db = GetDatabase();

    int64_t newId = mIdInput->text().toLongLong();
    int64_t oldId = mId.has_value() ? mId.value() : newId;
    bool idChanged = mId.has_value() && oldId != newId;

    if (idChanged) {
        // id changed, pls update these other tables!!!
        for (int i = 0; i < std::size(sTablesThatNeedToBeUpdated); i++) {
            Statement updateStmt = db->PrepareStatement(std::format("UPDATE {} SET Id = ? WHERE Id = ?;", sTablesThatNeedToBeUpdated[i]));
            updateStmt.Bind(1, newId);
            updateStmt.Bind(2, oldId);
            if (updateStmt.Step() != SQLITE_DONE) {
                QMessageBox::critical(this, "Cannot Save",
                    QString("Failed to update Id field in %1 table.\nLast error: %2")
                    .arg(sTablesThatNeedToBeUpdated[i])
                    .arg(QString::fromStdString(db->GetLastErrorMsg())));
                return false;
            }
        }
    }

    std::string name = mNameInput->text().toStdString();
    std::string description = mOwned_DescriptionInput->text().toStdString();
    int64_t imageId = mImageIdInput->text().toLongLong();
    bool isPublic = mAsset_PublicInput->isChecked();
    int64_t minMembership = mAsset_MinMembershipInput->text().toLongLong();
    int64_t contentRating = mAsset_ContentRatingInput->text().toLongLong();

    Statement stmt = db->PrepareStatement(R"(
        INSERT INTO Asset (Id, Name, Description, Created, Updated, ImageId, Type, Public, MinimumMembershipLevel, ContentRatingTypeId) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT (Id) DO UPDATE SET
            LastRecorded = (unixepoch()),
            Name = excluded.Name,
            Description = excluded.Description,
            Created = excluded.Created,
            Updated = excluded.Updated,
            ImageId = excluded.ImageId,
            Type = excluded.Type,
            Public = excluded.Public,
            MinimumMembershipLevel = excluded.MinimumMembershipLevel,
            ContentRatingTypeId = excluded.ContentRatingTypeId;
    )");
    stmt.Bind(1, newId);
    stmt.Bind(2, name);
    stmt.Bind(3, description);
    stmt.Bind(4, static_cast<int64_t>(mOwned_CreatedInput->dateTime().toSecsSinceEpoch()));
    stmt.Bind(5, static_cast<int64_t>(mOwned_UpdatedInput->dateTime().toSecsSinceEpoch()));
    stmt.Bind(6, imageId);
    stmt.Bind(7, static_cast<int>(mAsset_AssetTypeInput->currentData().value<Roblox::AssetType>()));
    stmt.Bind(8, isPublic);
    stmt.Bind(9, minMembership);
    stmt.Bind(10, contentRating);
    if (stmt.Step() != SQLITE_DONE) {
        QMessageBox::critical(this, "Failed to Save Changes", QString("Saving changes to the database failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
        return false;
    }

    // Persist asset statistics into the AssetHistorical table.
    {
        bool isNew = mAsset_IsNewInput->isChecked();
        int64_t sales = mAsset_SalesInput->text().toLongLong();
        int64_t favorites = mAsset_FavoritesInput->text().toLongLong();
        int64_t likes = mAsset_LikesInput->text().toLongLong();
        int64_t dislikes = mAsset_DislikesInput->text().toLongLong();

        Statement histStmt = db->PrepareStatement(R"(
            INSERT INTO AssetHistorical (Id, IsNew, Sales, Favorites, Likes, Dislikes) VALUES (?, ?, ?, ?, ?, ?)
            ON CONFLICT (Id) DO UPDATE SET
                IsNew = excluded.IsNew,
                Sales = excluded.Sales,
                Favorites = excluded.Favorites,
                Likes = excluded.Likes,
                Dislikes = excluded.Dislikes;
        )");
        histStmt.Bind(1, newId);
        histStmt.Bind(2, isNew);
        histStmt.Bind(3, sales);
        histStmt.Bind(4, favorites);
        histStmt.Bind(5, likes);
        histStmt.Bind(6, dislikes);
        if (histStmt.Step() != SQLITE_DONE) {
            QMessageBox::critical(this, "Failed to Save Changes", QString("Saving changes to the database failed.\nLast error message: %1").arg(QString::fromStdString(db->GetLastErrorMsg())), QMessageBox::Ok);
            return false;
        }
    }

    for (QString filePath : mAsset_DataPendingFiles) {
        std::filesystem::path fsPath(filePath.toStdString());
        std::string hashOutput;
        SqlDb::Response res = GetDatabase()->AddBlob(fsPath, &hashOutput);
        if (res != SqlDb::Response::Success && res != SqlDb::Response::DidNothing) {
            QString errMsg;
            switch (res) {
            default: errMsg = "The reason is unknown."; break;
            case SqlDb::Response::Busy: errMsg = "The database is busy."; break;
            case SqlDb::Response::Misuse: errMsg = "The SQLite API was misused."; break;
            case SqlDb::Response::ConstraintViolation: errMsg = "A constraint was violated."; break;
            case SqlDb::Response::BlobOpenFailed: errMsg = "Failed to open the blob."; break;
            case SqlDb::Response::BlobCompressionFailed: errMsg = "Failed to compress the given file."; break;
            }
            QMessageBox::critical(this, "Error", "Unable to add file to database\n" + errMsg);
            return false;
        }
        Statement dataStmt = db->PrepareStatement(
            "INSERT INTO AssetData (Id, Version, DataHash) VALUES (?, (SELECT COALESCE(MAX(Version), 0) + 1 FROM AssetData WHERE Id = ?), ?);"
        );
        dataStmt.Bind(1, newId);
        dataStmt.Bind(2, newId); // subquery also needs the id
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
        delStmt.Bind(1, newId);
        delStmt.Bind(2, version);
        if (delStmt.Step() != SQLITE_DONE) {
            QMessageBox::critical(this, "Failed to Save Changes",
                QString("Failed to delete version %1.\nLast error: %2")
                    .arg(version)
                    .arg(QString::fromStdString(db->GetLastErrorMsg())));
            return false;
        }
    }

    // Persist place thumbnails (only relevant for Place assets, but harmless otherwise).
    for (int64_t thumbnailId : mAsset_Place_DataPendingDeleteThumbnails) {
        Statement delStmt = db->PrepareStatement("DELETE FROM AssetPlaceThumbnail WHERE Id = ? AND Thumbnail = ?;");
        delStmt.Bind(1, newId);
        delStmt.Bind(2, thumbnailId);
        if (delStmt.Step() != SQLITE_DONE) {
            QMessageBox::critical(this, "Failed to Save Changes",
                QString("Failed to remove thumbnail %1.\nLast error: %2")
                    .arg(thumbnailId)
                    .arg(QString::fromStdString(db->GetLastErrorMsg())));
            return false;
        }
    }

    for (int64_t thumbnailId : mAsset_Place_DataPendingThumbnails) {
        Statement insStmt = db->PrepareStatement("INSERT OR IGNORE INTO AssetPlaceThumbnail (Id, Thumbnail) VALUES (?, ?);");
        insStmt.Bind(1, newId);
        insStmt.Bind(2, thumbnailId);
        if (insStmt.Step() != SQLITE_DONE) {
            QMessageBox::critical(this, "Failed to Save Changes",
                QString("Failed to add thumbnail %1.\nLast error: %2")
                    .arg(thumbnailId)
                    .arg(QString::fromStdString(db->GetLastErrorMsg())));
            return false;
        }
    }
    return true;
}
